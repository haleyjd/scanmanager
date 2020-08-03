// This is the main DLL file.

#include "stdafx.h"

#include "WiaAutomationProxy.h"

void WiaAutomationProxy::CommonDialog::ShowPhotoPrintingWizard(const char ** filenames, unsigned int numfilenames)
{
   WIA::CommonDialogClass cdc;
   WIA::VectorClass vector;

   for(unsigned int i = 0; i < numfilenames; i++)
      vector.Add(gcnew System::String(filenames[i]), 0);

   cdc.ShowPhotoPrintingWizard(%vector);
}

// EOF

