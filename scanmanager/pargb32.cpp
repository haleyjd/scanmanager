/*
  Scan Manager
 
  PARGB32 Bitmap utilities
*/

#include <Windows.h>
#include <Uxtheme.h>
#include <gdiplus.h>
#include "pargb32.h"

//=============================================================================
//
// UXTHEME Dependencies
//

typedef HRESULT      (WINAPI *PFNGetBufferedPaintBits)(HPAINTBUFFER hBufferedPaint, RGBQUAD **ppbBuffer, int *pcxRow);
typedef HPAINTBUFFER (WINAPI *PFNBeginBufferedPaint  )(HDC hdcTarget, const RECT *prcTarget, BP_BUFFERFORMAT dwFormat, BP_PAINTPARAMS *pPaintParams, HDC *phdc);
typedef HRESULT      (WINAPI *PFNEndBufferedPaint    )(HPAINTBUFFER hBufferedPaint, BOOL fUpdateTarget);

static PFNGetBufferedPaintBits pGetBufferedPaintBits;
static PFNBeginBufferedPaint   pBeginBufferedPaint;
static PFNEndBufferedPaint     pEndBufferedPaint;

//=============================================================================
//
// Internal utilities to convert colors
//

//
// Internal utility to test for alpha channel in ARGB data
//
static bool HasAlpha(Gdiplus::ARGB *pargb, const SIZE &imgSize, int cxRow)
{
   ULONG cxDelta = cxRow - imgSize.cx;

   for(ULONG y = imgSize.cy; y; --y)
   {
      for(ULONG x = imgSize.cx; x; --x)
      {
         if(*pargb++ & 0xFF000000)
            return true;
      }

      pargb += cxDelta;
   }

   return false;
}

//
// Internal utility to convert Gdiplus ARGB buffer to HBITMAP
//
HRESULT ConvertToPARGB32(HDC hdc, Gdiplus::ARGB *pargb, HBITMAP hbmp, const SIZE &imgSize, int cxRow)
{
   BITMAPINFO bmi;
   memset(&bmi, 0, sizeof(bmi));
   bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
   bmi.bmiHeader.biPlanes      = 1;
   bmi.bmiHeader.biCompression = BI_RGB;
   bmi.bmiHeader.biWidth       = imgSize.cx;
   bmi.bmiHeader.biHeight      = imgSize.cy;
   bmi.bmiHeader.biBitCount    = 32;

   HANDLE hHeap = GetProcessHeap();
   void *bits = HeapAlloc(hHeap, 0, bmi.bmiHeader.biWidth * 4 * bmi.bmiHeader.biHeight);
   if(!bits)
      return E_OUTOFMEMORY;

   HRESULT hr = E_UNEXPECTED;
   if(GetDIBits(hdc, hbmp, 0, bmi.bmiHeader.biHeight, bits, &bmi, DIB_RGB_COLORS) == bmi.bmiHeader.biHeight)
   {
      ULONG cxDelta = cxRow - bmi.bmiHeader.biWidth;
      Gdiplus::ARGB *mask = static_cast<Gdiplus::ARGB *>(bits);

      for(ULONG y = bmi.bmiHeader.biHeight; y; y--)
      {
         for(ULONG x = bmi.bmiHeader.biWidth; x; x--)
         {
            if(*mask++)
            {
               // transparent pixel
               *pargb++ = 0;
            }
            else
            {
               // opaque pixel
               *pargb++ |= 0xFF000000;
            }
         }

         pargb += cxDelta;
      }

      hr = S_OK;
   }

   HeapFree(hHeap, 0, bits);

   return hr;
}

//
// Internal utility to convert non-alpha icons
//
static HRESULT ConvertBufferToPARGB32(HPAINTBUFFER hPaintBuffer, HDC hdc, HICON hIcon, const SIZE &iconSize)
{
   RGBQUAD *pQuad;
   int      cxRow;
   HRESULT  hr = pGetBufferedPaintBits(hPaintBuffer, &pQuad, &cxRow);
   
   if(SUCCEEDED(hr))
   {
      auto pargb = reinterpret_cast<Gdiplus::ARGB *>(pQuad);
      if(!HasAlpha(pargb, iconSize, cxRow))
      {
         ICONINFO info;
         if(GetIconInfo(hIcon, &info))
         {
            if(info.hbmMask)
               hr = ConvertToPARGB32(hdc, pargb, info.hbmMask, iconSize, cxRow);

            DeleteObject(info.hbmColor);
            DeleteObject(info.hbmMask);
         }
      }
   }

   return hr;
}

//=============================================================================
//
// PARGB32Utils methods
//

//
// Constructor
//
PARGB32Utils::PARGB32Utils() : images()
{
   // load the UXTHEME library
   HMODULE hUxLib = LoadLibrary(L"UXTHEME.DLL");

   if(hUxLib)
   {
      pGetBufferedPaintBits = reinterpret_cast<PFNGetBufferedPaintBits>(GetProcAddress(hUxLib, "GetBufferedPaintBits"));
      pBeginBufferedPaint   = reinterpret_cast<PFNBeginBufferedPaint  >(GetProcAddress(hUxLib, "BeginBufferedPaint"  ));
      pEndBufferedPaint     = reinterpret_cast<PFNEndBufferedPaint    >(GetProcAddress(hUxLib, "EndBufferedPaint"    ));
   }
}

//
// Destructor
//
PARGB32Utils::~PARGB32Utils()
{
   while(images.head)
      delete images.head->dllObject;
}

//
// Create a 32-bit HBITMAP
//
bool PARGB32Utils::create32BitBitmap(HDC hdc, const SIZE &size, void **bits, HBITMAP *phBmp)
{
   *phBmp = nullptr;

   BITMAPINFO bmi;
   memset(&bmi, 0, sizeof(bmi));
   bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
   bmi.bmiHeader.biPlanes      = 1;
   bmi.bmiHeader.biCompression = BI_RGB;
   bmi.bmiHeader.biWidth       = size.cx;
   bmi.bmiHeader.biHeight      = size.cy;
   bmi.bmiHeader.biBitCount    = 32;

   HDC theDC = hdc ? hdc : GetDC(nullptr);
   if(theDC)
   {
      *phBmp = CreateDIBSection(theDC, &bmi, DIB_RGB_COLORS, bits, nullptr, 0);
      if(theDC != hdc)
         ReleaseDC(nullptr, theDC);
   }

   return (*phBmp != nullptr);
}

//
// Given an HICON handle, return a PARGB32 HBITMAP. The HICON will be deleted.
//
HBITMAP PARGB32Utils::bitmapFromIcon(HICON hIcon)
{
   HBITMAP ret = nullptr;

   if(!hIcon)
      return nullptr;

   // must have UXTHEME utilities
   if(!pBeginBufferedPaint || !pEndBufferedPaint || !pGetBufferedPaintBits)
   {
      DestroyIcon(hIcon);
      return nullptr;
   }

   // get small icon size
   SIZE iconSize;
   iconSize.cx = GetSystemMetrics(SM_CXSMICON);
   iconSize.cy = GetSystemMetrics(SM_CYSMICON);

   // calculate icon rect
   RECT iconRect;
   SetRect(&iconRect, 0, 0, iconSize.cx, iconSize.cy);

   HDC hdcDest = CreateCompatibleDC(nullptr);
   if(hdcDest)
   {
      // create a 32-bit bitmap
      if(create32BitBitmap(hdcDest, iconSize, nullptr, &ret))
      {
         // paint the bitmap with the icon contents using the screen-compatible DC
         HBITMAP curBmp = static_cast<HBITMAP>(SelectObject(hdcDest, ret));
         if(curBmp)
         {
            // set up paint parameters
            BLENDFUNCTION  alphaBlend  = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
            BP_PAINTPARAMS paintParams = { 0 };
            paintParams.cbSize         = sizeof(paintParams);
            paintParams.dwFlags        = BPPF_ERASE;
            paintParams.pBlendFunction = &alphaBlend;

            // do the drawing
            HDC hdcBuffer;
            HPAINTBUFFER hPaintBuffer = pBeginBufferedPaint(hdcDest, &iconRect, BPBF_DIB, &paintParams, &hdcBuffer);
            if(hPaintBuffer)
            {
               if(DrawIconEx(hdcBuffer, 0, 0, hIcon, iconSize.cx, iconSize.cy, 0, nullptr, DI_NORMAL))
               {
                  // if has no alpha channel, the buffer requires conversion to PARGB32
                  ConvertBufferToPARGB32(hPaintBuffer, hdcDest, hIcon, iconSize);
               }

               // write buffer contents to destination bitmap
               pEndBufferedPaint(hPaintBuffer, TRUE);
            }

            SelectObject(hdcDest, curBmp);
         }
      }

      DeleteDC(hdcDest);
   }

   DestroyIcon(hIcon);

   if(ret)
   {
      auto newImage = new ImageNode();
      newImage->hBitmap = ret;
      images.insert(newImage);
   }

   return ret;
}

// Global singleton
PARGB32Utils gPARGB32Utils;

// EOF

