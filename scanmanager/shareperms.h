/*
  Scan Manager

  Authenticated file share access code
*/

#ifndef SHAREPERMS_H__
#define SHAREPERMS_H__

#include <Windows.h>
#include <string>

DWORD ScanMgr_AuthenticateShare(NETRESOURCEA &nr, const std::string &path, const std::string &user, const std::string &pwd);
void  ScanMgr_DisconnectShare(NETRESOURCEA &nr);

#endif

// EOF

