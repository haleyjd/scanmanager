/*
  Scan Manager

  TWAIN scanning module for image acquisition
*/

#ifndef SCANNING_H__
#define SCANNING_H__

//
// The TWAINManager object provides TWAIN scanning services.
//
class TWAINManager
{
protected:
   bool openSource();
   bool closeSource();
   bool checkCapability(HWND hWnd, short &count);
   bool negotiateCaps(HWND hWnd);
   bool enableSource(HWND hWnd);
   bool disableSource();
   bool updateImageInfo(void *pvImgInfo);
   void doAbortXfer();
   void setupAndXferImage(void (*cb)(HBITMAP));

public:
   bool loadSourceManager();
   bool openSourceManager(HWND hWnd);
   bool selectSource();
   bool doAcquire(HWND hWnd);
   bool TWAINCheckEvent(MSG &msg, void (*cb)(HBITMAP));
   void shutdown(HWND hWnd);
};

#endif

// EOF
