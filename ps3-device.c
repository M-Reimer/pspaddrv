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

#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>
#include "usb.h"
#include "uinput.h"
#include "ps3-device.h"

#define SIXAXIS_REPORT_0xF2_SIZE 17
#define SIXAXIS_ENDPOINT_IN 1 | LIBUSB_ENDPOINT_IN

struct Playstation3USBMsg {
  unsigned int unknown00 :8; // always 01
  unsigned int unknown01 :8; // always 00

  // 02
  unsigned int btn_select  :1;
  unsigned int btn_l3      :1;
  unsigned int btn_r3      :1;
  unsigned int btn_start   :1;

  unsigned int btn_dpad_up    :1;
  unsigned int btn_dpad_right :1;
  unsigned int btn_dpad_down  :1;
  unsigned int btn_dpad_left  :1;

  // 03
  unsigned int btn_l2   :1;
  unsigned int btn_r2   :1;
  unsigned int btn_l1   :1;
  unsigned int btn_r1   :1;

  unsigned int btn_triangle :1;
  unsigned int btn_circle   :1;
  unsigned int btn_cross    :1;
  unsigned int btn_square   :1;

  // 04
  unsigned int btn_playstation :1;
  unsigned int unknown04   :7;
  unsigned int unknown05 :8; // always 00

  unsigned int abs_lx :8;
  unsigned int abs_ly :8;
  unsigned int abs_rx :8;
  unsigned int abs_ry :8;

  unsigned int unknown10 :8; // always 00
  unsigned int unknown11 :8; // always 00
  unsigned int unknown12 :8; // always 00
  unsigned int unknown13 :8; // always 00

  unsigned int a_dpad_up    :8;

  unsigned int a_dpad_right :8;
  unsigned int a_dpad_down  :8;
  unsigned int a_dpad_left  :8;

  unsigned int abs_l2 :8;
  unsigned int abs_r2 :8;
  unsigned int a_l1 :8;
  unsigned int a_r1 :8;

  unsigned int a_triangle :8;
  unsigned int a_circle   :8;
  unsigned int a_cross    :8;

  unsigned int a_square   :8;

  unsigned int unknown26 :8; // always 00
  unsigned int unknown27 :8; // always 00
  unsigned int unknown28 :8; // always 00

  // Bluetooth id start (or something like that)
  unsigned int unknown29 :8;
  unsigned int unknown30 :8;
  unsigned int unknown31 :8;
  unsigned int unknown32 :8;
  unsigned int unknown33 :8;
  unsigned int unknown34 :8;

  unsigned int unknown35 :8;
  unsigned int unknown36 :8;
  unsigned int unknown37 :8;
  unsigned int unknown38 :8;
  unsigned int unknown39 :8;
  unsigned int unknown40 :8;
  // Bluetooth id end

  unsigned int accl_x :16; // little vs big endian!?!
  unsigned int accl_y :16; // little vs big endian!?!
  unsigned int accl_z :16; // little vs big endian!?!

  unsigned int rot_z :16; // very low res (3 or 4 bits), neutral at 5 or 6
} __attribute__((__packed__));


/*
 * Sending HID_REQ_GET_REPORT changes the operation mode of the ps3 controller
 * to "operational".  Without this, the ps3 controller will not report any
 * events.
 */
int PS3SetOperationalUSB(libusb_device_handle *usbdev) {
  int   ret, i;
  unsigned char  buf[17];

  printf("    REQUEST: Get_Report >> 0x%04x\n", (HID_FEATURE_REPORT<<8)|0xf2);

  ret = libusb_control_transfer(usbdev,
                        LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
                        HID_REQ_GET_REPORT,
                        (HID_FEATURE_REPORT<<8)|0xf2,
                        0,
                        buf,
                        SIXAXIS_REPORT_0xF2_SIZE,
                        USB_CTRL_GET_TIMEOUT);

  if (ret == 0) {
    printf("    SUCCESS: Data <<");
    for (i = 0; i < 17; i++) {
      printf(" %02x", buf[i] & 0xff);
    }
    printf("\n");
  }

  return ret;
}

int PS3PollInputUSB(libusb_device_handle *usbdev, struct XpadMsg *msg_out) {
  struct Playstation3USBMsg ps3msg;
  int transferred;
  int ret = libusb_interrupt_transfer(usbdev,
                                      SIXAXIS_ENDPOINT_IN,
                                      (unsigned char*)&ps3msg, sizeof(ps3msg),
                                      &transferred, USB_CTRL_GET_TIMEOUT);
  if (ret < 0) {
    // Timeout --> User still has to press PS button
    if (ret == LIBUSB_ERROR_TIMEOUT)
      return 0;

    return ret;
  }

  msg_out->btn_a = ps3msg.btn_cross;
  msg_out->btn_b = ps3msg.btn_circle;
  msg_out->btn_x = ps3msg.btn_square;
  msg_out->btn_y = ps3msg.btn_triangle;
  msg_out->btn_start = ps3msg.btn_start;
  msg_out->btn_select = ps3msg.btn_select;
  msg_out->btn_guide = ps3msg.btn_playstation;
  msg_out->btn_ls = ps3msg.btn_l3;
  msg_out->btn_rs = ps3msg.btn_r3;
  msg_out->btn_lb = ps3msg.btn_l1;
  msg_out->btn_rb = ps3msg.btn_r1;
  msg_out->abs_lt = ps3msg.abs_l2;
  msg_out->abs_rt = ps3msg.abs_r2;
  msg_out->abs_lx = ps3msg.abs_lx;
  msg_out->abs_ly = ps3msg.abs_ly;
  msg_out->abs_rx = ps3msg.abs_rx;
  msg_out->abs_ry = ps3msg.abs_ry;

  if (ps3msg.btn_dpad_up)
    msg_out->abs_dy = -1;
  else if (ps3msg.btn_dpad_down)
    msg_out->abs_dy = 1;
  else
    msg_out->abs_dy = 0;
  if (ps3msg.btn_dpad_left)
    msg_out->abs_dx = -1;
  else if (ps3msg.btn_dpad_right)
    msg_out->abs_dx = 1;
  else
    msg_out->abs_dx = 0;

  return 0;
}

int PS3SendRumbleUSB(libusb_device_handle *usbdev, int weak, int strong) {
  uint8_t left = strong / 256;
  uint8_t right = weak ? 1 : 0;

  uint8_t cmd[] = {
    0x00, 254, right, 254, left,  // rumble values
    0x00, 0x00, 0x00, 0x00, 0x03,   // 0x10=LED1 .. 0x02=LED4
    0xff, 0x27, 0x10, 0x00, 0x32,   // LED 4
    0xff, 0x27, 0x10, 0x00, 0x32,   // LED 3
    0xff, 0x27, 0x10, 0x00, 0x32,   // LED 2
    0xff, 0x27, 0x10, 0x00, 0x32,   // LED 1
    0x00, 0x00, 0x00, 0x00, 0x00
  };

  return libusb_control_transfer(usbdev,
                        LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
                        HID_REQ_SET_REPORT,
                        (HID_OUTPUT_REPORT<<8)|0x01,
                        0,
                        cmd,
                        sizeof(cmd),
                        USB_CTRL_GET_TIMEOUT);
}
