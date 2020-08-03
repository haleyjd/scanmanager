/*
   Native C++ consumable header for WiaAutomationProxy utils
*/

#ifndef WIAAUTOMATIONPROXY_H__
#define WIAAUTOMATIONPROXY_H__

namespace WiaAutomationProxy 
{
   class __declspec(dllexport) CommonDialog
   {
   public:
      static void ShowPhotoPrintingWizard(const char **filenames, unsigned int numfilenames);
   };
}

#endif

// EOF
