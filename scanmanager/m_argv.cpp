/*
  Scan Manager
 
  Argument manager
*/

#include <stdlib.h>
#include <string.h>
#include "m_argv.h"

//
// Returns true if the specified argument was passed to the program,
// and false otherwise.
//
bool M_FindArgument(const char *arg)
{
   for(int i = 1; i < argc; i++)
   {
      if(!strcmp(argv[i], arg))
         return true;
   }

   return false;
}

//
// Returns the index of the first parameter to the specified argument, if
// the argument was passed and can have at least "count" required arguments
// after it on the command line. Returns 0 if either the parameter was not
// passed at all or it is too near the end of the command line.
//
int M_GetArgParameter(const char *arg, int count)
{
   for(int i = 1; i < argc; i++)
   {
      if(!strcmp(argv[i], arg))
      {
         if(i < argc - count)
            return i + 1;
         else
            return 0;
      }
   }

   return 0;
}

// EOF

