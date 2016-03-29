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

#include <syslog.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "uinput.h"

// Creates new event device and initializes it with all the properties of the
// XBox 360 USB gamepad.
int UinputInit() {
  int fd;
  if ((fd = open("/dev/uinput", O_RDWR)) == -1) {
    syslog(LOG_ERR, "Failed to open /dev/uinput!");
    return -1;
  }

  int i;

  // Set evbits
  int evbits[] = {EV_ABS, EV_KEY, EV_SYN};
  for (i = 0; i < sizeof(evbits)/sizeof(int); i++) {
    if (ioctl(fd, UI_SET_EVBIT, evbits[i]) < 0) {
      syslog(LOG_ERR, "uinput ioctl failed!");
      close(fd);
      return -1;
    }
  }

  // Set keybits
  int keybits[] = {BTN_A, BTN_B, BTN_X, BTN_Y,
                   BTN_SELECT, BTN_START, BTN_MODE,
                   BTN_THUMBL, BTN_TL, BTN_THUMBR, BTN_TR};
  for (i = 0; i < sizeof(keybits)/sizeof(int); i++) {
    if (ioctl(fd, UI_SET_KEYBIT, keybits[i]) < 0) {
      syslog(LOG_ERR, "uinput ioctl failed!");
      close(fd);
      return -1;
    }
  }

  // Set absbits
  int absbits[] = {ABS_HAT0X, ABS_HAT0Y,
                   ABS_X, ABS_Y, ABS_Z,
                   ABS_RX, ABS_RY, ABS_RZ};
  for (i = 0; i < sizeof(absbits)/sizeof(int); i++) {
    if (ioctl(fd, UI_SET_ABSBIT, absbits[i]) < 0) {
      syslog(LOG_ERR, "uinput ioctl failed!");
      close(fd);
      return -1;
    }
  }

  // Set force feedback bits
  if (1) { // TODO: Make this configurable
    if (ioctl(fd, UI_SET_EVBIT, EV_FF) < 0) {
      syslog(LOG_ERR, "uinput ioctl failed!");
      close(fd);
      return -1;
    }
    if (ioctl(fd, UI_SET_FFBIT, FF_RUMBLE) < 0) {
      syslog(LOG_ERR, "uinput ioctl failed!");
      close(fd);
      return -1;
    }
  }

  // Set up uinput device information
  struct uinput_user_dev uidev;
  memset(&uidev, 0, sizeof(uidev));

  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Microsoft X-Box 360 pad");
  uidev.id.bustype = BUS_USB;
  uidev.id.vendor  = 0x045e;
  uidev.id.product = 0x028e;
  uidev.id.version = 0x110;

  uidev.absmin[ABS_X] = XPAD_STICKMIN;
  uidev.absmax[ABS_X] = XPAD_STICKMAX;
  uidev.absfuzz[ABS_X] = XPAD_FUZZ;
  uidev.absflat[ABS_X] = XPAD_FLAT;
  uidev.absmin[ABS_Y] = XPAD_STICKMIN;
  uidev.absmax[ABS_Y] = XPAD_STICKMAX;
  uidev.absfuzz[ABS_Y] = XPAD_FUZZ;
  uidev.absflat[ABS_Y] = XPAD_FLAT;

  uidev.absmin[ABS_RX] = XPAD_STICKMIN;
  uidev.absmax[ABS_RX] = XPAD_STICKMAX;
  uidev.absfuzz[ABS_RX] = XPAD_FUZZ;
  uidev.absflat[ABS_RX] = XPAD_FLAT;
  uidev.absmin[ABS_RY] = XPAD_STICKMIN;
  uidev.absmax[ABS_RY] = XPAD_STICKMAX;
  uidev.absfuzz[ABS_RY] = XPAD_FUZZ;
  uidev.absflat[ABS_RY] = XPAD_FLAT;

  uidev.absmin[ABS_Z] = 0;
  uidev.absmax[ABS_Z] = XPAD_TRIGGERMAX;
  uidev.absfuzz[ABS_Z] = 0;
  uidev.absflat[ABS_Z] = 0;
  uidev.absmin[ABS_RZ] = 0;
  uidev.absmax[ABS_RZ] = XPAD_TRIGGERMAX;
  uidev.absfuzz[ABS_RZ] = 0;
  uidev.absflat[ABS_RZ] = 0;

  uidev.absmin[ABS_HAT0X] = -1;
  uidev.absmax[ABS_HAT0X] = 1;
  uidev.absfuzz[ABS_HAT0X] = 0;
  uidev.absflat[ABS_HAT0X] = 0;
  uidev.absmin[ABS_HAT0Y] = -1;
  uidev.absmax[ABS_HAT0Y] = 1;
  uidev.absfuzz[ABS_HAT0Y] = 0;
  uidev.absflat[ABS_HAT0Y] = 0;

  if (1) // TODO: Make this configurable
    uidev.ff_effects_max = 1;

  if (write(fd, &uidev, sizeof(uidev)) < 0) {
    syslog(LOG_ERR, "uinput write failed!");
    close(fd);
    return -1;
  }
  if (ioctl(fd, UI_DEV_CREATE) < 0) {
    syslog(LOG_ERR, "uinput device creation failed!");
    close(fd);
    return -1;
  }

  return fd;
}


int TranslateStickValue(int value) {
  value -= 128;

  const int PS_MIN_ABS = 128;
  const int PS_MAX_ABS = 127;
  const int H_PS_FLAT = PS_FLAT / 2;
  const int H_XPAD_FLAT = XPAD_FLAT / 2;

  if (value >= -1*H_PS_FLAT && value <= H_PS_FLAT)
    return 0;
  else if (value < 0)
    return (value + H_PS_FLAT)*((-1*XPAD_STICKMIN-H_XPAD_FLAT)/(PS_MIN_ABS-H_PS_FLAT))-H_XPAD_FLAT;
  else
    return (value - H_PS_FLAT)*((XPAD_STICKMAX-H_XPAD_FLAT)/(PS_MAX_ABS-H_PS_FLAT))+H_XPAD_FLAT;
}


// This function sends one group of messages out to the event device
// Stick values have to be passed in the PlayStation range (0 to 255) and are
// translated before sending to the kernel.
void UinputSendXpadMsg(int fd, struct XpadMsg msg) {
  struct input_event event;
  event.type = EV_KEY;
  event.code = BTN_A;
  event.value = msg.btn_a;
  write(fd, &event, sizeof(event));
  event.code = BTN_B;
  event.value = msg.btn_b;
  write(fd, &event, sizeof(event));
  event.code = BTN_X;
  event.value = msg.btn_x;
  write(fd, &event, sizeof(event));
  event.code = BTN_Y;
  event.value = msg.btn_y;
  write(fd, &event, sizeof(event));
  event.code = BTN_SELECT;
  event.value = msg.btn_select;
  write(fd, &event, sizeof(event));
  event.code = BTN_START;
  event.value = msg.btn_start;
  write(fd, &event, sizeof(event));
  event.code = BTN_MODE;
  event.value = msg.btn_guide;
  write(fd, &event, sizeof(event));
  event.code = BTN_THUMBL;
  event.value = msg.btn_ls;
  write(fd, &event, sizeof(event));
  event.code = BTN_THUMBR;
  event.value = msg.btn_rs;
  write(fd, &event, sizeof(event));
  event.code = BTN_TL;
  event.value = msg.btn_lb;
  write(fd, &event, sizeof(event));
  event.code = BTN_TR;
  event.value = msg.btn_rb;
  write(fd, &event, sizeof(event));

  event.type = EV_ABS;
  event.code = ABS_HAT0X;
  event.value = msg.abs_dx;
  write(fd, &event, sizeof(event));
  event.code = ABS_HAT0Y;
  event.value = msg.abs_dy;
  write(fd, &event, sizeof(event));
  event.code = ABS_Z;
  event.value = msg.abs_lt;
  write(fd, &event, sizeof(event));
  event.code = ABS_RZ;
  event.value = msg.abs_rt;
  write(fd, &event, sizeof(event));

  event.code = ABS_X;
  event.value = TranslateStickValue(msg.abs_lx);
  write(fd, &event, sizeof(event));
  event.code = ABS_Y;
  event.value = TranslateStickValue(msg.abs_ly);
  write(fd, &event, sizeof(event));
  event.code = ABS_RX;
  event.value = TranslateStickValue(msg.abs_rx);
  write(fd, &event, sizeof(event));
  event.code = ABS_RY;
  event.value = TranslateStickValue(msg.abs_ry);
  write(fd, &event, sizeof(event));

  event.type = EV_SYN;
  event.code = 0;
  event.value = 0;
  write(fd, &event, sizeof(event));
}
