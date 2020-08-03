//
// ScanManager
//
// Image effects dialog class
//

#include "stdafx.h"
#include <CommCtrl.h>
#include <ShlObj.h>
#include <gdiplus.h>
#include "vc2015/resource.h"
#include "scanmanager.h"
#include "effectdlg.h"
#include "imagelist.h"

//
// Data for setting up GDI+ image effects
//
static const fxparams_t fxParams[FXTYPE_MAX] =
{
   { 
      L"Sharpen Image:",
      FXTYPE_SHARPEN, FXPARAMS_REAL, 2,
      { L"Radius", L"Amount" },
      { { 0 }, { 0 }, { 0 } }, 
      { { 0.0f, 255.0f, 0.0f }, { 0.0f, 100.0f, 0.0f } }
   },
   { 
      L"Adjust Tint:",
      FXTYPE_TINT, FXPARAMS_INT, 2,
      { L"Hue", L"Amount" },
      { { -180, 180, 0 }, { -100, 100, 0 } }
   },
   { 
      L"Adjust Brightness and Contrast:",
      FXTYPE_BRIGHTNESS, FXPARAMS_INT, 2,
      { L"Brightness", L"Contrast" },
      { { -255, 255, 0 }, { -100, 100, 0 } }
   },
   {
      L"Adjust Hue, Saturation, and Lightness:",
      FXTYPE_HSL, FXPARAMS_INT, 3,
      { L"Hue", L"Saturation", L"Lightness" },
      { { -180, 180, 0 }, { -100, 100, 0 }, { -100, 100, 0 } }
   },
   {
      L"Adjust Color Balance:",
      FXTYPE_BALANCE, FXPARAMS_INT, 3,
      { L"Cyan/Red", L"Magenta/Green", L"Yellow/Blue" },
      { { -100, 100, 0 }, { -100, 100, 0 }, { -100, 100, 0 } }
   }
};

// Control ID set for each effect parameter slider
struct slideridgrp_t
{
   int sliderID;   // ID of slider control
   int fieldLblID; // ID of field name static label
   int minLblID;   // ID of minimum value static label
   int maxLblID;   // ID of maximum value static label
};

// IDs for the three groups of effect sliders
static const slideridgrp_t idsForParamNum[] =
{
   { IDC_SLIDER1, IDC_LABEL_SETTING1, IDC_LABEL_FX1LO, IDC_LABEL_FX1HI },
   { IDC_SLIDER2, IDC_LABEL_SETTING2, IDC_LABEL_FX2LO, IDC_LABEL_FX2HI },
   { IDC_SLIDER3, IDC_LABEL_SETTING3, IDC_LABEL_FX3LO, IDC_LABEL_FX3HI }
};

//
// Effect dispatch table
//
ScanManagerEffectDlg::fxfunc_t ScanManagerEffectDlg::FxFuncs[FXTYPE_MAX] =
{
   &ScanManagerEffectDlg::doSharpen,                // FXTYPE_SHARPEN
   &ScanManagerEffectDlg::doTint,                   // FXTYPE_TINT
   &ScanManagerEffectDlg::doBrightnessContrast,     // FXTYPE_BRIGHTNESS
   &ScanManagerEffectDlg::doHueSaturationLightness, // FXTYPE_HSL
   &ScanManagerEffectDlg::doColorBalance            // FXTYPE_BALANCE
};

//
// Constructor
// 
ScanManagerEffectDlg::ScanManagerEffectDlg(fxtype_e effectType, ImageNode *pImg)
   : m_hDialog(nullptr), m_effectType(effectType), m_pImageNode(pImg), m_bConfirmed(false)
{
   // create a backup of the GDI+ bitmap for the image node
   m_pImageNode->backup();
}

//
// Destructor
//
ScanManagerEffectDlg::~ScanManagerEffectDlg()
{
   if(m_pImageNode)
   {
      // if the user didn't confirm, restore from backup
      if(!m_bConfirmed)
         m_pImageNode->restoreBackup();

      // update the parent form either way.
      ScanMgr_SetCurrentImage(m_pImageNode);
   }
}

//
// Apply sharpen effect
//
bool ScanManagerEffectDlg::doSharpen()
{
   Gdiplus::SharpenParams params;
   params.radius = float(SendDlgItemMessage(m_hDialog, idsForParamNum[0].sliderID, TBM_GETPOS, 0, 0));
   params.amount = float(SendDlgItemMessage(m_hDialog, idsForParamNum[1].sliderID, TBM_GETPOS, 0, 0));

   Gdiplus::Sharpen fx;
   fx.SetParameters(&params);
   return (m_pImageNode->gdiBitmap->ApplyEffect(&fx, nullptr) == Gdiplus::Ok);
}

//
// Apply tint effect
//
bool ScanManagerEffectDlg::doTint()
{
   Gdiplus::TintParams params;
   params.hue    = INT(SendDlgItemMessage(m_hDialog, idsForParamNum[0].sliderID, TBM_GETPOS, 0, 0));
   params.amount = INT(SendDlgItemMessage(m_hDialog, idsForParamNum[1].sliderID, TBM_GETPOS, 0, 0));

   Gdiplus::Tint fx;
   fx.SetParameters(&params);
   return (m_pImageNode->gdiBitmap->ApplyEffect(&fx, nullptr) == Gdiplus::Ok);
}

//
// Apply brightness/contrast effect
//
bool ScanManagerEffectDlg::doBrightnessContrast()
{
   Gdiplus::BrightnessContrastParams params;
   params.brightnessLevel = INT(SendDlgItemMessage(m_hDialog, idsForParamNum[0].sliderID, TBM_GETPOS, 0, 0));
   params.contrastLevel   = INT(SendDlgItemMessage(m_hDialog, idsForParamNum[1].sliderID, TBM_GETPOS, 0, 0));

   Gdiplus::BrightnessContrast fx;
   fx.SetParameters(&params);
   return (m_pImageNode->gdiBitmap->ApplyEffect(&fx, nullptr) == Gdiplus::Ok);
}

//
// Apply HSL effect
//
bool ScanManagerEffectDlg::doHueSaturationLightness()
{
   Gdiplus::HueSaturationLightnessParams params;
   params.hueLevel        = INT(SendDlgItemMessage(m_hDialog, idsForParamNum[0].sliderID, TBM_GETPOS, 0, 0));
   params.saturationLevel = INT(SendDlgItemMessage(m_hDialog, idsForParamNum[1].sliderID, TBM_GETPOS, 0, 0));
   params.lightnessLevel  = INT(SendDlgItemMessage(m_hDialog, idsForParamNum[2].sliderID, TBM_GETPOS, 0, 0));

   Gdiplus::HueSaturationLightness fx;
   fx.SetParameters(&params);
   return (m_pImageNode->gdiBitmap->ApplyEffect(&fx, nullptr) == Gdiplus::Ok);
}

//
// Apply color balance effect
//
bool ScanManagerEffectDlg::doColorBalance()
{
   Gdiplus::ColorBalanceParams params;
   params.cyanRed      = INT(SendDlgItemMessage(m_hDialog, idsForParamNum[0].sliderID, TBM_GETPOS, 0, 0));
   params.magentaGreen = INT(SendDlgItemMessage(m_hDialog, idsForParamNum[1].sliderID, TBM_GETPOS, 0, 0));
   params.yellowBlue   = INT(SendDlgItemMessage(m_hDialog, idsForParamNum[2].sliderID, TBM_GETPOS, 0, 0));

   Gdiplus::ColorBalance fx;
   fx.SetParameters(&params);
   return (m_pImageNode->gdiBitmap->ApplyEffect(&fx, nullptr) == Gdiplus::Ok);
}

//
// Apply the GDI+ image effect to the image
//
bool ScanManagerEffectDlg::applyEffect()
{
   if(!m_pImageNode || !m_pImageNode->gdiBitmap || m_pImageNode->gdiBitmap->GetLastStatus() != Gdiplus::Ok)
      return false;

   // restore the original backup (overwriting any preview results), and then make another one
   if(!m_pImageNode->restoreBackup())
      return false;

   m_pImageNode->backup();

   if(!m_pImageNode->gdiBitmap || m_pImageNode->gdiBitmap->GetLastStatus() != Gdiplus::Ok)
      return false;

   bool res;
   if((res = (this->*(FxFuncs[m_effectType]))()))
      ScanMgr_SetCurrentImage(m_pImageNode);
   
   return res;
}

//
// Static method to act as dialog procedure
//
INT_PTR CALLBACK ScanManagerEffectDlg::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   auto pEffectDlg = reinterpret_cast<ScanManagerEffectDlg *>(GetWindowLongPtr(hwndDlg, GWLP_USERDATA));
   
   switch(uMsg)
   {
   case WM_INITDIALOG:
#if 0
      // This centers the dialog, but, we don't really want to do it as it puts it right in front of the image
      {
         RECT rcOwner, rcDlg, rc;
         HWND hwndOwner;
         if((hwndOwner = GetParent(hwndDlg)) == nullptr)
         {
            hwndOwner = GetDesktopWindow();
         }

         GetWindowRect(hwndOwner, &rcOwner);
         GetWindowRect(hwndDlg,   &rcDlg);
         CopyRect(&rc, &rcOwner);

         // offset rectangles
         OffsetRect(&rcDlg, -rcDlg.left,  -rcDlg.top);
         OffsetRect(&rc,    -rc.left,     -rc.top);
         OffsetRect(&rc,    -rcDlg.right, -rcDlg.bottom);

         // set centered position
         SetWindowPos(hwndDlg, HWND_TOP, rcOwner.left + (rc.right / 2), rcOwner.top + (rc.bottom / 2), 0, 0, SWP_NOSIZE);
      }
#endif
      return TRUE; // set focus to default control
   case WM_SHOWWINDOW:
      if(pEffectDlg)
      {
         // setup the dialog for the current effect
         const fxparams_t &params = fxParams[pEffectDlg->m_effectType];

         SetDlgItemText(pEffectDlg->m_hDialog, IDC_LABEL_SETTINGNAME, params.szName);
         for(int i = 0; i < params.numParams; i++)
         {
            const slideridgrp_t &idgrp = idsForParamNum[i];

            ShowWindow(GetDlgItem(pEffectDlg->m_hDialog, idgrp.fieldLblID), SW_SHOW);
            ShowWindow(GetDlgItem(pEffectDlg->m_hDialog, idgrp.sliderID  ), SW_SHOW);
            ShowWindow(GetDlgItem(pEffectDlg->m_hDialog, idgrp.minLblID  ), SW_SHOW);
            ShowWindow(GetDlgItem(pEffectDlg->m_hDialog, idgrp.maxLblID  ), SW_SHOW);

            SetDlgItemText(pEffectDlg->m_hDialog, idgrp.fieldLblID, params.szParamNames[i]);
            if(params.paramType == FXPARAMS_REAL)
            {
               // setup for floating-point parameters
               float step = (params.paramsf[i][FXP_MAX] - params.paramsf[i][FXP_MIN]) / 10.0f;
               SendDlgItemMessage(pEffectDlg->m_hDialog, idgrp.sliderID, TBM_SETRANGEMIN, 1, int(params.paramsf[i][FXP_MIN    ]));
               SendDlgItemMessage(pEffectDlg->m_hDialog, idgrp.sliderID, TBM_SETRANGEMAX, 1, int(params.paramsf[i][FXP_MAX    ]));
               SendDlgItemMessage(pEffectDlg->m_hDialog, idgrp.sliderID, TBM_SETPOS,      1, int(params.paramsf[i][FXP_DEFAULT]));
               SendDlgItemMessage(pEffectDlg->m_hDialog, idgrp.sliderID, TBM_SETTICFREQ,  int(step), 0);
               SetDlgItemInt(pEffectDlg->m_hDialog, idgrp.minLblID, (unsigned int)(params.paramsf[i][FXP_MIN]), TRUE);
               SetDlgItemInt(pEffectDlg->m_hDialog, idgrp.maxLblID, (unsigned int)(params.paramsf[i][FXP_MAX]), TRUE);
            }
            else
            {
               // setup for integer parameters
               int step = (params.paramsi[i][FXP_MAX] * 2) / 10;
               SendDlgItemMessage(pEffectDlg->m_hDialog, idgrp.sliderID, TBM_SETRANGEMIN, 1, params.paramsi[i][FXP_MIN    ]);
               SendDlgItemMessage(pEffectDlg->m_hDialog, idgrp.sliderID, TBM_SETRANGEMAX, 1, params.paramsi[i][FXP_MAX    ]);
               SendDlgItemMessage(pEffectDlg->m_hDialog, idgrp.sliderID, TBM_SETPOS,      1, params.paramsi[i][FXP_DEFAULT]);
               SendDlgItemMessage(pEffectDlg->m_hDialog, idgrp.sliderID, TBM_SETTICFREQ,  step, 0);
               SetDlgItemInt(pEffectDlg->m_hDialog, idgrp.minLblID, params.paramsi[i][FXP_MIN], TRUE);
               SetDlgItemInt(pEffectDlg->m_hDialog, idgrp.maxLblID, params.paramsi[i][FXP_MAX], TRUE);
            }
         }
      }
      return TRUE; 
   case WM_COMMAND:
      switch(LOWORD(wParam))
      {
      case IDOK:
         pEffectDlg->m_bConfirmed = true; // remember that the user clicked OK
         if(!pEffectDlg->applyEffect())
            MessageBox(GetParent(hwndDlg), L"Failed to apply effect", L"Scan Manager", MB_ICONERROR | MB_OK);
         // Fall-through
      case IDCANCEL:
         DestroyWindow(pEffectDlg->m_hDialog);
         pEffectDlg->m_hDialog = nullptr;
         return TRUE;
      case IDC_BUTTON_PREVIEW:
         // apply effect to the current image
         if(!pEffectDlg->applyEffect())
            MessageBox(GetParent(hwndDlg), L"Failed to apply effect", L"Scan Manager", MB_ICONERROR | MB_OK);
         return TRUE;
      }
      break;
   default:
      break;
   }

   return FALSE;
}

//
// Call to create the dialog
//
bool ScanManagerEffectDlg::create(HWND hWndParent)
{
   if(IsWindow(m_hDialog))
      return false; // already has a dialog

   m_hDialog = CreateDialog(nullptr, MAKEINTRESOURCE(IDD_DIALOG1), hWndParent, DialogProc);

   if(IsWindow(m_hDialog))
   {
      SetWindowLongPtr(m_hDialog, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
      ShowWindow(m_hDialog, SW_SHOW);
      return true;
   }
   else
      return false;
}

// EOF

