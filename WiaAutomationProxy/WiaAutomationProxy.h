// WiaAutomationProxy.h

#pragma once

using namespace System;

namespace WiaAutomationProxy 
{
   class __declspec(dllexport) CommonDialog
   {
   public:
      static void ShowPhotoPrintingWizard(const char **filenames, unsigned int numfilenames);
   };
}
