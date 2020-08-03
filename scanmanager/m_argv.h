/*
  Scan Manager
 
  Argument manager
*/

#ifndef M_ARGV_H__
#define M_ARGV_H__

extern char **argv;
extern int    argc;

bool M_FindArgument(const char *arg);
int  M_GetArgParameter(const char *arg, int count);

#endif

// EOF

