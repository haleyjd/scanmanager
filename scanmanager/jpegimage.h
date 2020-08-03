/*
  Scan Manager

  JPEG Image Management

*/

#ifndef JPEGIMAGE_H__
#define JPEGIMAGE_H__

#include <Windows.h>
#include <stdint.h>
#include <memory>

#define CXIMAGE_DEFAULT_DPI 96

class CxMemFile;

//
// Stores unpacked data extracted from the DIB HBITMAP returned by a 
// scanner device.
//
class BitmapImage
{
public:
   struct imageinfo_t
   {
      uint8_t *pImage;
      uint32_t effWidth;
      int32_t  xDPI;
      int32_t  yDPI;
   };

protected:
   BITMAPINFOHEADER header;
   imageinfo_t      info;
   
   std::unique_ptr<uint8_t []> pDib;

   void  startup();
   void *create(uint32_t width, uint32_t height, uint32_t bpp, uint32_t stride);

public:
   BitmapImage(HBITMAP hBmp);

   void      setXDPI(int32_t dpi);
   void      setYDPI(int32_t dpi);
   int32_t   getXDPI()   const { return info.xDPI;       }
   int32_t   getYDPI()   const { return info.yDPI;       }
   uint32_t  getWidth()  const { return header.biWidth;  }
   uint32_t  getHeight() const { return header.biHeight; }

   int32_t   getSize() const;
   uint32_t  getPaletteSize() const;
   RGBQUAD  *getPalette() const;
   uint8_t  *getBits(uint32_t row = 0) const;
   RGBQUAD   getPaletteColor(uint8_t idx) const;
   uint8_t   blindGetPixelIndex(int32_t x, int32_t y) const;
   RGBQUAD   blindGetPixelColor(int32_t x, int32_t y) const;
   bool      encodeToRGB(CxMemFile *hFile, bool bFlipY = false);
   bool      encodeToRGB(uint8_t *&buffer, int32_t &size, bool bFlipY = false);
   bool      isGrayScale() const;
   bool      createFromRGB(const uint8_t *pArray, uint32_t width, uint32_t height, uint32_t stride, bool flipimage);
   bool      writeJPEG(const char *filename, int quality);
};

#endif

// EOF

