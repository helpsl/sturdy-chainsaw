#pragma once
#include <usbhid.h>
#define EP_MAXPKTSIZE       128
#define SM_CONTROL_PIPE    0
#define SM_INPUT_PIPE      1
#define SM_MAX_ENDPOINTS   2

class InputDevice
  : public USBDeviceConfig
{
    uint8_t readBuf[EP_MAXPKTSIZE];
    USB *pUsb;
    uint8_t bAddress;
    EpInfo epInfo[SM_MAX_ENDPOINTS];
    bool bPollEnable;
  public:
    InputDevice(USB *pUsb);
    uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed) override;
    uint8_t Poll() override;
    bool VIDPIDOK(uint16_t vid, uint16_t pid) override;

    void init();
    void task();
};

extern InputDevice input;
