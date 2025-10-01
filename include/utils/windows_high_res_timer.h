#pragma once

#ifdef _WIN32
  #include <Windows.h>
  #include <mmsystem.h>
  #pragma comment(lib, "winmm.lib")
  
  class WindowsHighResTimer {
      public:
      explicit WindowsHighResTimer(unsigned ms = 1) {
          resolution_ms_ = ms;
          if (timeBeginPeriod(resolution_ms_) == TIMERR_NOERROR) {
              active_ = true;
            }
        }
        
        ~WindowsHighResTimer() {
            if (active_) {
                timeEndPeriod(resolution_ms_);
            }
        }
        
        WindowsHighResTimer(const WindowsHighResTimer&) = delete;
        WindowsHighResTimer& operator=(const WindowsHighResTimer&) = delete;
        
        private:
UINT resolution_ms_{1};
bool active_{false};

};
#endif