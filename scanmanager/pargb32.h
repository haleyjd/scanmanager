/*
  Scan Manager
 
  PARGB32 Bitmap utilities
*/

#ifndef PARGB32_H__
#define PARGB32_H__

#include "imagelist.h"

class PARGB32Utils
{
protected:
   ImageList images;

public:
   PARGB32Utils();
   ~PARGB32Utils();

   bool    create32BitBitmap(HDC hdc, const SIZE &size, void **bits, HBITMAP *phBmp);
   HBITMAP bitmapFromIcon(HICON hIcon);
};

// Global singleton
extern PARGB32Utils gPARGB32Utils;

#endif

// EOF

