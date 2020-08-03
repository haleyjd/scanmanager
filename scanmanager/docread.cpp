/*
  Scan Manager

  Document reader
*/

#include <set>
#include <string>
#include <sys/stat.h>
#include <stdio.h>
#include "cached_files.h"
#include "docwrite.h"
#include "i_opndir.h"
#include "imagelist.h"

//=============================================================================
//
// Image buffer tracking
//

struct hglobalbuffer_t
{
   DLListItem<hglobalbuffer_t> links;
   HGLOBAL hBuffer;

   ~hglobalbuffer_t()
   {
      // remove from list
      links.remove();

      // unlock and free buffer if valid
      if(hBuffer)
      {
         GlobalUnlock(hBuffer);
         GlobalFree(hBuffer);
         hBuffer = nullptr;
      }
   }
};

typedef DLList<hglobalbuffer_t, &hglobalbuffer_t::links> HGlobalList;

static HGlobalList imageBuffers;

//
// Add an image buffer.
//
static void ScanMgr_AddImageBuffer(HGLOBAL hBuffer)
{
   auto newBuffer = new hglobalbuffer_t();
   newBuffer->hBuffer = hBuffer;
   imageBuffers.tailInsert(newBuffer);
}

//
// Delete all image buffers.
//
void ScanMgr_DeleteImageBuffers()
{
   while(imageBuffers.head)
      delete imageBuffers.head->dllObject;
}

//=============================================================================
//
// Local utilities
//

//
// Read in a single file in the directory
//
static bool ScanMgr_ReadDocumentFile(const std::string &inpath, const std::string &fn, Gdiplus::Bitmap *&bmpOut)
{
   std::string fullname = FileCache::PathConcatenate(inpath, fn);

   bmpOut = nullptr;

   struct stat st;
   if(stat(fullname.c_str(), &st))
      return false;

   long filesize = st.st_size;
   if(!filesize)
      return false;

   HGLOBAL hBuffer;
   if((hBuffer = GlobalAlloc(GMEM_MOVEABLE, SIZE_T(filesize))))
   {
      void *buffer = GlobalLock(hBuffer);
      if(buffer)
      {
         FILE *f;
         if((f = fopen(fullname.c_str(), "rb")))
         {
            if(fread(buffer, 1, filesize, f) == filesize)
            {
               IStream *pStream = nullptr;
               if(CreateStreamOnHGlobal(hBuffer, FALSE, &pStream) == S_OK)
               {
                  bmpOut = Gdiplus::Bitmap::FromStream(pStream);
                  pStream->Release();
                  if(bmpOut && bmpOut->GetLastStatus() == Gdiplus::Ok)
                  {
                     fclose(f);
                     ScanMgr_AddImageBuffer(hBuffer);
                     return true;
                  }

                  // bitmap creation failed
                  delete bmpOut;
                  bmpOut = nullptr;
               }
               // IStream creation failed
            }
            // fread failed
            fclose(f);
         }
         // fopen failed
         GlobalUnlock(hBuffer);
      }
      // GlobalLock failed
      GlobalFree(hBuffer);
   }
   // GlobalAlloc failed
   return false;
}

//=============================================================================
//
// Interface
//

//
// Read in the document from the path that was saved in the database.
//
bool ScanMgr_ReadDocumentFromPath(const std::string &inpath, ImageList &list)
{
   DIR     *dir;
   dirent  *ent;
   bool     res = true;
   std::set<std::string> filenames;

   if(!ScanMgr_ConnectToShare())
      return false;
   
   if(!(dir = opendir(inpath.c_str())))
      return false;

   while((ent = readdir(dir)))
   {
      if(!strstr(ent->d_name, ".jpg"))
         continue;

      // put the file names into a set so they will come out in numeric order.
      filenames.insert(ent->d_name);
   }

   closedir(dir);

   for(auto &fn : filenames)
   {
      auto node = new ImageNode();
      node->hBitmap   = nullptr;
      node->gdiBitmap = nullptr;

      if((res = ScanMgr_ReadDocumentFile(inpath, fn, node->gdiBitmap)))
         list.tailInsert(node);
      else
      {
         delete node;
         res = false;
         break;
      }
   }

   return res;
}

//
// Get the paths of all JPEG images that are part of the document.
//
bool ScanMgr_GetDocumentImagePaths(const std::string &inpath, std::set<std::string> &filenames)
{
   DIR     *dir;
   dirent  *ent;
   bool     res = true;

   if(!ScanMgr_ConnectToShare())
      return false;
   
   if(!(dir = opendir(inpath.c_str())))
      return false;

   while((ent = readdir(dir)))
   {
      if(!strstr(ent->d_name, ".jpg"))
         continue;

      filenames.insert(inpath + "\\" + ent->d_name);
   }

   closedir(dir);

   return res;
}

//
// Find a PDF document filepath from the path that was saved in the database.
//
bool ScanMgr_ReadPDFDocumentFromPath(const std::string &inpath, std::string &outpath)
{
   DIR     *dir;
   dirent  *ent;
   bool     res = false;

   if(!ScanMgr_ConnectToShare())
      return false;
   
   if(!(dir = opendir(inpath.c_str())))
      return false;

   while((ent = readdir(dir)))
   {
      if(!strstr(ent->d_name, ".pdf") && !strstr(ent->d_name, ".PDF"))
         continue;

      outpath = FileCache::PathConcatenate(inpath, ent->d_name);
      res = true;
      break; // there is only one PDF in the directory
   }

   closedir(dir);

   return res;
}

// EOF

