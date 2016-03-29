/*
  pspaddrv - Usermode Playstation 3/4 to XBox 360 gamepad driver
  Copyright (C) 2016  Manuel Reimer <manuel.reimer@gmx.de>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <libusb.h>

#define HID_INPUT_REPORT    0x01
#define HID_OUTPUT_REPORT   0x02
#define HID_FEATURE_REPORT  0x03

#define HID_REQ_GET_REPORT      0x01
#define HID_REQ_SET_REPORT      0x09

#define USB_CTRL_GET_TIMEOUT    5000 // Timeout for libusb requests

// Constants for "devtype" in "USBDeviceHandlerArgs"
#define PS3_DEVICE 1
#define PS4_DEVICE 2

struct USBDeviceHandlerArgs {
  int busnum;
  int devnum;
  int devtype;
};

int USBOpenDevice(struct USBDeviceHandlerArgs* args, libusb_device_handle** handle);
