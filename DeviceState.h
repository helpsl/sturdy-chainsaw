#pragma once
#include <stdint.h>

struct DeviceState {
  uint16_t buttons;
  int16_t xAxis;
  int16_t yAxis;
  int16_t zAxis;
  int16_t rxAxis;
  int16_t ryAxis;
  int16_t rzAxis;
};

extern DeviceState state;
