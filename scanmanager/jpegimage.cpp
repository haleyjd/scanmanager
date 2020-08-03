/*
  Scan Manager

  JPEG Image File Manipulation

*/

#include <Windows.h>
#include <gdiplus.h>
#include <stdio.h>
#include <exception>
#include <math.h>
#include <setjmp.h>
#include "docwrite.h"
#include "jpegimage.h"
#include "memfile.h"

#include "jpeg-9b/jpeglib.h"

//=============================================================================
//
// BitmapImage methods
//

//
// Return palette dimensions in bytes
//
uint32_t BitmapImage::getPaletteSize() const 
{
   return header.biClrUsed * sizeof(RGBQUAD);
}

//
// Return a pointer to the first palette index
//
RGBQUAD *BitmapImage::getPalette() const 
{
   uint8_t *paletteBytes;

   if((paletteBytes = pDib.get()) && header.biClrUsed)
      return (RGBQUAD *)(paletteBytes + sizeof(BITMAPINFOHEADER));
   else
      return nullptr;
}

//
// Return size in bytes of the internal pDib object
//
int32_t BitmapImage::getSize() const
{
   return header.biSize + header.biSizeImage + getPaletteSize();
}

//
// Protected method to create image store
//
void *BitmapImage::create(uint32_t width, uint32_t height, uint32_t bpp, uint32_t stride)
{
   if(width == 0 || width >= 65536u || height == 0 || height >= 65536u)
      throw DocException("Invalid image dimensions");

   if(bpp <= 1)
      bpp = 1;
   else if(bpp <= 4)
      bpp = 4;
   else if(bpp <= 8)
      bpp = 8;
   else
      bpp = 24;

   switch(bpp)
   {
   case 1:
      header.biClrUsed = 2;
      break;
   case 4:
      header.biClrUsed = 16;
      break;
   case 8:
      header.biClrUsed = 256;
      break;
   default:
      header.biClrUsed = 0;
      break;
   }

   info.effWidth = stride; //((((bpp * width) + 31) / 32) * 4);

   header.biSize        = sizeof(BITMAPINFOHEADER);
   header.biWidth       = width;
   header.biHeight      = height;
   header.biPlanes      = 1;
   header.biBitCount    = uint16_t(bpp);
   header.biCompression = BI_RGB;
   header.biSizeImage   = info.effWidth * height;

   pDib.reset(new byte [getSize()]);
   RGBQUAD *pal = getPalette();
   if(pal)
      memset(pal, 0, getPaletteSize());

   auto lpbi = reinterpret_cast<BITMAPINFOHEADER *>(pDib.get());
   *lpbi = header;

   info.pImage = getBits();

   return pDib.get();
}

//
// Return pointer to the image pixels.
//
uint8_t *BitmapImage::getBits(uint32_t row) const
{
   byte *dib;

   if((dib = pDib.get()))
   {
      if(row)
      {
         if(row < uint32_t(header.biHeight))
            return (dib + *(uint32_t *)dib + getPaletteSize() + (info.effWidth * row));
         else
            return nullptr;
      }
      else
         return (dib + *(uint32_t *)dib + getPaletteSize());
   }

   return nullptr;
}

void BitmapImage::setXDPI(int32_t dpi)
{
   if(dpi <= 0)
      dpi = CXIMAGE_DEFAULT_DPI;
   info.xDPI = dpi;
   header.biXPelsPerMeter = int32_t(floor(dpi * 10000.0 / 254.0 + 0.5));
   byte *dib;
   if((dib = pDib.get()))
      ((BITMAPINFOHEADER *)dib)->biXPelsPerMeter = header.biXPelsPerMeter;
}

void BitmapImage::setYDPI(int32_t dpi)
{
   if(dpi <= 0)
      dpi = CXIMAGE_DEFAULT_DPI;
   info.yDPI = dpi;
   header.biYPelsPerMeter = int32_t(floor(dpi * 10000.0 / 254.0 + 0.5));
   byte *dib;
   if((dib = pDib.get()))
      ((BITMAPINFOHEADER *)dib)->biYPelsPerMeter = header.biYPelsPerMeter;
}

//
// Initialize data structures
//
void BitmapImage::startup()
{
   pDib.reset(nullptr);
   memset(&header, 0, sizeof(BITMAPINFOHEADER));
   info.pImage = nullptr;
   info.effWidth = 0;
   setXDPI(CXIMAGE_DEFAULT_DPI);
   setYDPI(CXIMAGE_DEFAULT_DPI);
}

//
// Constructor for HBITMAP
//
BitmapImage::BitmapImage(HBITMAP hBmp) : info(), pDib()
{
   startup();

   if(!hBmp)
      throw DocException("Invalid HBITMAP");

   std::unique_ptr<Gdiplus::Bitmap> upBitmap;
   Gdiplus::Bitmap *bitmap;

   auto dib = PBITMAPINFO(GlobalLock(hBmp));
   if(!dib)
      throw DocException("Cannot lock HBITMAP");
   std::unique_ptr<std::remove_pointer<HBITMAP>::type, void (*)(HBITMAP)> upHBmp(hBmp, [] (HBITMAP hBmp) {
      GlobalUnlock(hBmp);
   });

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
      throw DocException("Invalid bitdepth in HBITMAP, cannot convert");
   }

   upBitmap.reset(new Gdiplus::Bitmap(dib, (uint8_t *)dib + sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * paletteSize)));
   if(!(bitmap = upBitmap.get()))
      throw DocException("Failed to instantiate a Gdiplus::Bitmap object");

   if(bitmap->GetLastStatus() != Gdiplus::Ok)
      throw DocException("Gdiplus::Bitmap object reports invalid status");

   Gdiplus::BitmapData bmpData;
   if(bitmap->LockBits(&Gdiplus::Rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight()), Gdiplus::ImageLockModeRead, PixelFormat24bppRGB, &bmpData) != Gdiplus::Ok)
      throw DocException("Could not lock bits from Gdiplus::Bitmap");

   if(!create(bmpData.Width, bmpData.Height, 24, uint32_t(abs(bmpData.Stride))))
   {
      bitmap->UnlockBits(&bmpData);
      throw DocException("Could not create internal BitmapImage buffer");
   }

   uint8_t *start;
   if(bmpData.Stride < 0)
      start = (uint8_t *)bmpData.Scan0 - (bmpData.Height - 1) * -bmpData.Stride;
   else
      start = (uint8_t *)bmpData.Scan0;

   memcpy(info.pImage, start, info.effWidth * bmpData.Height);

   bitmap->UnlockBits(&bmpData);
}

//
// Get a palette color for an index.
//
RGBQUAD BitmapImage::getPaletteColor(uint8_t idx) const
{
   RGBQUAD rgb = { 0, 0, 0, 0 };
   byte *dib;

   if((dib = pDib.get()) && header.biClrUsed)
   {
      uint8_t *iDst = dib + sizeof(BITMAPINFOHEADER);
      if(idx < header.biClrUsed)
      {
         int32_t ldx = idx * sizeof(RGBQUAD);
         rgb.rgbBlue     = iDst[ldx++];
         rgb.rgbGreen    = iDst[ldx++];
         rgb.rgbRed      = iDst[ldx++];
         rgb.rgbReserved = 255u;
      }
   }

   return rgb;
}

//
// Unchecked pixel retrieval for paletted images
//
uint8_t BitmapImage::blindGetPixelIndex(int32_t x, int32_t y) const
{
   if(header.biBitCount == 8)
   {
      return info.pImage[y * info.effWidth + x];
   }
   else
   {
      uint8_t pos;
      uint8_t iDst = info.pImage[y * info.effWidth + (x * header.biBitCount >> 3)];
      if(header.biBitCount == 4)
      {
         pos = uint8_t(4 * (1 - x % 2));
         iDst &= (0x0F << pos);
         return uint8_t(iDst >> pos);
      }
      else if(header.biBitCount == 1)
      {
         pos = uint8_t(7 - x % 8);
         iDst &= (0x01 << pos);
         return uint8_t(iDst >> pos);
      }
   }

   return 0;
}

//
// Unchecked pixel color retrieval routine.
//
RGBQUAD BitmapImage::blindGetPixelColor(int32_t x, int32_t y) const
{
   RGBQUAD rgb;

   if(header.biClrUsed)
   {
      rgb = getPaletteColor(blindGetPixelIndex(x, y));
   }
   else
   {
      uint8_t *iDst = info.pImage + y * info.effWidth + x * 3;
      rgb.rgbBlue  = *iDst++;
      rgb.rgbGreen = *iDst++;
      rgb.rgbRed   = *iDst;
      rgb.rgbReserved = 255u;
   }

   return rgb;
}

//
// Export the image into an RGB buffer.
//
bool BitmapImage::encodeToRGB(CxMemFile *hFile, bool bFlipY)
{
   if(hFile == nullptr)
      return false;

   if(pDib.get() == nullptr)
      return false;

   for(int32_t y1 = 0; y1 < header.biHeight; y1++)
   {
      int32_t y = bFlipY ? header.biHeight - 1 - y1 : y1;
      
      for(int32_t x = 0; x < header.biWidth; x++)
      {
         RGBQUAD color = blindGetPixelColor(x, y);
         hFile->putc(color.rgbRed);
         hFile->putc(color.rgbGreen);
         hFile->putc(color.rgbBlue);
      }
   }

   return true;
}

//
// Export the image into an RGB buffer.
//
bool BitmapImage::encodeToRGB(uint8_t *&buffer, int32_t &size, bool bFlipY)
{
   if(buffer != nullptr)
      return false;

   CxMemFile file;
   file.open();
   if(encodeToRGB(&file, bFlipY))
   {
      buffer = file.getBuffer();
      size   = file.size();
      return true;
   }

   return false;
}

//
// True if image has a 256-color linear gray palette
//
bool BitmapImage::isGrayScale() const
{
   RGBQUAD *ppal = getPalette();

   if(!(pDib.get() && ppal && header.biClrUsed))
      return false;

   for(uint32_t i = 0; i < header.biClrUsed; i++)
   {
      if(ppal[i].rgbBlue != i || ppal[i].rgbGreen != i || ppal[i].rgbRed != i)
         return false;
   }

   return true;
}

bool BitmapImage::createFromRGB(const uint8_t *pArray, uint32_t width, uint32_t height, uint32_t stride, bool flipimage)
{
   if(pArray == nullptr)
      return false;

   if(!create(width, height, 3, stride))
      return false;

   const uint8_t *src;
   uint8_t *dst;

   for(uint32_t y = 0; y < height; y++)
   {
      dst = info.pImage + (flipimage ? (height - 1 - y) : y) * info.effWidth;
      src = pArray + y * width * 4;
      for(uint32_t x = 0; x < width; x++)
      {
         *dst++ = src[2]; // B
         *dst++ = src[1]; // G
         *dst++ = src[0]; // R
         src += 3;
      }
   }

   return true;
}

//
// Write image to a JPEG file
//
bool BitmapImage::writeJPEG(const char *filename, int quality)
{
   struct jpeg_compress_struct cinfo;
   struct jpeg_error_mgr       jerr;

   uint8_t *inputImg  = nullptr;
   int32_t  inputSize = 0;

   FILE *outfile;
   JSAMPROW row_pointer[1];
   int row_stride;

   // get input RGB data
   if(!encodeToRGB(inputImg, inputSize, true) || !inputImg)
      return false;

   // open output file
   if((outfile = fopen(filename, "wb")) == nullptr)
   {
      free(inputImg);
      return false;
   }

   // allocate and initialize JPEG compression object

   // setup error handler first
   cinfo.err = jpeg_std_error(&jerr);
   jpeg_create_compress(&cinfo);

   // specify data destination

   jpeg_stdio_dest(&cinfo, outfile);

   // set parameters for compression

   cinfo.image_width      = header.biWidth;
   cinfo.image_height     = header.biHeight;
   cinfo.input_components = 3;
   cinfo.in_color_space   = JCS_RGB;
   jpeg_set_defaults(&cinfo);
   jpeg_set_quality(&cinfo, quality, TRUE);

   // start compressor

   jpeg_start_compress(&cinfo, TRUE);

   // write scanlines

   row_stride = cinfo.image_width * 3;

   while(cinfo.next_scanline < cinfo.image_height)
   {
      row_pointer[0] = &inputImg[cinfo.next_scanline * row_stride];
      jpeg_write_scanlines(&cinfo, row_pointer, 1);
   }

   // finish compression

   jpeg_finish_compress(&cinfo);

   // close file and free temp RGB image

   fclose(outfile);
   free(inputImg);

   // destroy compressor

   jpeg_destroy_compress(&cinfo);

   return true;
}

// EOF

