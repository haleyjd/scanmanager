/*
  Scan Manager

  Image list; maintains the current set of scanned-in images.
*/

#ifndef IMAGELIST_H__
#define IMAGELIST_H__

#include <Windows.h>
#include <Unknwn.h>
#include <gdiplus.h>
#include "dllist.h"

class GDIPImageNode
{
public:
   DLListItem<GDIPImageNode> links;
   Gdiplus::Bitmap *gdiBitmap;

   ~GDIPImageNode()
   {
      // unlink from list
      links.remove();

      // free the Gdiplus bitmap object
      if(gdiBitmap)
         delete gdiBitmap;
      gdiBitmap = nullptr;
   }
};

typedef DLList<GDIPImageNode, &GDIPImageNode::links> GDIPImageList;

class ImageNode
{
public:
   DLListItem<ImageNode> links;      // linked list links
   HBITMAP               hBitmap;    // HBITMAP from TWAIN device
   Gdiplus::Bitmap      *gdiBitmap;  // current GDI bitmap
   GDIPImageList         prevImages; // previous versions for undo

   ~ImageNode()
   {
      // unlink from list
      links.remove();

      // free image
      if(hBitmap)
         DeleteObject(hBitmap);
      hBitmap = nullptr;

      // delete the Gdiplus bitmap if it is valid
      if(gdiBitmap)
         delete gdiBitmap;
      gdiBitmap = nullptr;

      // delete any backup copies
      while(prevImages.head)
         delete prevImages.head->dllObject;
   }

   // Create a backup copy of the Gdiplus bitmap
   void backup()
   {
      if(!gdiBitmap)
         return;

      auto pGdipNode = new GDIPImageNode();
      pGdipNode->gdiBitmap = gdiBitmap->Clone(0, 0, INT(gdiBitmap->GetWidth()), INT(gdiBitmap->GetHeight()), PixelFormatDontCare);
      prevImages.insert(pGdipNode);
   }

   // If there is a backup Gdiplus bitmap, restore it to be the current image
   bool restoreBackup()
   {
      if(!prevImages.head)
         return false;

      // if we currently own one, delete it.
      if(gdiBitmap)
         delete gdiBitmap;

      // take over the first backup object's bitmap and then delete it
      GDIPImageNode *pNode = prevImages.head->dllObject;
      gdiBitmap = pNode->gdiBitmap;
      pNode->gdiBitmap = nullptr;
      delete pNode;

      return true;
   }
};

typedef DLList<ImageNode, &ImageNode::links> ImageList;

#endif

// EOF

