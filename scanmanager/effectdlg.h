//
// ScanManager
//
// Image effects dialog class
//

#ifndef EFFECTDLG_H__
#define EFFECTDLG_H__

#include <Windows.h>

class ImageNode;

// Types of GDI+ effects supported
enum fxtype_e
{
   FXTYPE_SHARPEN,
   FXTYPE_TINT,
   FXTYPE_BRIGHTNESS,
   FXTYPE_HSL,
   FXTYPE_BALANCE,
   FXTYPE_MAX
};

// Parmameter type for effects
enum fxparamtype_e
{
   FXPARAMS_INT, // uses int params
   FXPARAMS_REAL // uses float params
};

enum fxparamidx_e
{
   FXP_MIN,
   FXP_MAX,
   FXP_DEFAULT
};

// Generic effect type structure, contains all fields needed for the different
// GDI+ effects.
struct fxparams_t
{
   LPCTSTR       szName;          // name of the effect
   fxtype_e      effectType;      // what effect type this applies to
   fxparamtype_e paramType;       // which data type to use for params
   int           numParams;       // number of valid parameters
   LPCTSTR       szParamNames[3]; // param names
   INT           paramsi[3][3];   // min, max, default
   float         paramsf[3][3];   // min, max, default
};

//
// Effect Dialog Class
//
class ScanManagerEffectDlg
{
protected:
   HWND       m_hDialog;
   fxtype_e   m_effectType;
   ImageNode *m_pImageNode;
   bool       m_bConfirmed;

   static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

   bool applyEffect();

   bool doSharpen();
   bool doTint();
   bool doBrightnessContrast();
   bool doHueSaturationLightness();
   bool doColorBalance();

   using fxfunc_t = bool (ScanManagerEffectDlg::*)();
   static fxfunc_t FxFuncs[FXTYPE_MAX];

public:
   ScanManagerEffectDlg(fxtype_e effectType, ImageNode *pImg);
   virtual ~ScanManagerEffectDlg();

   bool create(HWND hWndParent);

   HWND     getDialogHWND() const { return m_hDialog;    }
   fxtype_e getEffectType() const { return m_effectType; }
};

#endif

// EOF

