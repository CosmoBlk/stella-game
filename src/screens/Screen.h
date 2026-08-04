#pragma once
#include <stdint.h>

class Screen {
public:
  virtual ~Screen() = default;
  virtual void enter() = 0;
  virtual void exit() {}
  virtual void update(uint32_t deltaMs) = 0;
  virtual void draw() = 0;
  virtual void onButtonA() {}
  virtual void onButtonB() {}
  virtual void onButtonC() {}
  virtual void onButtonALong() {}
  virtual void onButtonBLong() {}   // default global behaviour: App handles "back"
  virtual void onButtonCLong() {}
  // Return false if this screen consumes long-B itself (e.g. task hold-to-complete).
  virtual bool allowGlobalBack() const { return true; }
  virtual const char* name() const = 0;
};
