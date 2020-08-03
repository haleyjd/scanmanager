/*
  Scan Manager

  Authenticated file share access code
*/

#include <Windows.h>
#include <string>

#pragma comment(lib, "mpr.lib")

//
// Establish an authenticated connection to the given file share using the provided credentials.
//
DWORD ScanMgr_AuthenticateShare(NETRESOURCEA &nr, const std::string &path, const std::string &user, const std::string &pwd)
{
   nr.dwType       = RESOURCETYPE_DISK;
   nr.lpLocalName  = nullptr;
   nr.lpRemoteName = strdup(path.c_str());
   nr.lpProvider   = nullptr;

   return WNetAddConnection2A(&nr, pwd.c_str(), user.c_str(), CONNECT_TEMPORARY);
}

//
// Close network connection to file share
//
void ScanMgr_DisconnectShare(NETRESOURCEA &nr)
{
   WNetCancelConnection2A(nr.lpRemoteName, 0, TRUE);
}

// EOF

