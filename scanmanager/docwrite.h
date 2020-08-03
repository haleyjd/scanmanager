/*
  Scan Manager

  Document writer
*/

#ifndef DOCWRITE_H__
#define DOCWRITE_H__

#include <exception>
#include <string>
#include <stdio.h>
#include "imagelist.h"

class PrometheusUser;

// Reasons why a document write can fail.
enum docwritefail_e
{
   DOCWRITE_OK,             // 0 is not an error
   DOCWRITE_NOCONNECTION,   // cannot connect to the file share
   DOCWRITE_NODIR,          // cannot create a new document directory
   DOCWRITE_IMGWRITEFAILED, // an exception was thrown by the image writer
   DOCWRITE_DATABASEERROR,  // database error
   DOCWRITE_UNKNOWNERROR    // something strange happened.
};

// structure tracking status of a document write process
class DocWriteStatus
{
public:
   docwritefail_e code;     // if non-0 after write, an error has occurred
   std::string    errorMsg; // contains the error message describing the failure reason, if any
   std::string    path;     // when successful, this contains the document's output path
};

class DocException : public std::exception
{
protected:
   char m_msg[128];

public:
   DocException(const char *msg) : exception()
   {
      snprintf(m_msg, sizeof(m_msg), "%s", msg);
   }

   virtual const char *what() const
   {
      return m_msg;
   }
};

bool ScanMgr_ConnectToShare();
void ScanMgr_CloseShare();
bool ScanMgr_RemoveFailedDocument(const std::string &path);
bool ScanMgr_WriteDocument(DocWriteStatus &status, const ImageList &il);
bool ScanMgr_WritePDFDocument(DocWriteStatus &status, const std::string &filepath);
bool ScanMgr_WriteDocumentRecord(DocWriteStatus &status, PrometheusUser &user, 
                                 const std::string &personID, const std::string &title, 
                                 const std::string &date, std::string &filepath, 
                                 const std::string &type);

#endif

// EOF

