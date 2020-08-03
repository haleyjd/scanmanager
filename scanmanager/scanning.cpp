/*
  Scan Manager

  TWAIN scanning module for image acquisition
*/

#include <Windows.h>
#include <string>
#include "twain.h"
#include "cached_files.h"
#include "scanning.h"

static DSMENTRYPROC pDSM_Entry; // DSM_Entry TWAIN entry point routine loaded from DLL
static HMODULE      hDSMLib;    // handle to DSM library

// TWAIN state variables
static bool twain_loaded;          // DSM loaded? (in state 2 or higher)
static bool twain_opened;          // DSM opened? (in state 3 or higher)
static bool twain_source_opened;   // DS opened?  (in state 4 or higher)
static bool twain_caps_negotiated; // Capabilities negotiated? (during state 4)
static bool twain_source_enabled;  // DS enabled? (in state 5 or higher)
static bool twain_transfer_ready;  // xfer ready? (in state 6 or higher)

// identity structures
static TW_IDENTITY AppID; // application ID structure; run-time constant once initialized
static TW_IDENTITY SrcID; // source ID structure of currently selected source (not necessarily open)
static TW_IDENTITY SelID; // source ID structure of currently opened source (not necessarily selected)

static TW_INT16 negotiatedImageCount;

//
// Bring the application into state 2 by loading the TWAIN source manager
//
bool TWAINManager::loadSourceManager()
{
   if(!twain_loaded)
   {
      std::string path;
      char winPath[MAX_PATH];
      ZeroMemory(winPath, sizeof(winPath));

      GetWindowsDirectoryA(winPath, sizeof(winPath));
      path = winPath;
      path = FileCache::PathConcatenate(path, "TWAIN_32.DLL");

      if((hDSMLib = LoadLibraryA(path.c_str())))
      {
         if((pDSM_Entry = DSMENTRYPROC(GetProcAddress(hDSMLib, MAKEINTRESOURCEA(1)))))
            twain_loaded = true;
      }
   }

   return twain_loaded;
}

//
// Bring the application into state 3 by opening the TWAIN source manager
//
bool TWAINManager::openSourceManager(HWND hWnd)
{
   if(!twain_loaded) // TWAIN must be loaded first
      return false;

   if(!twain_opened)
   {
      ZeroMemory(&AppID, sizeof(AppID));

      // initialize the AppID object
      AppID.Id = 0;
      AppID.Version.MajorNum = 1;
      AppID.Version.MinorNum = 0;
      AppID.Version.Language = TWLG_ENGLISH_USA;
      AppID.Version.Country  = TWCY_USA;
      strcpy(AppID.Version.Info, "v1.0");
      AppID.ProtocolMajor    = TWON_PROTOCOLMAJOR;
      AppID.ProtocolMinor    = TWON_PROTOCOLMINOR;
      AppID.SupportedGroups  = (DG_IMAGE|DG_CONTROL);
      strcpy(AppID.Manufacturer,  "Absentee Shawnee Tribal Health");
      strcpy(AppID.ProductFamily, "Health IT Apps");
      strcpy(AppID.ProductName,   "Scan Manager");

      if((*pDSM_Entry)(&AppID, nullptr, DG_CONTROL, DAT_PARENT, MSG_OPENDSM, TW_MEMREF(&hWnd)) == TWRC_SUCCESS)
         twain_opened = true;
   }

   return twain_opened;
}

//
// Call to do TWAIN source selection.
//
bool TWAINManager::selectSource()
{
   if(!twain_opened)
      return false; // must be in state 3 at least.

   TW_UINT16 rc;
   TW_IDENTITY tempSrcID;
   ZeroMemory(&tempSrcID, sizeof(tempSrcID));

   rc = (*pDSM_Entry)(&AppID, nullptr, DG_CONTROL, DAT_IDENTITY, MSG_USERSELECT, TW_MEMREF(&tempSrcID));
   switch(rc)
   {
   case TWRC_SUCCESS:
      // copy tempSrcID to SrcID
      SrcID = tempSrcID;
      return true;
   case TWRC_CANCEL:
      // do nothing
      return true;
   default:
      // error
      return false;
   }
}

//
// Open TWAIN image source, protected method.
//
bool TWAINManager::openSource()
{
   TW_UINT16 rc;

   if(twain_source_opened) // in state 4?
   {
      if(twain_source_enabled) // in state 5?
      {
         // try to disable source (state 5 -> 4)
         if(!disableSource()) // still in state 5? abort pending xfers
         {
            doAbortXfer();
            if(!disableSource())
               return false;
         }
      }

      // close TWAIN data source (state 4 -> 3)
      if(!closeSource())
         return false;
   }

   rc = (*pDSM_Entry)(&AppID, nullptr, DG_CONTROL, DAT_IDENTITY, MSG_OPENDS, TW_MEMREF(&SrcID));
   if(rc == TWRC_SUCCESS)
   {
      // The source is now open. Copy SrcID to SelID to continue using this source identity information
      // as long as the application remains in state 4 or higher, even if the user selects a different
      // device during that period.
      SelID = SrcID;
      twain_source_opened = true;
      twain_caps_negotiated = false; // capabilities have not been negotiated yet
      negotiatedImageCount = 0;
      return true;
   }
   else
      return false;
}

//
// Close any opened TWAIN source. Protected method.
//
bool TWAINManager::closeSource()
{
   if(!twain_source_opened)
      return true; // fine, no source is open.

   TW_UINT16 rc = (*pDSM_Entry)(&AppID, nullptr, DG_CONTROL, DAT_IDENTITY, MSG_CLOSEDS, TW_MEMREF(&SelID));
   if(rc == TWRC_SUCCESS)
   {
      twain_source_opened = false; // now back in state 3
      ZeroMemory(&SelID, sizeof(SelID));
      return true;
   }
   else
      return false;
}

//
// If device cannot set CAP_XFERCOUNT capability exactly, it will attempt to negotiate an
// acceptable value. Protected subroutine for negotiateCaps.
//
bool TWAINManager::checkCapability(HWND hWnd, short &count)
{
   TW_CAPABILITY twCapability;
   TW_UINT16     rc;
   pTW_ONEVALUE  pval;

   twCapability.Cap        = CAP_XFERCOUNT;
   twCapability.ConType    = TWON_DONTCARE16; // source will specify
   twCapability.hContainer = nullptr;         // source allocates and fills container

   rc = (*pDSM_Entry)(&AppID, &SelID, DG_CONTROL, DAT_CAPABILITY, MSG_GET, TW_MEMREF(&twCapability));
   if(twCapability.hContainer)
   {
      bool ret;
      pval = pTW_ONEVALUE(GlobalLock(twCapability.hContainer));
      if(pval)
      {
         count = short(pval->Item);
         ret   = true;
      }
      else
      {
         MessageBoxA(hWnd, "Could not lock source value, check scanner.", "Scan Manager", MB_OK);
         ret = false;
      }
      GlobalFree(HGLOBAL(twCapability.hContainer));
      return ret;
   }
   else
   {
      MessageBoxA(hWnd, "Bad scanner, returned null capability container.", "Scan Manager", MB_OK);
      return false;
   }
}

//
// Negotiate capabilities with opened TWAIN source; protected method.
//
bool TWAINManager::negotiateCaps(HWND hWnd)
{
   if(!twain_source_opened) // source must be open
      return false;

   TW_CAPABILITY twCapability;
   TW_INT16      count;
   TW_STATUS     twStatus;
   TW_UINT16     rc;
   pTW_ONEVALUE  pval;
   bool          retval = false;

   // setup MSG_GET for CAP_XFERCOUNT
   twCapability.Cap        = CAP_XFERCOUNT;
   twCapability.ConType    = TWON_ONEVALUE;
   twCapability.hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE));

   if(twCapability.hContainer)
   {
      pval = pTW_ONEVALUE(GlobalLock(twCapability.hContainer));
      if(pval)
      {
         pval->ItemType = TWTY_INT16;
         pval->Item     = -1;
         GlobalUnlock(twCapability.hContainer);

         rc = (*pDSM_Entry)(&AppID, &SelID, DG_CONTROL, DAT_CAPABILITY, MSG_SET, TW_MEMREF(&twCapability));

         if(rc == TWRC_SUCCESS)
         {
            // done, image count set at -1 (accept any number of images)
            twain_caps_negotiated = true;
            negotiatedImageCount  = -1;
            retval = true;
         }
         else if(rc == TWRC_CHECKSTATUS)
         {
            // source wants to negotiate
            if((retval = checkCapability(hWnd, count)))
            {
               twain_caps_negotiated = true;
               negotiatedImageCount  = count;
            }
         }
         else if(rc == TWRC_FAILURE)
         {
            // error
            rc = (*pDSM_Entry)(&AppID, &SelID, DG_CONTROL, DAT_STATUS, MSG_GET, TW_MEMREF(&twStatus));
            switch(twStatus.ConditionCode)
            {
            case TWCC_BADCAP:
            case TWCC_CAPUNSUPPORTED:
            case TWCC_CAPBADOPERATION:
            case TWCC_CAPSEQERROR:
               MessageBoxA(hWnd, "Scanner error. Cannot set CAP_XFERCOUNT.", "Scan Manager", MB_OK);
               break;
            case TWCC_BADDEST:
               MessageBoxA(hWnd, "Source not open, check scanner connection.", "Scan Manager", MB_OK);
               break;
            case TWCC_BADVALUE:
               MessageBoxA(hWnd, "Source problem. CAP_XFERCOUNT invalid.", "Scan Manager", MB_OK);
               break;
            case TWCC_SEQERROR:
               MessageBoxA(hWnd, "Programming error, contact Health IT.", "Scan Manager", MB_OK);
               break;
            }
            retval = false;
         }
      }
      else
      {
         MessageBoxA(hWnd, "Devcaps negotiation failed in GlobalLock.", "Scan Manager", MB_OK);
         retval = false;
      }

      GlobalFree(HGLOBAL(twCapability.hContainer));
   }
   else
   {
      MessageBoxA(hWnd, "Devcaps negotiation failed in GlobalAlloc, out of memory.", "Scan Manager", MB_OK);
      retval = false;
   }

   return retval;
}

//
// Part 3 of image acquisition process; puts application into state 5. Protected method.
//
bool TWAINManager::enableSource(HWND hWnd)
{
   if(!twain_source_opened)
      return false; // must be in state 4

   TW_USERINTERFACE twInterface;
   TW_UINT16 rc;

   twInterface.ShowUI  = TRUE;
   twInterface.ModalUI = FALSE;
   twInterface.hParent = hWnd;

   rc = (*pDSM_Entry)(&AppID, &SelID, DG_CONTROL, DAT_USERINTERFACE, MSG_ENABLEDS, TW_MEMREF(&twInterface));
   if(rc == TWRC_SUCCESS)
      twain_source_enabled = true; // now in state 5; message forwarding will begin immediately
   else
      MessageBoxA(hWnd, "Failed to enable source, check scanner.", "Scan Manager", MB_OK);

   return twain_source_enabled;
}

//
// Perform image acquisition
//
bool TWAINManager::doAcquire(HWND hWnd)
{
   if(!twain_opened)
      return false; // TWAIN must be open

   if(twain_source_opened)
      return false; // a source must NOT already be open

   if(!openSource())
      return false;

   // now in state 4, need capabilities.
   if(!negotiateCaps(hWnd))
      return false; // negotiation failed

   bool res = enableSource(hWnd);
   if(!res)
      closeSource(); // if not in state 5, drop back to state 3

   return res;
}

//
// Disable an enabled source; transition from state 5 back to state 4.
//
bool TWAINManager::disableSource()
{
   if(!twain_source_enabled)
      return true; // fine, source wasn't enabled.

   TW_UINT16 rc;
   TW_USERINTERFACE foo; // not used
   ZeroMemory(&foo, sizeof(foo));

   rc = (*pDSM_Entry)(&AppID, &SelID, DG_CONTROL, DAT_USERINTERFACE, MSG_DISABLEDS, TW_MEMREF(&foo));

   if(rc == TWRC_SUCCESS)
      twain_source_enabled = false;

   return !twain_source_enabled; // successful if source is not enabled now.
}

//
// Subroutine for setupAndXferImage; gets an image info structure
//
bool TWAINManager::updateImageInfo(void *pvImgInfo)
{
   pTW_IMAGEINFO imgInfo = pTW_IMAGEINFO(pvImgInfo);
   TW_UINT16 rc = (*pDSM_Entry)(&AppID, &SelID, DG_IMAGE, DAT_IMAGEINFO, MSG_GET, TW_MEMREF(imgInfo));
   return rc == TWRC_SUCCESS;   
}

//
// Subroutine for setupAndXferImage for error handling.
//
void TWAINManager::doAbortXfer()
{
   TW_PENDINGXFERS pxfers;
   ZeroMemory(&pxfers, sizeof(pxfers));

   // try to end the current transfer
   (*pDSM_Entry)(&AppID, &SelID, DG_CONTROL, DAT_PENDINGXFERS, MSG_ENDXFER, TW_MEMREF(&pxfers));
   if(pxfers.Count != 0)
   {
      // abort remaining transfers
      ZeroMemory(&pxfers, sizeof(pxfers));
      (*pDSM_Entry)(&AppID, &SelID, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, TW_MEMREF(&pxfers));
   }
}

//
// Part 4 of image acquisition process. This is called when the source sends an event
// back to the application via the DSM message loop handler to indicate that it is
// prepared to send data. It will wait for acknowledgement; this routine places the
// application into state 7, but in a transitory manner.
//
void TWAINManager::setupAndXferImage(void (*cb)(HBITMAP))
{
   TW_UINT16 rc;
   bool havePendingXfers = true;

   if(twain_transfer_ready)
      return; // ????

   // we are now in state 6
   twain_transfer_ready = true;

   while(havePendingXfers)
   {
      TW_IMAGEINFO currInfo;
      HBITMAP      currImage = nullptr;

      if(updateImageInfo(&currInfo))
      {
         // get image handle
         rc = (*pDSM_Entry)(&AppID, &SelID, DG_IMAGE, DAT_IMAGENATIVEXFER, MSG_GET, TW_MEMREF(&currImage));
         if(rc == TWRC_XFERDONE)
         {
            TW_PENDINGXFERS pxfers;
            ZeroMemory(&pxfers, sizeof(pxfers));

            // write out/save images here using callback
            cb(currImage);

            rc = (*pDSM_Entry)(&AppID, &SelID, DG_CONTROL, DAT_PENDINGXFERS, MSG_ENDXFER, TW_MEMREF(&pxfers));
            if(rc == TWRC_SUCCESS)
            {
               if(pxfers.Count == 0)
                  havePendingXfers = false; // done, end loop
            }
            else
               havePendingXfers = false; // error
         }
         else if(rc == TWRC_CANCEL)
            break; // error
         else if(rc == TWRC_FAILURE)
            break; // error
      }
      else
         break; // error
   }

   if(havePendingXfers) // left the loop due to an error?
      doAbortXfer();

   // return to state 5
   twain_transfer_ready = false;
}

//
// Call from program's main event loop. TWAIN will check if it needs to intercept
// the Windows message. If not (function returns false), process it normally.
//
bool TWAINManager::TWAINCheckEvent(MSG &msg, void (*cb)(HBITMAP))
{
   if(!twain_source_enabled)
      return false; // a source is not in the enabled state, no message processing.

   TW_EVENT twEvent;
   TW_INT16 rc = TWRC_NOTDSEVENT;

   twEvent.pEvent    = TW_MEMREF(&msg);
   twEvent.TWMessage = MSG_NULL;

   rc = (*pDSM_Entry)(&AppID, &SelID, DG_CONTROL, DAT_EVENT, MSG_PROCESSEVENT, TW_MEMREF(&twEvent));

   switch(twEvent.TWMessage)
   {
   case MSG_XFERREADY: // transfer is ready
      setupAndXferImage(cb); // now in state 6 courtesy of source; acknowledge and move to state 7 for acquisition
      break;
   case MSG_CLOSEDSREQ: // request to close source
   case MSG_CLOSEDSOK:  // source closed (?)
      if(disableSource())
         closeSource();
      break;
   case MSG_NULL: // nothing
   default:
      break;
   }

   return !(rc == TWRC_NOTDSEVENT);
}

//
// Shutdown TWAIN
//
void TWAINManager::shutdown(HWND hWnd)
{
   if(twain_loaded) // in state 2?
   {
      if(twain_opened) // in state 3?
      {
         if(twain_source_opened) // in state 4?
         {
            if(twain_source_enabled) // in state 5?
            {
               // try to disable source (state 5 -> 4)
               if(!disableSource()) // still in state 5? abort pending xfers
               {
                  doAbortXfer();
                  disableSource();
               }
            }

            // close TWAIN data source (state 4 -> 3)
            closeSource();
         }

         // close TWAIN DSM (state 3 -> 2)
         (*pDSM_Entry)(&AppID, nullptr, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM, TW_MEMREF(&hWnd));
         twain_opened = false;
      }

      // shut down TWAIN library (state 2 -> 1)
      pDSM_Entry   = nullptr;
      twain_loaded = false;
   }
}

// EOF

