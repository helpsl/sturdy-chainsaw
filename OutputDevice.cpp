#include "OutputDevice.h"
#include "DeviceState.h"
#include <SPI.h>
#include "HID.h"

static const uint8_t _hidReportDescriptor[] PROGMEM = {
  /* Gamepad with 16 buttons and 6 axis*/
  0x05, 0x01,         /* USAGE_PAGE (Generic Desktop) */
  0x09, 0x05,         /* USAGE (Game Pad) */
  0xa1, 0x01,         /* COLLECTION (Application) */
  /* 16 Buttons */
  0x05, 0x09,         /*   USAGE_PAGE (Button) */
  0x19, 0x01,         /*   USAGE_MINIMUM (Button 1) */
  0x29, 0x10,         /*   USAGE_MAXIMUM (Button 16) */
  0x15, 0x00,         /*   LOGICAL_MINIMUM (0) */
  0x25, 0x01,         /*   LOGICAL_MAXIMUM (1) */
  0x75, 0x01,         /*   REPORT_SIZE (1) */
  0x95, 0x10,         /*   REPORT_COUNT (16) */
  0x81, 0x02,         /*   INPUT (Data,Var,Abs) */
  /* 6 16bit Axis */
  0x05, 0x01,         /*   USAGE_PAGE (Generic Desktop) */
  0xa1, 0x00,         /*   COLLECTION (Physical) */
  0x09, 0x30,         /*     USAGE (X) */
  0x09, 0x31,         /*     USAGE (Y) */
  0x09, 0x32,         /*     USAGE (Z) */
  0x09, 0x33,         /*     USAGE (Rx) */
  0x09, 0x34,         /*     USAGE (Ry) */
  0x09, 0x35,         /*     USAGE (Rz) */
  0x16, 0x00, 0x80,   /*     LOGICAL_MINIMUM (-32768) */
  0x26, 0xFF, 0x7F,   /*     LOGICAL_MAXIMUM (32767) */
  0x75, 0x10,         /*     REPORT_SIZE (16) */
  0x95, 0x06,         /*     REPORT_COUNT (6) */
  0x81, 0x02,         /*     INPUT (Data,Var,Abs) */
  0xc0,               /*   END_COLLECTION */
  0xc0                /* END_COLLECTION */
};

class Gamepad : public PluggableUSBModule
{
  public:
    Gamepad(void) : PluggableUSBModule(1, 1, &epType)
    {
      epType = EP_TYPE_INTERRUPT_IN;
      PluggableUSB().plug(this);
    }

    void sendState() {
      USB_Send(pluggedEndpoint | TRANSFER_RELEASE, &state, sizeof(state));
    }
  protected:
    // Implementation of the PUSBListNode
    int getInterface(uint8_t* interfaceCount)
    {
      *interfaceCount += 1; // uses 1
      HIDDescriptor hidInterface = {
        D_INTERFACE(pluggedInterface, 1, USB_DEVICE_CLASS_HUMAN_INTERFACE, HID_SUBCLASS_NONE, HID_PROTOCOL_NONE),
        D_HIDREPORT(sizeof(_hidReportDescriptor)),
        D_ENDPOINT(USB_ENDPOINT_IN(pluggedEndpoint), USB_ENDPOINT_TYPE_INTERRUPT, USB_EP_SIZE, 0x01)
      };
      return USB_SendControl(0, &hidInterface, sizeof(hidInterface));
    }
    int getDescriptor(USBSetup& setup)
    {
      // Check if this is a HID Class Descriptor request
      if (setup.bmRequestType != REQUEST_DEVICETOHOST_STANDARD_INTERFACE) return 0;
      if (setup.wValueH != HID_REPORT_DESCRIPTOR_TYPE) return 0;
      // In a HID Class Descriptor wIndex cointains the interface number
      if (setup.wIndex != pluggedInterface) return 0;

      return USB_SendControl(TRANSFER_PGM, _hidReportDescriptor, sizeof(_hidReportDescriptor));
    }
    bool setup(USBSetup& setup);

    uint8_t epType;
};

bool Gamepad::setup(USBSetup& setup)
{
  if (pluggedInterface != setup.wIndex)
    return false;

  uint8_t request = setup.bRequest;
  uint8_t requestType = setup.bmRequestType;

  if (requestType == REQUEST_DEVICETOHOST_CLASS_INTERFACE) {
    if (request == HID_GET_REPORT) {
      Serial.println("HID_GET_REPORT");
      return true;
    }
    if (request == HID_GET_PROTOCOL) {
      Serial.println("HID_GET_PROTOCOL");
      return true;
    }
  }

  if (requestType == REQUEST_HOSTTODEVICE_CLASS_INTERFACE) {
    if (request == HID_SET_PROTOCOL) {
      Serial.print("HID_SET_PROTOCOL ");
      Serial.println(setup.wValueL);
      return true;
    }
    if (request == HID_SET_IDLE) {
      Serial.print("HID_SET_IDLE ");
      Serial.println(setup.wValueL);
      return true;
    }
    if (request == HID_SET_REPORT)
    {
      Serial.print("HID_SET_REPORT ");
      Serial.println(setup.wValueL);
    }
  }

  return false;
}

Gamepad gamepad;

void OutputDevice::Init() {
  gamepad.sendState();

  // Setup HID report structure
  //static HIDSubDescriptor node(_hidReportDescriptor, sizeof(_hidReportDescriptor));
  //HID().AppendDescriptor(&node);
}

void OutputDevice::SendState() {
  gamepad.sendState();

  //HID().SendReport(HID_REPORT_PROTOCOL | TRANSFER_RELEASE, &state, sizeof(state));
}
