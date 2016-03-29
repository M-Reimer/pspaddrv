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

#include "usb.h"

// This function opens an USB device based on a USBDeviceHandlerArgs struct
// It also handles detaching the kernel driver and claiming the interface
int USBOpenDevice(struct USBDeviceHandlerArgs* args, libusb_device_handle** handle) {
  ssize_t cnt;
  libusb_device **devs;
  cnt = libusb_get_device_list(NULL, &devs);
  if (cnt < 0) {
    return cnt;
  }

  libusb_device *dev;
  int i = 0;
  int found = 0;
  while ((dev = devs[i++]) != NULL) {
    if (libusb_get_bus_number(dev) == args->busnum &&
        libusb_get_device_address(dev) == args->devnum) {
      found = 1;
      break;
    }
  }

  int ret = LIBUSB_ERROR_NO_DEVICE;
  if (found)
    ret = libusb_open(dev, handle);

  if (ret == 0) {
    libusb_detach_kernel_driver(*handle, 0);
    libusb_claim_interface(*handle, 0);
  }

  libusb_free_device_list(devs, 1);
  return ret;
}
