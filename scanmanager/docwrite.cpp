/*
  Scan Manager

  Document writer
*/

#include <rpc.h>
#include <exception>
#include <direct.h>
#include <Windows.h>
#include <gdiplus.h>
#include "cached_files.h"
#include "docwrite.h"
#include "i_opndir.h"
#include "imagelist.h"
#include "jpegimage.h"
#include "prometheusdb.h"
#include "promuser.h"
#include "scanmanager.h"
#include "shareperms.h"
#include "sqlLib.h"
#include "util.h"

//
// Defines
//

// File share path
#define CHS_FILESHARE_BASE "\\\\<chsserver>\\CHS"

// User credentials
#define CHS_FILESHARE_USER "<chsdomain>\\<chsuser>"
#define CHS_FILESHARE_PWD  "<chssharepwd>"

// JPEG options
#define SCANMGR_JPEG_QUALITY 100

//
// Module globals
//

// Global file share network resource
static NETRESOURCEA gnrFileShare;
static bool         gShareConnected;

//=============================================================================
//
// Local code
//

//
// Create a new globally unique directory name
//
static bool ScanMgr_GetUniqueDirectoryName(std::string &out)
{
   UUID uuid;
   RPC_STATUS ret = UuidCreate(&uuid);

   if(ret == RPC_S_OK)
   {
      RPC_CSTR szUuid = nullptr;
      UuidToStringA(&uuid, &szUuid);
      if(szUuid)
      {
         out = reinterpret_cast<const char *>(szUuid);
         RpcStringFreeA(&szUuid);
      }
      else
         ret = RPC_S_OUT_OF_MEMORY;
   }

   return (ret == RPC_S_OK);
}

//
// Create a new uniquely named directory under the root file share for the new document.
//
static bool ScanMgr_CreateNewDocumentDir(const std::string &basePath, std::string &out)
{
   bool ret = false;
   int dirsToTry = 100;

   FileCache::SetBasePath(basePath);

   while(dirsToTry--)
   {
      std::string dirName;
      if(ScanMgr_GetUniqueDirectoryName(dirName))
      {
         std::string combDirName = FileCache::PathConcatenate(basePath, dirName);
         if(!FileCache::DirectoryExists(combDirName, false))
         {
            if(FileCache::CreateDirectorySingle(dirName))
            {
               out = combDirName;
               ret = true;
               break;
            }
         }
      }
   }

   return ret;
}

static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   UINT num = 0;  // number of image encoders
   UINT size = 0; // size of the image encoder array in bytes

   Gdiplus::ImageCodecInfo *pImageCodecInfo = nullptr;

   Gdiplus::GetImageEncodersSize(&num, &size);
   if(size == 0)
      return -1;  // Failure

   pImageCodecInfo = static_cast<Gdiplus::ImageCodecInfo *>(malloc(size));
   if(pImageCodecInfo == nullptr)
      return -1;  // Failure

   Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j)
   {
      if(wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
      {
         *pClsid = pImageCodecInfo[j].Clsid;
         free(pImageCodecInfo);
         return j;  // Success
      }    
   }

   free(pImageCodecInfo);
   return -1;  // Failure
}

//
// Write one image file to the server share
//
static bool ScanMgr_WriteOneImage(DocWriteStatus &status, const std::string &path, ImageNode &node)
{
   try
   {
      Gdiplus::Bitmap *bitmap;
      Gdiplus::EncoderParameters encParams;

      if(!node.gdiBitmap)
         ScanMgr_HBITMAPToGdiplusBitmap(&node);

      if(!(bitmap = node.gdiBitmap))
      {
         status.code     = DOCWRITE_IMGWRITEFAILED;
         status.errorMsg = "Invalid Gdiplus Bitmap object";
         return false;
      }

      if(bitmap->GetLastStatus() != Gdiplus::Ok)
      {
         status.code     = DOCWRITE_IMGWRITEFAILED;
         status.errorMsg = "Gdiplus Bitmap object had bad status.";
         return false;
      }

      size_t reqSize = mbstowcs(nullptr, path.c_str(), 0);
      std::unique_ptr<wchar_t []> upWCS(new wchar_t [reqSize + 1]);
      size_t size = mbstowcs(upWCS.get(), path.c_str(), reqSize + 1);
      if(size == size_t(-1))
      {
         status.code     = DOCWRITE_IMGWRITEFAILED;
         status.errorMsg = "Could not convert filepath to wide char string.";
         return false;
      }

      CLSID jpegCLSID;
      if(GetEncoderClsid(L"image/jpeg", &jpegCLSID) < 0)
      {
         status.code     = DOCWRITE_IMGWRITEFAILED;
         status.errorMsg = "Could not retrieve CLSID for image/jpeg MIME type.";
         return false;
      }

      encParams.Count = 1;
      encParams.Parameter[0].Guid = Gdiplus::EncoderQuality;
      encParams.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
      encParams.Parameter[0].NumberOfValues = 1;

      ULONG quality = SCANMGR_JPEG_QUALITY;
      encParams.Parameter[0].Value = &quality;

      if(bitmap->Save(upWCS.get(), &jpegCLSID, &encParams) != Gdiplus::Ok)
      {
         status.code     = DOCWRITE_IMGWRITEFAILED;
         status.errorMsg = "Could not save Gdiplus image to JPEG.";
         return false;
      }

      return true;
   }
   catch(const std::exception &ex)
   {
      status.code     = DOCWRITE_IMGWRITEFAILED;
      status.errorMsg = ex.what();
      return false;
   }
   catch(...)
   {
      status.code     = DOCWRITE_IMGWRITEFAILED;
      status.errorMsg = "An unknown error occurred during image writing.";
      return false;
   }
}

//
// Write out a list of images to the server share
//
static bool ScanMgr_WriteImageList(DocWriteStatus &status, const std::string &basePath, const ImageList &il)
{
   auto img = il.head;
   char filename[16];
   int  imagenum = 0;

   while(img)
   {
      _snprintf(filename, sizeof(filename), "%08d.jpg", imagenum++);
      std::string fullpath = FileCache::PathConcatenate(basePath, filename);

      if(!ScanMgr_WriteOneImage(status, fullpath, *(img->dllObject)))
         return false;

      img = img->dllNext;
   }

   return true;
}

//
// Copy a single PDF file to the server share
//
static bool ScanMgr_WritePDFFile(DocWriteStatus &status, const std::string &basePath, const std::string &srcPath)
{
   std::string filename = FileCache::GetFileSpec(srcPath).second;
   std::string destPath = FileCache::PathConcatenate(basePath, filename);

   if(CopyFileA(srcPath.c_str(), destPath.c_str(), TRUE) != 0)
   {
      return true;
   }
   else
   {
      LPVOID lpMsgBuf;
      DWORD  err = GetLastError();

      FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                     nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, nullptr);

      status.code     = DOCWRITE_IMGWRITEFAILED;
      status.errorMsg = (LPSTR)lpMsgBuf;

      LocalFree(lpMsgBuf);
      return false;
   }
}

//=============================================================================
//
// Global Interface
//

//
// Establish a network connection to the CHS server's file share root folder if one hasn't been opened already.
//
bool ScanMgr_ConnectToShare()
{
   if(gShareConnected)
      return true; // already connected

   DWORD code = ScanMgr_AuthenticateShare(gnrFileShare, CHS_FILESHARE_BASE, CHS_FILESHARE_USER, CHS_FILESHARE_PWD);

   return (gShareConnected = (code == NO_ERROR));
}

//
// Call at shutdown to disconnect the share
//
void ScanMgr_CloseShare()
{
   if(gShareConnected)
      ScanMgr_DisconnectShare(gnrFileShare);
   gShareConnected = false;
}

//
// Remove all files and the created directory in the event of an error.
//
bool ScanMgr_RemoveFailedDocument(const std::string &path)
{
   bool ret = true;
   DIR *dir;

   // try to delete all files
   if((dir = opendir(path.c_str())))
   {
      bool removedAllFiles = true;
      dirent *ent;

      while((ent = readdir(dir)))
      {
         if(ent->d_name[0] != '.')
         {
            std::string fullName = FileCache::PathConcatenate(path, ent->d_name);
            if(remove(fullName.c_str()))
               removedAllFiles = false; // couldn't delete file
         }
      }

      if(!removedAllFiles)
         ret = false;

      closedir(dir);
   }
   else
      ret = false;

   // try to remove directory
   if(_rmdir(path.c_str()))
      ret = false; // could not remove directory

   return ret;
}

//
// Write a document consisting of the images in il.
// Success or failure information is returned in the status structure.
//
bool ScanMgr_WriteDocument(DocWriteStatus &status, const ImageList &il)
{
   status.code     = DOCWRITE_UNKNOWNERROR;
   status.errorMsg = "An unknown error has occurred.";
   status.path     = "";

   // Connect to the global CHS document share
   if(!ScanMgr_ConnectToShare())
   {
      status.code     = DOCWRITE_NOCONNECTION;
      status.errorMsg = "Cannot connect to CHS document file share.";
      return false;
   }

   // Create a unique document directory
   if(!ScanMgr_CreateNewDocumentDir(CHS_FILESHARE_BASE, status.path))
   {
      status.code     = DOCWRITE_NODIR;
      status.errorMsg = "Cannot create a new document directory.";
      return false;
   }

   // Write the images to the directory
   if(!ScanMgr_WriteImageList(status, status.path, il))
   {
      // the status code and message were set by the image writing process.
      return false;
   }

   // successful!
   status.code     = DOCWRITE_OK;
   status.errorMsg = "";
   return true;
}

//
// Write a document consisting of a single PDF file.
// Success or failure information is returned in the status structure.
//
bool ScanMgr_WritePDFDocument(DocWriteStatus &status, const std::string &filepath)
{
   status.code     = DOCWRITE_UNKNOWNERROR;
   status.errorMsg = "An unknown error has occurred.";
   status.path     = "";

   // Connect to the global CHS document share
   if(!ScanMgr_ConnectToShare())
   {
      status.code     = DOCWRITE_NOCONNECTION;
      status.errorMsg = "Cannot connect to CHS document file share.";
      return false;
   }

   // Create a unique document directory
   if(!ScanMgr_CreateNewDocumentDir(CHS_FILESHARE_BASE, status.path))
   {
      status.code     = DOCWRITE_NODIR;
      status.errorMsg = "Cannot create a new document directory.";
      return false;
   }

   // Copy the PDF to the directory
   if(!ScanMgr_WritePDFFile(status, status.path, filepath))
   {
      // the status code and message were set by the write function.
      return false;
   }

   // successful!
   status.code     = DOCWRITE_OK;
   status.errorMsg = "";
   return true;
}


//=============================================================================
//
// Document database record creation
//

bool ScanMgr_WriteDocumentRecord(DocWriteStatus &status, PrometheusUser &user, 
                                 const std::string &personID, const std::string &title, 
                                 const std::string &date, std::string &filepath, 
                                 const std::string &type)
{
   PrometheusDB &db = user.getDatabase();
   PrometheusTransaction tr;
   pdb::stringmap   fields;
   pdb::strtointmap options;
   std::string id;

   status.code     = DOCWRITE_UNKNOWNERROR;
   status.errorMsg = "An unknown error occurred.";

   PrometheusLookups::LoadLookup(db, "current_types");
   PrometheusLookups::LoadLookup(db, "document_types");

   if(!tr.stdTransaction(db))
   {
      status.code     = DOCWRITE_DATABASEERROR;
      status.errorMsg = "Could not open a database transaction.";
      return false;
   }

   tr.getNextId("documents", id);
   if(!StringToInt(id))
   {
      status.code     = DOCWRITE_DATABASEERROR;
      status.errorMsg = "Could not reserve a new document ID.";
      return false;
   }

   fields["id"               ] = id;
   fields["last_updated_by"  ] = std::to_string(user.getUserID());
   fields["last_update_stamp"] = RightNow();
   fields["created_on"       ] = fields["last_update_stamp"];
   fields["current_id"       ] = PrometheusLookups::IdOf("current_types", "Current");
   fields["person_id"        ] = personID;
   fields["title"            ] = title;
   fields["filepath"         ] = filepath;
   fields["document_id"      ] = PrometheusLookups::IdOf("document_types", type);
   fields["received_date"    ] = date;

   options["filepath"] = sql_no_uppercasing;

   if(!tr.executeInsertStatement("documents", fields, &options))
   {
      status.code     = DOCWRITE_DATABASEERROR;
      status.errorMsg = "Failed to insert new document record.";
      return false;
   }

   if(tr.commit())
   {
      status.code     = DOCWRITE_OK;
      status.errorMsg = "";
      return true;
   }
   else
   {
      status.code     = DOCWRITE_DATABASEERROR;
      status.errorMsg = "Failed to commit transaction.";
      return false;
   }
}

// EOF

