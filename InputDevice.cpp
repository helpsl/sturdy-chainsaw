#include "InputDevice.h"
#include "DeviceState.h"
#include <usbhub.h>

InputDevice::InputDevice(USB *p)
  : pUsb(p) // pointer to USB class instance - mandatory
  , bAddress(0) // device address - mandatory
  , bPollEnable(false) // don't start polling before dongle is connected
{
  for (uint8_t i = 0; i < SM_MAX_ENDPOINTS; i++) {
    epInfo[i].epAddr = 0;
    epInfo[i].maxPktSize = (i) ? 0 : 8;
    epInfo[i].bmSndToggle = 0;
    epInfo[i].bmRcvToggle = 0;
    epInfo[i].bmNakPower = (i) ? USB_NAK_NOWAIT : USB_NAK_MAX_POWER;
  }

  if (pUsb) // register in USB subsystem
    pUsb->RegisterDeviceClass(this); //set devConfig[] entry
}


uint8_t InputDevice::Init(uint8_t parent, uint8_t port, bool lowspeed) {
  uint8_t buf[sizeof (USB_DEVICE_DESCRIPTOR)];
  USB_DEVICE_DESCRIPTOR * udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR*>(buf);
  uint8_t rcode;
  UsbDevice *p = NULL;
  EpInfo *oldep_ptr = NULL;

  // get memory address of USB device address pool
  AddressPool &addrPool = pUsb->GetAddressPool();

  // check if address has already been assigned to an instance
  if (bAddress)
    return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;

  // Get pointer to pseudo device with address 0 assigned
  p = addrPool.GetUsbDevicePtr(0);

  if (!p)
    return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

  if (!p->epinfo)
    return USB_ERROR_EPINFO_IS_NULL;

  // Save old pointer to EP_RECORD of address 0
  oldep_ptr = p->epinfo;

  // Temporary assign new pointer to epInfo to p->epinfo in order to avoid toggle inconsistence
  p->epinfo = epInfo;

  p->lowspeed = lowspeed;

  // Get device descriptor
  rcode = pUsb->getDevDescr(0, 0, sizeof (USB_DEVICE_DESCRIPTOR), (uint8_t*)buf); // Get device descriptor - addr, ep, nbytes, data
  // Restore p->epinfo
  p->epinfo = oldep_ptr;

  if (rcode)
    goto FailGetDevDescr;

  if (!VIDPIDOK(udd->idVendor, udd->idProduct))
    goto FailUnknownDevice;

  // Allocate new address according to device class
  bAddress = addrPool.AllocAddress(parent, false, port);

  if (!bAddress)
    return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;

  // Extract Max Packet Size from device descriptor
  epInfo[0].maxPktSize = udd->bMaxPacketSize0;

  // Assign new address to the device
  rcode = pUsb->setAddr(0, 0, bAddress);
  if (rcode) {
    p->lowspeed = false;
    addrPool.FreeAddress(bAddress);
    bAddress = 0;
    return rcode;
  }
  //delay(300); // Spec says you should wait at least 200ms

  p->lowspeed = false;

  //get pointer to assigned address record
  p = addrPool.GetUsbDevicePtr(bAddress);
  if (!p)
    return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

  p->lowspeed = lowspeed;

  // Assign epInfo to epinfo pointer - only EP0 is known
  rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);
  if (rcode)
    goto FailSetDevTblEntry;

  /* The application will work in reduced host mode, so we can save program and data
     memory space. After verifying the VID we will use known values for the
     configuration values for device, interface, endpoints and HID for the XBOX360 Controllers */

  /* Initialize data structures for endpoints of device */
  epInfo[ SM_INPUT_PIPE ].epAddr = 0x01; // XBOX 360 report endpoint
  epInfo[ SM_INPUT_PIPE ].epAttribs = USB_TRANSFER_TYPE_INTERRUPT;
  epInfo[ SM_INPUT_PIPE ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
  epInfo[ SM_INPUT_PIPE ].maxPktSize = EP_MAXPKTSIZE;
  epInfo[ SM_INPUT_PIPE ].bmSndToggle = 0;
  epInfo[ SM_INPUT_PIPE ].bmRcvToggle = 0;

  rcode = pUsb->setEpInfoEntry(bAddress, SM_MAX_ENDPOINTS, epInfo);
  if (rcode)
    goto FailSetDevTblEntry;

  delay(200); // Give time for address change

  rcode = pUsb->setConf(bAddress, epInfo[ SM_CONTROL_PIPE ].epAddr, 1);
  if (rcode)
    goto FailSetConfDescr;

  bPollEnable = true;
  return 0; // Successful configuration

  /* Diagnostic messages */
FailGetDevDescr:
#ifdef DEBUG_USB_HOST
  NotifyFailGetDevDescr();
  goto Fail;
#endif

FailSetDevTblEntry:
#ifdef DEBUG_USB_HOST
  NotifyFailSetDevTblEntry();
  goto Fail;
#endif

FailSetConfDescr:
#ifdef DEBUG_USB_HOST
  NotifyFailSetConfDescr();
#endif
  goto Fail;

FailUnknownDevice:
#ifdef DEBUG_USB_HOST
  NotifyFailUnknownDevice(udd->idVendor, udd->idProduct);
#endif
  rcode = USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;

Fail:
#ifdef DEBUG_USB_HOST
  Notify(PSTR("\r\nXbox 360 Init Failed, error code: "), 0x80);
  NotifyFail(rcode);
#endif
  Release();
  return rcode;
}

int16_t processAxis(int32_t v) {
  return constrain(v * 127, -32768, 32767);
}

uint8_t InputDevice::Poll() {
  //if (!bPollEnable)
  //  return 0;
  uint16_t BUFFER_SIZE = EP_MAXPKTSIZE;
  for (int i = 0; i < SM_MAX_ENDPOINTS; i++) {



    BUFFER_SIZE = EP_MAXPKTSIZE;
    uint32_t len = pUsb->inTransfer(bAddress, epInfo[ i ].epAddr, &BUFFER_SIZE, readBuf);
    if (len < 3) {
      continue;
    }
    
//    Serial.print("Polling pipe ");
//    Serial.print(i);
//    Serial.print(" length ");
//    Serial.print(len);
//    Serial.print(" ");

    const int16_t* evt = (const int16_t*)(readBuf + 1);

    switch (readBuf[0]) {
      case 1: // Translation vector
        state.xAxis = processAxis(evt[0]);
        state.yAxis = processAxis(evt[1]);
        state.zAxis = processAxis(evt[2]);
        break;
      case 2: // Rotation vector
        state.rxAxis = processAxis(evt[0]);
        state.ryAxis = processAxis(evt[1]);
        state.rzAxis = processAxis(evt[2]);
        break;
      case 3: // Buttons
        state.buttons = evt[0];
        break;
      default:
        Serial.print("Unknown event recieved ");
        Serial.print(readBuf[0]);
        Serial.print(" [");
        for (int i = 1; i < len; i++) {
          Serial.print(" ");
          Serial.print(readBuf[i]);
        }
        Serial.println("]");
        continue;
    }

//    Serial.print(F("\tT: ["));
//    Serial.print(state.xAxis);
//    Serial.print(F(", "));
//    Serial.print(state.yAxis);
//    Serial.print(F(", "));
//    Serial.print(state.zAxis);
//    Serial.print(F("]"));
//    Serial.print(F("\tR: ["));
//    Serial.print(state.rxAxis);
//    Serial.print(F(", "));
//    Serial.print(state.ryAxis);
//    Serial.print(F(", "));
//    Serial.print(state.rzAxis);
//    Serial.print(F("]"));
//    Serial.print(F("\tBtn: "));
//    Serial.print(state.buttons, HEX);
//    Serial.println("");
  }

  return 0;
}

bool InputDevice::VIDPIDOK(uint16_t vid, uint16_t pid) {
  Serial.print("Testing VID[");
  Serial.print(vid, HEX);
  Serial.print("] ");
  Serial.print("PID[");
  Serial.print(pid, HEX);
  Serial.println("]");
  return vid == 0x046D && pid == 0xC627;
};


USB   Usb;
USBHub Hub(&Usb);
InputDevice input(&Usb);

void InputDevice::init() {
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while (1); //halt
  }
}
void InputDevice::task() {
  Usb.Task();
}
