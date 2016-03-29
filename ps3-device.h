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

int PS3SetOperationalUSB(libusb_device_handle *usbdev);
int PS3PollInputUSB(libusb_device_handle *usbdev, struct XpadMsg *msg_out);
int PS3SendRumbleUSB(libusb_device_handle *usbdev, int weak, int strong);
