// scanmanager.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <CommCtrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <memory>
#include "cached_files.h"
#include "docwrite.h"
#include "docread.h"
#include "imagelist.h"
#include "m_argv.h"
#include "pargb32.h"
#include "prometheusdb.h"
#include "promuser.h"
#include "scanning.h"
#include "scanmanager.h"
#include "effectdlg.h"
#include "WiaAutomationProxy.h"

// setup application manifest to load GDI+ 1.1
#if defined(_M_AMD64)
#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.GdiPlus' version='1.1.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.GdiPlus' version='1.1.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
static void         ShowError(const char *title, const char *message, HWND hWnd = nullptr);

// More globals - command line arguments
char **argv;
int    argc;

// Statics
static TWAINManager   twainMgr;   // scanning manager object
static PrometheusUser theUser;    // Prometheus user object
static HWND           mainWnd;    // main window
static HWND           toolbarWnd; // toolbar control
static HWND           rebarWnd;   // rebar control
static std::string    personID;   // person ID passed on command line
static std::string    docTitle;   // document title passed on command line
static std::string    docRecv;    // date document was received, passed on cmd line
static bool           viewMode;   // if true, viewing existing document
static std::string    viewPath;   // path of document to view
static bool           isPDF;      // if true, this is a PDF document
static bool           modified;   // if true, document is modified but not saved

static bool           canEdit = true; // if false, document mutate commands cannot be unlocked

static ScanManagerEffectDlg *pEffectDlg; // current modeless effect dialog

//=============================================================================
//
// Image List Management
//

// Statics
static ImageList  gImageList;    // list of scanned-in image objects
static ImageNode *gCurrentImage; // currently viewed image

// Forward declarations

static void ScanMgr_EnableOneCmd(HMENU hMenu, UINT cmd);
static void ScanMgr_DisableOneCmd(HMENU hMenu, UINT cmd);
static void ScanMgr_DisableDocumentMutateCmds(bool lockCanEdit);
static void ScanMgr_EnableDocumentMutateCmds();
static void ScanMgr_DisableGDIPlusEditCmds();
static void ScanMgr_EnableGDIPlusEditCmds();

//
// Update the availability of next/previous image navigation commands
// depending on the state of the image list.
//
static void ScanMgr_UpdateViewImgCmds()
{
   HMENU hViewMenu = GetSubMenu(GetMenu(mainWnd), 1);

   // if a modeless effect dialog is open, neither command can be used.
   if(pEffectDlg)
   {
      ScanMgr_DisableOneCmd(hViewMenu, ID_VIEW_PREVIOUSIMAGE);
      ScanMgr_DisableOneCmd(hViewMenu, ID_VIEW_NEXTIMAGE);
      return;
   }

   // handle previous button: should be enabled if there is a valid image
   // and it is not the first on the list.
   if(gCurrentImage && gCurrentImage != gImageList.head->dllObject)
      ScanMgr_EnableOneCmd(hViewMenu, ID_VIEW_PREVIOUSIMAGE);
   else
      ScanMgr_DisableOneCmd(hViewMenu, ID_VIEW_PREVIOUSIMAGE);

   // handle next button: should be enabled if there is a valid image
   // and it is not the last on the list.
   if(gCurrentImage && gCurrentImage->links.dllNext)
      ScanMgr_EnableOneCmd(hViewMenu, ID_VIEW_NEXTIMAGE);
   else
      ScanMgr_DisableOneCmd(hViewMenu, ID_VIEW_NEXTIMAGE);
}

//
// Calculate the client rect of the image drawing area of the main window. 
// This has the rebar control's height subtracted from the parent rect.
//
static RECT ScanMgr_CalcImageRect()
{
   RECT barRect;
   RECT mainRect;

   GetClientRect(mainWnd,  &mainRect);
   GetClientRect(rebarWnd, &barRect);

   mainRect.top = barRect.bottom;

   return mainRect;
}

//
// Set the currently viewed image.
//
void ScanMgr_SetCurrentImage(ImageNode *node)
{
   gCurrentImage = node;
   RECT mainRect = ScanMgr_CalcImageRect();
   InvalidateRect(mainWnd, &mainRect, TRUE);
   ScanMgr_UpdateViewImgCmds();
   
   if(gCurrentImage)
      ScanMgr_EnableGDIPlusEditCmds();
   else
      ScanMgr_DisableGDIPlusEditCmds();
}

//
// Empty the images list.
//
void ScanMgr_ClearImageList()
{
   while(gImageList.head)
      delete gImageList.head->dllObject;

   ScanMgr_SetCurrentImage(nullptr);
}

//
// Create a new image and add it to the image list
//
void ScanMgr_AddNewImage(HBITMAP hBitmap)
{
   auto newImage = new ImageNode();
   newImage->hBitmap = hBitmap;
   gImageList.tailInsert(newImage);
   ScanMgr_SetCurrentImage(newImage);
}

//
// Go to next image on image list
//
void ScanMgr_GotoNextImage()
{
   if(gCurrentImage && gCurrentImage->links.dllNext)
      ScanMgr_SetCurrentImage(gCurrentImage->links.dllNext->dllObject);   
}

//
// Go to previous image on image list
//
void ScanMgr_GotoPrevImage()
{
   if(!gCurrentImage || gCurrentImage == gImageList.head->dllObject)
      return;

   auto link = gImageList.head;

   // find the previous image on the list.
   while(link)
   {
      if(link->dllNext == &gCurrentImage->links)
      {
         ScanMgr_SetCurrentImage(link->dllObject);
         return;
      }
      link = link->dllNext;
   }
}

//
// Images to view in document view mode have been loaded;
// Set them up for viewing.
//
void ScanMgr_SetupViewImages()
{
   if(gImageList.head)
      ScanMgr_SetCurrentImage(gImageList.head->dllObject);

   HMENU hFileMenu = GetSubMenu(GetMenu(mainWnd), 0);
   ScanMgr_EnableOneCmd(hFileMenu, ID_FILE_PRINT);
}

//
// Free all image resources loaded by the program.
//
void ScanMgr_ShutdownImages()
{
   ScanMgr_ClearImageList();
   ScanMgr_DeleteImageBuffers();
}

//=============================================================================
//
// Manage Image Effect Dialogs
//

static void ScanMgr_SpawnEffectDialog(fxtype_e type)
{
   if(!gCurrentImage)
      return; // need an image to edit

   if(pEffectDlg)
      return; // already have one active.

   if((pEffectDlg = new (std::nothrow) ScanManagerEffectDlg(type, gCurrentImage)))
   {
      if(!pEffectDlg->create(mainWnd))
      {
         delete pEffectDlg;
         pEffectDlg = nullptr;
      }
   }

   if(!pEffectDlg)
      ShowError("Scan Manager", "Operation failed", mainWnd);
   else
   {
      ScanMgr_DisableDocumentMutateCmds(false);
      ScanMgr_DisableGDIPlusEditCmds();
      ScanMgr_UpdateViewImgCmds();
   }
}

//=============================================================================
//
// GDI+ Graphics
//

static ULONG_PTR gdiplusToken; // startup token for GDI+ API
static bool      gdiplusInit;
static bool      comInit;

//
// Initialize the GDI+ library
//
static bool ScanMgr_InitGDIPlus()
{
   CoInitializeEx(nullptr, COINIT_MULTITHREADED);
   comInit = true;

   Gdiplus::GdiplusStartupInput gdiplusStartupInput;
   if(Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) == Gdiplus::Ok)
      gdiplusInit = true;

   return gdiplusInit;
}

//
// Shutdown the GDI+ library
//
static void ScanMgr_ShutdownGDIPlus()
{
   if(gdiplusInit)
      Gdiplus::GdiplusShutdown(gdiplusToken);

   if(comInit)
      CoUninitialize();
}

//
// Convert TWAIN-derived HBITMAP to Gdiplus::Bitmap
//
void ScanMgr_HBITMAPToGdiplusBitmap(ImageNode *node)
{
   HBITMAP hBmp = node->hBitmap;
   if(!hBmp)
      return;

   auto dib = PBITMAPINFO(GlobalLock(hBmp));
   if(!dib)
      return;

   DWORD paletteSize = 0;
   switch(dib->bmiHeader.biBitCount)
   {
   case 1:
      paletteSize = 2;
      break;
   case 4:
      paletteSize = 16;
      break;
   case 8:
      paletteSize = 256;
      break;
   case 24:
      break;
   default:
      return;
   }

   node->gdiBitmap = new Gdiplus::Bitmap(dib, (uint8_t *)dib + sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * paletteSize));
   GlobalUnlock(hBmp);
}

//
// Rotate and/or flip images
//
bool ScanMgr_FlipAndRotate(Gdiplus::RotateFlipType rft)
{
   if(!gCurrentImage)
      return true;

   // Apply the selected transformation
   bool res = false;
   Gdiplus::Bitmap *pBmp = gCurrentImage->gdiBitmap;
   if(pBmp && pBmp->GetLastStatus() == Gdiplus::Ok)
      res = (pBmp->RotateFlip(rft) == Gdiplus::Ok);

   // refresh image view
   ScanMgr_SetCurrentImage(gCurrentImage);

   return res;
}

//
// Paint function
//
static void OnPaint(HDC hdc)
{
   Gdiplus::Bitmap *bitmap;
   Gdiplus::Rect    gxRect, dstRect;
   int              dstWidth, dstHeight;

   if(!gCurrentImage)
      return;

   if(!gCurrentImage->gdiBitmap)
      ScanMgr_HBITMAPToGdiplusBitmap(gCurrentImage);

   if(!(bitmap = gCurrentImage->gdiBitmap))
      return;

   if(bitmap->GetLastStatus() != Gdiplus::Ok)
      return;

   Gdiplus::Graphics graphics(hdc);
   graphics.GetVisibleClipBounds(&gxRect);

   RECT rebarRect;
   GetClientRect(rebarWnd, &rebarRect);

   dstHeight = gxRect.Height - (rebarRect.bottom - rebarRect.top + 1);
   dstWidth  = (dstHeight * bitmap->GetWidth()) / bitmap->GetHeight();
   
   if(dstWidth > gxRect.Width)
   {
      dstWidth  = gxRect.Width;
      dstHeight = (dstWidth * bitmap->GetHeight()) / bitmap->GetWidth();
   }

   dstRect.X = (gxRect.Width  - dstWidth ) / 2;
   dstRect.Y = (gxRect.Height - dstHeight + rebarRect.bottom) / 2;
   dstRect.Width  = dstWidth;
   dstRect.Height = dstHeight;

   graphics.DrawImage(bitmap, dstRect);
}

//=============================================================================
//
// Command line parser
//

static void UnEscapeQuotes(char *arg)
{
   char *last = nullptr;

   while(*arg)
   {
      if(*arg == '"' && (last != nullptr && *last == '\\'))
      {
         char *c_curr = arg;
         char *c_last = last;

         while(*c_curr)
         {
            *c_last = *c_curr;
            c_last = c_curr;
            c_curr++;
         }
         *c_last = '\0';
      }
      last = arg;
      arg++;
   }
}

static int ParseCommandLine(char *cmdline, char **argv)
{
   char *bufp;
   char *lastp = nullptr;
   int argc, last_argc;

   argc = last_argc = 0;
   for(bufp = cmdline; *bufp; )
   {
      // skip leading whitespace
      while(isspace(*bufp))
         ++bufp;

      // skip over argument
      if(*bufp == '"')
      {
         ++bufp;
         if(*bufp)
         {
            if(argv)
               argv[argc] = bufp;
            ++argc;
         }
         // skip over word
         lastp = bufp;
         while(*bufp && (*bufp != '"' || *lastp == '\\'))
         {
            lastp = bufp;
            ++bufp;
         }
      }
      else
      {
         if(*bufp)
         {
            if(argv)
               argv[argc] = bufp;
            ++argc;
         }
         // skip over word
         while(*bufp && !isspace(*bufp))
            ++bufp;
      }
      if(*bufp)
      {
         if(argv)
            *bufp = '\0';
         ++bufp;
      }

      // strip out \ from \" sequences
      if(argv && last_argc != argc)
         UnEscapeQuotes(argv[last_argc]);
   }
   if(argv)
      argv[argc] = nullptr;

   return argc;
}

//=============================================================================
//
// Utils
//

static void ShowError(const char *title, const char *message, HWND hWnd)
{
   MessageBoxA(hWnd, message, title, MB_ICONHAND|MB_OK);
}

static void ShowErrorForWndAndExit(const char *title, const char *message)
{
   MessageBoxA(mainWnd, message, title, MB_ICONHAND|MB_OK);
   PostMessageA(mainWnd, WM_CLOSE, 0, 0);
}

static BOOL OutOfMemory(void)
{
   ShowError("Fatal Error", "Out of memory");
   return FALSE;
}

//=============================================================================
//
// Toolbar
//

static TBBUTTON tbButtons[] =
{
   { MAKELONG(STD_FILESAVE,   0), ID_FILE_SAVEDOCUMENT,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, INT_PTR(L"Save document")  },
   { MAKELONG(STD_PRINT,      0), ID_FILE_PRINT,         0,               BTNS_BUTTON, {0}, 0, INT_PTR(L"Print")          },
   { MAKELONG(STD_FILEOPEN,   0), ID_FILE_INSERTPDF,     TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, INT_PTR(L"Insert PDF")     },
   { MAKELONG(STD_FILENEW,    0), ID_FILE_ACQUIRE,       TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, INT_PTR(L"Acquire")        },
   { MAKELONG(STD_PROPERTIES, 0), ID_FILE_SELECTSOURCE,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, INT_PTR(L"Select source")  },
   { MAKELONG(STD_DELETE,     0), ID_FILE_CLEARIMAGES,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, INT_PTR(L"Clear images")   },
   { MAKELONG(STD_UNDO,       0), ID_VIEW_PREVIOUSIMAGE, 0,               BTNS_BUTTON, {0}, 0, INT_PTR(L"Previous image") },
   { MAKELONG(STD_REDOW,      0), ID_VIEW_NEXTIMAGE,     0,               BTNS_BUTTON, {0}, 0, INT_PTR(L"Next image")     },
};

#define NUMTOOLBARBUTTONS (sizeof(tbButtons) / sizeof(TBBUTTON))

//
// Create a toolbar for the main application window
//
static HWND ScanMgr_CreateToolbar(HWND hParent)
{
   HWND hToolbar = 
      CreateWindowEx(0, TOOLBARCLASSNAME, nullptr, WS_CHILD|TBSTYLE_LIST|TBSTYLE_FLAT|TBSTYLE_TOOLTIPS, 
                     0, 0, 0, 0, hParent, nullptr, hInst, nullptr);
   if(!hToolbar)
   {
      ShowErrorForWndAndExit("Scan Manager", "Cannot create toolbar window");
      return nullptr;
   }

   HIMAGELIST hImgList = 
      ImageList_Create(16, 16, ILC_COLOR16|ILC_MASK, NUMTOOLBARBUTTONS, 0);

   if(!hImgList)
   {
      ShowErrorForWndAndExit("Scan Manager", "Cannot create toolbar image list");
      return nullptr;
   }

   // set extended styles
   SendMessage(hToolbar, TB_SETEXTENDEDSTYLE, 0, LPARAM(TBSTYLE_EX_MIXEDBUTTONS));

   // set image list
   SendMessage(hToolbar, TB_SETIMAGELIST, 0, LPARAM(hImgList));

   // load button images
   SendMessage(hToolbar, TB_LOADIMAGES, WPARAM(IDB_STD_SMALL_COLOR), LPARAM(HINST_COMMCTRL));

   // disable display of labels
   SendMessage(hToolbar, TB_SETMAXTEXTROWS, 0, 0);

   // add buttons
   SendMessage(hToolbar, TB_BUTTONSTRUCTSIZE, WPARAM(sizeof(TBBUTTON)), 0);
   SendMessage(hToolbar, TB_ADDBUTTONS, WPARAM(NUMTOOLBARBUTTONS), LPARAM(&tbButtons));

   // resize toolbar and show it - unnecessary now; the rebar will set it visible.
   //SendMessage(hToolbar, TB_AUTOSIZE, 0, 0);
   //ShowWindow(hToolbar, TRUE);

   // set icons for menu bar as well while we have the image list available
   HMENU hMainMenu = GetMenu(mainWnd);
   if(hMainMenu)
   {
      for(int i = 0; i < NUMTOOLBARBUTTONS; i++)
      {
         HICON hIcon = ImageList_GetIcon(hImgList, tbButtons[i].iBitmap, ILD_TRANSPARENT);
         if(hIcon)
         {
            HBITMAP hARGBBmp = gPARGB32Utils.bitmapFromIcon(hIcon);
            if(hARGBBmp)
            {
               MENUITEMINFO mii;
               mii.cbSize   = sizeof(mii);
               mii.fMask    = MIIM_BITMAP;
               mii.hbmpItem = hARGBBmp;
               SetMenuItemInfo(hMainMenu, tbButtons[i].idCommand, FALSE, &mii);
            }
         }
      }
   }

   return hToolbar;
}

//
// Create a "coolbar", aka rebar control, to serve as the background and container for the toolbar.
//
static HWND ScanMgr_CreateRebar()
{
   // init common controls
   INITCOMMONCONTROLSEX icex;
   icex.dwSize = sizeof(icex);
   icex.dwICC  = ICC_COOL_CLASSES|ICC_BAR_CLASSES;
   InitCommonControlsEx(&icex);

   // create the rebar
   HWND hRebar = 
      CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, nullptr, 
         WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|RBS_VARHEIGHT|CCS_NODIVIDER|RBS_BANDBORDERS,
         0, 0, 0, 0, mainWnd, nullptr, hInst, nullptr);

   if(!hRebar)
   {
      ShowErrorForWndAndExit("Scan Manager", "Cannot create rebar control");
      return nullptr;
   }

   // create the toolbar now.
   HWND hToolbar = toolbarWnd = ScanMgr_CreateToolbar(hRebar);
   if(!hToolbar)
      return nullptr;

   // create band info for toolbar band
   REBARBANDINFO rbBand;
   rbBand.cbSize = REBARBANDINFO_V3_SIZE;
   rbBand.fMask  = RBBIM_STYLE|RBBIM_TEXT|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE;
   rbBand.fStyle = RBBS_CHILDEDGE|RBBS_NOGRIPPER;

   SIZE sizeTbar; 
   SendMessage(hToolbar, TB_GETMAXSIZE, 0, LPARAM(&sizeTbar));

   rbBand.lpText     = TEXT("");
   rbBand.hwndChild  = hToolbar;
   rbBand.cyChild    = sizeTbar.cy;
   rbBand.cxMinChild = sizeTbar.cx;
   rbBand.cyMinChild = sizeTbar.cy;
   rbBand.cx         = 0;

   // insert the band
   SendMessage(hRebar, RB_INSERTBAND, WPARAM(-1), LPARAM(&rbBand));

   // minimize band size
   SendMessage(hRebar, RB_MINIMIZEBAND, 0, 0);

   return hRebar;
}

//
// Enable a single menu command and its corresponding toolbar button.
//
static void ScanMgr_EnableOneCmd(HMENU hMenu, UINT cmd)
{
   EnableMenuItem(hMenu, cmd, MF_ENABLED);
   SendMessage(toolbarWnd, TB_ENABLEBUTTON, cmd, TRUE);
}

//
// Disable a single menu command and its corresponding toolbar button.
//
static void ScanMgr_DisableOneCmd(HMENU hMenu, UINT cmd)
{
   EnableMenuItem(hMenu, cmd, MF_DISABLED|MF_GRAYED);
   SendMessage(toolbarWnd, TB_ENABLEBUTTON, cmd, FALSE);
}

//
// Disable toolbar and menu commands for saving or altering the document.
//
static void ScanMgr_DisableDocumentMutateCmds(bool lockCanEdit)
{
   HMENU hFileMenu = GetSubMenu(GetMenu(mainWnd), 0);
   ScanMgr_DisableOneCmd(hFileMenu, ID_FILE_SAVEDOCUMENT);
   ScanMgr_DisableOneCmd(hFileMenu, ID_FILE_INSERTPDF);
   ScanMgr_DisableOneCmd(hFileMenu, ID_FILE_ACQUIRE);
   ScanMgr_DisableOneCmd(hFileMenu, ID_FILE_CLEARIMAGES);

   if(lockCanEdit)
      canEdit = false;
}

//
// Re-enable toolbar and menu commands for saving or altering the document
//
static void ScanMgr_EnableDocumentMutateCmds()
{
   if(!canEdit)
      return;

   HMENU hFileMenu = GetSubMenu(GetMenu(mainWnd), 0);
   ScanMgr_EnableOneCmd(hFileMenu, ID_FILE_SAVEDOCUMENT);
   ScanMgr_EnableOneCmd(hFileMenu, ID_FILE_INSERTPDF);
   ScanMgr_EnableOneCmd(hFileMenu, ID_FILE_ACQUIRE);
   ScanMgr_EnableOneCmd(hFileMenu, ID_FILE_CLEARIMAGES);
}

//
// Disable Gdiplus image editing commands (flip, rotate, apply effects)
//
static void ScanMgr_DisableGDIPlusEditCmds()
{
   HMENU hEditMenu = GetSubMenu(GetMenu(mainWnd), 1);

   // flip commands
   ScanMgr_DisableOneCmd(hEditMenu, ID_EDIT_FLIPIMAGE);
   ScanMgr_DisableOneCmd(hEditMenu, ID_EDIT_FLIPIMAGELEFTTORIGHT);

   // rotate commands
   ScanMgr_DisableOneCmd(hEditMenu, ID_ROTATEIMAGE_90DEGREES);
   ScanMgr_DisableOneCmd(hEditMenu, ID_ROTATEIMAGE_180DEGREES);
   ScanMgr_DisableOneCmd(hEditMenu, ID_ROTATEIMAGE_270DEGREES);

   // effect commands
   ScanMgr_DisableOneCmd(hEditMenu, ID_APPLYEFFECT_ADJUSTBRIGHTNESSANDCONTRAST);
   ScanMgr_DisableOneCmd(hEditMenu, ID_APPLYEFFECT_ADJUSTCOLORBALANCE);
   ScanMgr_DisableOneCmd(hEditMenu, ID_APPLYEFFECT_ADJUSTHUE);
   ScanMgr_DisableOneCmd(hEditMenu, ID_APPLYEFFECT_ADJUSTTINT);
   ScanMgr_DisableOneCmd(hEditMenu, ID_APPLYEFFECT_SHARPENIMAGE);
}

//
// Enable Gdiplus image editing commands (flip, rotate, apply effects)
//
static void ScanMgr_EnableGDIPlusEditCmds()
{
   if(!gCurrentImage)
      return;

   HMENU hEditMenu = GetSubMenu(GetMenu(mainWnd), 1);

   // flip commands
   ScanMgr_EnableOneCmd(hEditMenu, ID_EDIT_FLIPIMAGE);
   ScanMgr_EnableOneCmd(hEditMenu, ID_EDIT_FLIPIMAGELEFTTORIGHT);

   // rotate commands
   ScanMgr_EnableOneCmd(hEditMenu, ID_ROTATEIMAGE_90DEGREES);
   ScanMgr_EnableOneCmd(hEditMenu, ID_ROTATEIMAGE_180DEGREES);
   ScanMgr_EnableOneCmd(hEditMenu, ID_ROTATEIMAGE_270DEGREES);

   // effect commands
   ScanMgr_EnableOneCmd(hEditMenu, ID_APPLYEFFECT_ADJUSTBRIGHTNESSANDCONTRAST);
   ScanMgr_EnableOneCmd(hEditMenu, ID_APPLYEFFECT_ADJUSTCOLORBALANCE);
   ScanMgr_EnableOneCmd(hEditMenu, ID_APPLYEFFECT_ADJUSTHUE);
   ScanMgr_EnableOneCmd(hEditMenu, ID_APPLYEFFECT_ADJUSTTINT);
   ScanMgr_EnableOneCmd(hEditMenu, ID_APPLYEFFECT_SHARPENIMAGE);
}

//=============================================================================
//
// Document Saving
//

//
// Saves the images to disk on the CHS file share and then writes a document 
// record tied to the created file path to the Prometheus database. If any
// failure is encountered along the process, the document files are deleted
// and the directory created (if any) will be removed.
//
static void ScanMgr_SaveDocument()
{
   if(!gImageList.head)
   {
      // must have scanned some images in first.
      MessageBox(mainWnd, L"You must scan one or more images first.", L"Scan Manager", MB_OK|MB_ICONINFORMATION);
   }
   else if(MessageBox(mainWnd, L"Are you sure you want to save the current images?", L"Scan Manager", MB_YESNO|MB_ICONQUESTION) == IDYES)
   {
      DocWriteStatus status;

      if(ScanMgr_WriteDocument(status, gImageList))
      {
         // Saved files successfully; try to write database record.
         if(ScanMgr_WriteDocumentRecord(status, theUser, personID, docTitle, docRecv, status.path, "Scanned Document"))
         {        
            // Fully successful; disable further saving and acquisition.
            modified = false;
            ScanMgr_DisableDocumentMutateCmds(true);
            MessageBox(mainWnd, L"Document was successfully saved.", L"Scan Manager", MB_OK|MB_ICONINFORMATION);
            return;
         }
      }

      // Save failed, cleanup and warn user.
      if(status.path.length())
         ScanMgr_RemoveFailedDocument(status.path);
      ShowError("Document Write Error", status.errorMsg.c_str(), mainWnd);
   }
}

//=============================================================================
//
// Print Functionality
// Use the Windows image printing wizard to print out the contents of the
// document.
//

static void ScanMgr_PrintDocument()
{
   if(!viewMode)
      return;

   std::set<std::string> filenames;
   if(!ScanMgr_GetDocumentImagePaths(viewPath, filenames))
      return;

   size_t numfilenames;
   if(!(numfilenames = filenames.size()))
      return;

   size_t idx = 0;
   std::unique_ptr<const char * []> upTmpPaths(new const char * [numfilenames]);
   const char **pTmpPaths = upTmpPaths.get();
   for(auto &fn : filenames)
      pTmpPaths[idx++] = fn.c_str();

   WiaAutomationProxy::CommonDialog::ShowPhotoPrintingWizard(pTmpPaths, (unsigned int)numfilenames);
}

//=============================================================================
//
// PDF Functionality
// At the eleventh hour, they said they need a way to organize and attach PDFs
// to patient records, so that's what this is here for.
//

#define CHS_FAX_INBOUND_DIR "\\\\la-nas1\\data\\Contract_Health\\Faxes\\Inbound"

//
// Use a File Open dialog to retrieve the name of a PDF file to transfer.
//
static bool ScanMgr_GetPDFFileName(std::string &outstr)
{
   OPENFILENAMEA opfn;
   memset(&opfn, 0, sizeof(opfn));
   char filename[_MAX_PATH+1] = { '\0' };

   opfn.lStructSize     = sizeof(OPENFILENAMEA);
   opfn.hwndOwner       = mainWnd;
   opfn.lpstrFilter     = "Adobe PDF Files\0*.pdf\0\0";
   opfn.lpstrFile       = filename;
   opfn.nMaxFile        = sizeof(filename);
   opfn.lpstrTitle      = "Select a PDF File";
   opfn.lpstrInitialDir = CHS_FAX_INBOUND_DIR;
   opfn.Flags           = OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;

   if(GetOpenFileNameA(&opfn))
   {
      outstr = filename;
      return true;
   }
   else
      return false;
}

//
// Copy a PDF to user's local machine.
//
static bool ScanMgr_CopyPDF(const std::string &src, std::string &dst)
{
   char localAppData[_MAX_PATH + 1];
   memset(localAppData, 0, sizeof(localAppData));
   SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, localAppData);

   std::string fn = FileCache::GetFileSpec(src).second;
   FileCache::SetBasePath(localAppData);
   if(FileCache::CopyIntoCache(src, "Temp\\ScanManager"))
   {
      dst = FileCache::PathConcatenate(FileCache::PathConcatenate(localAppData, "Temp\\ScanManager"), fn);
      return true;
   }
   else
      return false;
}

//
// Prompts user to select a PDF file to copy to the CHS file share and then writes
// a document record tied to the created file path to the Prometheus database. 
// If any failure is encountered along the process, the document files are deleted
// and the directory created (if any) will be removed.
//
static void ScanMgr_SavePDFDocument()
{
   std::string filename;

   // If images have been scanned, warn. If user accepts, delete the images as they will
   // not be saved.
   if(gImageList.head)
   {
      if(MessageBox(mainWnd, L"Scanned images will be discarded. Are you sure?", L"Scan Manager", MB_YESNO|MB_ICONQUESTION) == IDYES)
         ScanMgr_ClearImageList();
      else
         return;
   }

   // get file name
   if(ScanMgr_GetPDFFileName(filename))
   {
      DocWriteStatus status;

      if(ScanMgr_WritePDFDocument(status, filename))
      {
         // Saved file successfully; try to write database record.
         if(ScanMgr_WriteDocumentRecord(status, theUser, personID, docTitle, docRecv, status.path, "Adobe PDF"))
         {        
            // Fully successful; disable further saving and acquisition.
            std::string loadPath, tmpFile;
            modified = false;
            ScanMgr_DisableDocumentMutateCmds(true);
            MessageBox(mainWnd, L"Document was successfully saved.", L"Scan Manager", MB_OK|MB_ICONINFORMATION);

            // display the file that was just saved
            if(ScanMgr_ReadPDFDocumentFromPath(status.path, loadPath))
            {
               if(ScanMgr_CopyPDF(loadPath, tmpFile))
                  ShellExecuteA(mainWnd, "open", tmpFile.c_str(), nullptr, nullptr, SW_SHOW);
            }
            return;
         }
      }

      // Save failed, cleanup and warn user.
      if(status.path.length())
         ScanMgr_RemoveFailedDocument(status.path);
      ShowError("Document Write Error", status.errorMsg.c_str(), mainWnd);
   }
}

//=============================================================================
//
// Document Viewing
//

//
// Setup the program to view the images of a previously saved document.
//
static void ScanMgr_SetupViewMode()
{
   ScanMgr_DisableDocumentMutateCmds(true);
   ScanMgr_DisableGDIPlusEditCmds();

   if(isPDF)
   {
      std::string pdfPath, tmpPath;
      if(!ScanMgr_ReadPDFDocumentFromPath(viewPath, pdfPath))
         ShowError("Document Read Error", "PDF file is missing or cannot be read.", mainWnd);
      else
      {
         if(!ScanMgr_CopyPDF(pdfPath, tmpPath))
            ShowError("Document Read Error", "Cannot copy PDF to local system for viewing.", mainWnd);
         else
            ShellExecuteA(mainWnd, "open", tmpPath.c_str(), nullptr, nullptr, SW_SHOW);
      }
   }
   else
   {
      if(!ScanMgr_ReadDocumentFromPath(viewPath, gImageList))
         ShowError("Document Read Error", "One or more document images could not be loaded.", mainWnd);
      else
         ScanMgr_SetupViewImages();
   }
}

//=============================================================================
//
// WinMain
//

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
   UNREFERENCED_PARAMETER(hPrevInstance);
   UNREFERENCED_PARAMETER(lpCmdLine);

   // Get commandline
   CHAR *text = GetCommandLineA();
   char *cmdline = strdup(text);

   if(cmdline == nullptr)
      return OutOfMemory();

   // parse into argv and argc
   argc = ParseCommandLine(cmdline, nullptr);
   argv = static_cast<char **>(calloc(argc + 1, sizeof(char *)));
   if(argv == nullptr)
      return OutOfMemory();
   ParseCommandLine(cmdline, argv);

   // Initialize global strings
   LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
   LoadStringW(hInstance, IDC_SCANMANAGER, szWindowClass, MAX_LOADSTRING);
   MyRegisterClass(hInstance);

   // Perform application initialization:
   if(!InitInstance(hInstance, nCmdShow))
   {
      return FALSE;
   }

   // initialize GDI+ library
   if(!ScanMgr_InitGDIPlus())
      ShowErrorForWndAndExit("Scan Manager", "Could not initialize GDI+ graphics.");

   // create toolbar window
   rebarWnd = ScanMgr_CreateRebar();

   // try to initialize TWAIN
   if(twainMgr.loadSourceManager())
   {
      if(!twainMgr.openSourceManager(mainWnd))
         ShowErrorForWndAndExit("Scan Manager", "Could not open TWAIN source manager.");
   }
   else
      ShowErrorForWndAndExit("Scan Manager", "Could not load TWAIN source manager.");

   // connect to Prometheus database
   if(!theUser.connect())
   {
      ScanMgr_DisableDocumentMutateCmds(true);
      ShowErrorForWndAndExit("Scan Manager", "Could not connect to Prometheus.");
   }
   else if(!theUser.checkCredentials()) // check user credentials
   {      
      ScanMgr_DisableDocumentMutateCmds(true);
      ShowErrorForWndAndExit("Scan Manager", "Incorrect user ID or password, access denied.");
   }
   else
   {
      // check for document view mode
      int p;
      if((p = M_GetArgParameter("-view", 1)))
      {
         modified = false;
         viewMode = true;
         viewPath = argv[p];
         if((p = M_FindArgument("-pdf")))
            isPDF = true;
         ScanMgr_SetupViewMode();
      }
      else
      {
         // check for required document parameters

         // person ID
         if(!(p = M_GetArgParameter("-person", 1)))
            ShowErrorForWndAndExit("Scan Manager", "Missing person ID for which to create document.");
         else
            personID = argv[p];

         // document title
         if(!(p = M_GetArgParameter("-title", 1)))
            ShowErrorForWndAndExit("Scan Manager", "Missing title for document.");
         else
            docTitle = argv[p];

         // document received date
         if(!(p = M_GetArgParameter("-date", 1)))
            ShowErrorForWndAndExit("Scan Manager", "Missing date for document.");
         else
            docRecv = argv[p];

         ScanMgr_DisableGDIPlusEditCmds(); // disable GDI edit commands until an image is scanned
         modified = true;
      }
   }

   HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SCANMANAGER));

   MSG msg;

   // Main message loop:
   while(GetMessage(&msg, nullptr, 0, 0))
   {
      // check for modeless effect dialog message
      if(pEffectDlg)
      {
         HWND hDlg = pEffectDlg->getDialogHWND();
         if(IsWindow(hDlg))
         {
            if(IsDialogMessage(hDlg, &msg))
               continue;
         }
         else
         {
            delete pEffectDlg;    // done with dialog object
            pEffectDlg = nullptr;
            ScanMgr_EnableDocumentMutateCmds();
            ScanMgr_EnableGDIPlusEditCmds();
            ScanMgr_UpdateViewImgCmds();
         }
      }

      // allow TWAIN to check if it handles this event
      if(twainMgr.TWAINCheckEvent(msg, ScanMgr_AddNewImage))
         continue;

      if(!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }

   free(argv);
   free(cmdline);

   return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
   WNDCLASSEXW wcex;

   wcex.cbSize = sizeof(WNDCLASSEX);

   wcex.style         = CS_HREDRAW | CS_VREDRAW;
   wcex.lpfnWndProc   = WndProc;
   wcex.cbClsExtra    = 0;
   wcex.cbWndExtra    = 0;
   wcex.hInstance     = hInstance;
   wcex.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SCANMANAGER));
   wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
   wcex.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
   wcex.lpszMenuName  = MAKEINTRESOURCEW(IDC_SCANMANAGER);
   wcex.lpszClassName = szWindowClass;
   wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

   return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, 
                             hInstance, nullptr);

   if(!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   mainWnd = hWnd;

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
   case WM_COMMAND:
      {
         int wmId = LOWORD(wParam);
         // Parse the menu selections:
         switch (wmId)
         {
         case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
         case IDM_EXIT:
            if(modified && !viewMode)
            {
               if(MessageBox(hWnd, L"Changes to the document have not been saved.\nAre you sure you want to exit?", L"Scan Manager", MB_YESNO|MB_ICONQUESTION) == IDNO)
                  break;
            }
            DestroyWindow(hWnd);
            break;
         case ID_FILE_SAVEDOCUMENT:
            // save images
            ScanMgr_SaveDocument();
            break;
         case ID_FILE_PRINT:
            ScanMgr_PrintDocument();
            break;
         case ID_FILE_INSERTPDF:
            ScanMgr_SavePDFDocument();
            break;
         case ID_FILE_ACQUIRE:
            // acquire TWAIN images
            twainMgr.doAcquire(mainWnd);
            break;
         case ID_FILE_SELECTSOURCE:
            // select TWAIN source
            if(!twainMgr.selectSource())
               ShowError("Scan Manager", "Could not select this scanner; check the device.", mainWnd);
            break;
         case ID_FILE_CLEARIMAGES:
            // clear scanned-in images
            ScanMgr_ClearImageList();
            break;
         case ID_EDIT_FLIPIMAGE:
            // flip image top-to-bottom
            if(!ScanMgr_FlipAndRotate(Gdiplus::RotateNoneFlipY))
               ShowError("Scan Manager", "Operation failed", mainWnd);
            break;
         case ID_EDIT_FLIPIMAGELEFTTORIGHT:
            // flip image left-to-right
            if(!ScanMgr_FlipAndRotate(Gdiplus::RotateNoneFlipX))
               ShowError("Scan Manager", "Operation failed", mainWnd);
            break;
         case ID_ROTATEIMAGE_90DEGREES:
            // rotate image 90 degrees
            if(!ScanMgr_FlipAndRotate(Gdiplus::Rotate90FlipNone))
               ShowError("Scan Manager", "Operation failed", mainWnd);
            break;
         case ID_ROTATEIMAGE_180DEGREES:
            // rotate image 180 degrees
            if(!ScanMgr_FlipAndRotate(Gdiplus::Rotate180FlipNone))
               ShowError("Scan Manager", "Operation failed", mainWnd);
            break;
         case ID_ROTATEIMAGE_270DEGREES:
            // rotate image 270 degrees
            if(!ScanMgr_FlipAndRotate(Gdiplus::Rotate270FlipNone))
               ShowError("Scan Manager", "Operation failed", mainWnd);
            break;
         case ID_APPLYEFFECT_ADJUSTBRIGHTNESSANDCONTRAST:
            ScanMgr_SpawnEffectDialog(FXTYPE_BRIGHTNESS);
            break;
         case ID_APPLYEFFECT_ADJUSTCOLORBALANCE:
            ScanMgr_SpawnEffectDialog(FXTYPE_BALANCE);
            break;
         case ID_APPLYEFFECT_ADJUSTHUE:
            ScanMgr_SpawnEffectDialog(FXTYPE_HSL);
            break;
         case ID_APPLYEFFECT_ADJUSTTINT:
            ScanMgr_SpawnEffectDialog(FXTYPE_TINT);
            break;
         case ID_APPLYEFFECT_SHARPENIMAGE:
            ScanMgr_SpawnEffectDialog(FXTYPE_SHARPEN);
            break;
         case ID_VIEW_NEXTIMAGE:
            // view next image on scanned images list
            ScanMgr_GotoNextImage();
            break;
         case ID_VIEW_PREVIOUSIMAGE:
            // view previous image on scanned images list
            ScanMgr_GotoPrevImage();
            break;
         default:
            return DefWindowProc(hWnd, message, wParam, lParam);
         }
      }
      break;
   case WM_SYSCOMMAND:
      if(wParam == SC_CLOSE && modified && !viewMode)
      {
         if(MessageBox(hWnd, L"Changes to the document have not been saved.\nAre you sure you want to exit?", L"Scan Manager", MB_YESNO|MB_ICONQUESTION) == IDNO)
            break;
      }
      return DefWindowProc(hWnd, message, wParam, lParam);
   case WM_KEYDOWN:
      switch(wParam)
      {
      case VK_LEFT:
         ScanMgr_GotoPrevImage();
         break;
      case VK_RIGHT:
         ScanMgr_GotoNextImage();
         break;
      default:
         return DefWindowProc(hWnd, message, wParam, lParam);
      }
      break;
   case WM_PAINT:
      {
         PAINTSTRUCT ps;
         HDC hdc = BeginPaint(hWnd, &ps);
         OnPaint(hdc);
         EndPaint(hWnd, &ps);
      }
      break;
   case WM_DESTROY:
      ScanMgr_CloseShare();
      ScanMgr_ShutdownImages();
      ScanMgr_ShutdownGDIPlus();
      twainMgr.shutdown(mainWnd);
      PostQuitMessage(0);
      break;
   default:
      return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
   UNREFERENCED_PARAMETER(lParam);
   switch(message)
   {
   case WM_INITDIALOG:
      return (INT_PTR)TRUE;

   case WM_COMMAND:
      if(LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
      {
         EndDialog(hDlg, LOWORD(wParam));
         return (INT_PTR)TRUE;
      }
      break;
   }
   return (INT_PTR)FALSE;
}

// EOF

