/*
  Scan Manager

  Document reader
*/

#ifndef DOCREAD_H__
#define DOCREAD_H__

#include <set>
#include "imagelist.h"

void ScanMgr_DeleteImageBuffers();
bool ScanMgr_ReadDocumentFromPath(const std::string &inpath, ImageList &list);
bool ScanMgr_GetDocumentImagePaths(const std::string &inpath, std::set<std::string> &filenames);
bool ScanMgr_ReadPDFDocumentFromPath(const std::string &inpath, std::string &outpath);

#endif

// EOF

