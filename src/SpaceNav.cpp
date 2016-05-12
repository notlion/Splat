#include "SpaceNav.hpp"

#include "cinder/Log.h"

#include <windows.h>
#include <stdexcept>

#include "si.h"
#include "siapp.h"

void SpaceNav::init(const ci::app::Window &window) {
  if (SiInitialize() == SPW_ERROR) {
    throw std::runtime_error("Error initializing 3DxWare Input Library");
  }

  SiOpenData openData;

#if defined(CINDER_MSW)
  SiOpenWinInit(&openData, static_cast<HWND>(window.getNative()));
#endif

  SiSetUiMode(SI_ALL_HANDLES, SI_UI_NO_CONTROLS);

  deviceHandle = SiOpen("SplatTest", SI_ANY_DEVICE, SI_NO_MASK, SI_EVENT, &openData);
  if (deviceHandle == nullptr) {
    SiTerminate();
    throw std::runtime_error("Error opening device");
  }
}

void SpaceNav::poll() {
  if (SiIsInitialized() == SPW_TRUE) {
  }
}