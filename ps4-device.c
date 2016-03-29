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
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>
#include "usb.h"
#include "uinput.h"
#include "ps4-device.h"

#define DUALSHOCK4_ENDPOINT_IN  4 | LIBUSB_ENDPOINT_IN

// http://www.psdevwiki.com/ps4/DS4-USB
struct Playstation4USBMsg {
/*00*/  unsigned int unknown00 :8;

/*01*/  unsigned int abs_lx :8;
/*02*/  unsigned int abs_ly :8;
/*03*/  unsigned int abs_rx :8;
/*04*/  unsigned int abs_ry :8;

/*05*/  unsigned int dpad_hat :4;

        unsigned int btn_square   :1;
        unsigned int btn_cross    :1;
        unsigned int btn_circle   :1;
        unsigned int btn_triangle :1;

/*06*/ unsigned int btn_l1   :1;
       unsigned int btn_r1   :1;
       unsigned int btn_l2   :1;
       unsigned int btn_r2   :1;

       unsigned int btn_share   :1;
       unsigned int btn_options :1;
       unsigned int btn_l3      :1;
       unsigned int btn_r3      :1;

/*07*/ unsigned int btn_playstation :1;
       unsigned int btn_tpad  :1;
       unsigned int unknown01 :6;

/*08*/ unsigned int abs_l2 :8;
/*09*/ unsigned int abs_r2 :8;

/*10*/ unsigned int unknown02 :8;
/*11*/ unsigned int unknown03 :8;

/*12*/ unsigned int battery_level :8;

/*13*/ unsigned int gyro_x :16;
/*15*/ unsigned int gyro_y :16;
/*17*/ unsigned int gyro_z :16;
/*19*/ unsigned int accl_x :16;
/*21*/ unsigned int accl_y :16;
/*23*/ unsigned int accl_z :16;

/*25*/ unsigned int unknown04 :8;
/*26*/ unsigned int unknown05 :8;
/*27*/ unsigned int unknown06 :8;
/*28*/ unsigned int unknown07 :8;
/*29*/ unsigned int unknown08 :8;

/*30*/ unsigned int ext_bitmask :8;

/*31*/ unsigned int unknown09 :8;
/*32*/ unsigned int unknown10 :8;

/*33*/ unsigned int tpad_event_active :4;
       unsigned int unknown11 :4;

/*34*/ unsigned int unknown12 :8;

/*35*/ unsigned int tpad_tracking_no1 :8;
/*36*/ unsigned int tpad_x1 :12;
/*38*/ unsigned int tpad_y1 :12;

/*39*/ unsigned int tpad_tracking_no2 :8;
/*40*/ unsigned int tpad_x2 :12;
/*42*/ unsigned int tpad_y2 :12;

/*43*/ unsigned int unknown13 :8; //?

/*44*/ unsigned int tpad_prev_tracking_no1 :8;
/*45*/ unsigned int tpad_prev_x1 :12;
/*47*/ unsigned int tpad_prev_y1 :12;

/*48*/ unsigned int tpad_prev_tracking_no2 :8;
/*49*/ unsigned int tpad_prev_x2 :12;
/*51*/ unsigned int tpad_prev_y2 :12;

/*52*/ unsigned int unknown14 :8;
/*53*/ unsigned int unknown15 :8;
/*54*/ unsigned int unknown16 :8;
/*55*/ unsigned int unknown17 :8;
/*56*/ unsigned int unknown18 :8;
/*57*/ unsigned int unknown19 :8;
/*58*/ unsigned int unknown20 :8;
/*59*/ unsigned int unknown21 :8;
/*60*/ unsigned int unknown22 :8;
/*61*/ unsigned int unknown23 :8;
/*62*/ unsigned int unknown24 :8;
/*63*/ unsigned int unknown25 :8;
} __attribute__((__packed__));



int PS4PollInputUSB(libusb_device_handle *usbdev, struct XpadMsg *msg_out) {
  struct Playstation4USBMsg ps4msg;
  int transferred;
  int ret = libusb_interrupt_transfer(usbdev,
                                      DUALSHOCK4_ENDPOINT_IN,
                                      (unsigned char*)&ps4msg, sizeof(ps4msg),
                                      &transferred, USB_CTRL_GET_TIMEOUT);

  if (ret < 0)
    return ret;

  msg_out->btn_a = ps4msg.btn_cross;
  msg_out->btn_b = ps4msg.btn_circle;
  msg_out->btn_x = ps4msg.btn_square;
  msg_out->btn_y = ps4msg.btn_triangle;
  msg_out->btn_start = ps4msg.btn_options;
  msg_out->btn_select = ps4msg.btn_share;
  msg_out->btn_guide = ps4msg.btn_playstation;
  msg_out->btn_ls = ps4msg.btn_l3;
  msg_out->btn_rs = ps4msg.btn_r3;
  msg_out->btn_lb = ps4msg.btn_l1;
  msg_out->btn_rb = ps4msg.btn_r1;
  msg_out->abs_lt = ps4msg.abs_l2;
  msg_out->abs_rt = ps4msg.abs_r2;
  msg_out->abs_lx = ps4msg.abs_lx;
  msg_out->abs_ly = ps4msg.abs_ly;
  msg_out->abs_rx = ps4msg.abs_rx;
  msg_out->abs_ry = ps4msg.abs_ry;

  msg_out->abs_dx = 0;
  msg_out->abs_dy = 0;
  switch(ps4msg.dpad_hat) {
  case 0:
    msg_out->abs_dy = -1;
    break;
  case 1:
    msg_out->abs_dx = 1;
    msg_out->abs_dy = -1;
    break;
  case 2:
    msg_out->abs_dx = 1;
    break;
  case 3:
    msg_out->abs_dx = 1;
    msg_out->abs_dy = 1;
  case 4:
    msg_out->abs_dy = 1;
    break;
  case 5:
    msg_out->abs_dx = -1;
    msg_out->abs_dy = 1;
    break;
  case 6:
    msg_out->abs_dx = -1;
    break;
  case 7:
    msg_out->abs_dx = -1;
    msg_out->abs_dy = -1;
    break;
  }

  return 0;
}
