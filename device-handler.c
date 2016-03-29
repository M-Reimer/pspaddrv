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
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <linux/uinput.h>
#include "device-handler.h"
#include "usb.h"
#include "uinput.h"
#include "ps3-device.h"
#include "ps4-device.h"

struct RumbleArgs {
  int fduinput;
  libusb_device_handle *usbdev;
  int devtype;
};

void *DeviceHandlerThreadRumble (void *attr) {
  struct RumbleArgs *args = (struct RumbleArgs *)attr;
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  struct input_event event;

  int effect_id = -1;
  int strong = 0;
  int weak = 0;

  printf("Startup: %d\n", args->fduinput);

  while (1) {
    ssize_t n = read(args->fduinput, &event, sizeof(event));
    if (n == -1) {
      if (errno == EINTR) {
        printf("EINTR\n");
        continue;
      }
      else {
        printf("Other error when reading %d\n", n);
        break;
      }
    }

    if (n != sizeof(event)) {
      printf("Size error\n");
      break;
    }

    printf("Received something\n");

    if (event.type == EV_FF && event.code == effect_id) {
      if (args->devtype == PS3_DEVICE)
        printf("Would send\n");
        //PS3SendRumbleUSB(args->usbdev, event.value ? weak : 0, event.value ? strong : 0);
    }
    else if (event.type == EV_UINPUT) {
      printf("EV_UINPUT %d\n", event.code);
      if (event.code == UI_FF_UPLOAD) {
        printf("UI_FF_UPLOAD start\n");
        struct uinput_ff_upload upload;
        memset(&upload, 0, sizeof(upload));

        upload.request_id = event.value;

        printf("UI_FF_UPLOAD middle\n");
        int ret;
        ret = ioctl(args->fduinput, UI_BEGIN_FF_UPLOAD, &upload);
        if (ret < 0) {
          printf("first ioctl failed %d %s\n", ret, strerror(errno));
        }

        // Remember ID and motor values for playback
        if (upload.effect.type == FF_RUMBLE) {
          effect_id = upload.effect.id;
          strong = upload.effect.u.rumble.strong_magnitude;
          weak = upload.effect.u.rumble.weak_magnitude;
          printf("Effect uploaded\n");
        }

        ret = ioctl(args->fduinput, UI_END_FF_UPLOAD, &upload);
        if (ret < 0) {
          printf("second ioctl failed %d %s\n", ret, strerror(errno));
        }
        printf("UI_FF_UPLOAD end\n");
      }
      else if (event.code == UI_FF_ERASE) {
        struct uinput_ff_erase erase;
        memset(&erase, 0, sizeof(erase));

        erase.request_id = event.value;

        // Doesn't make sense to actually erase something...
        ioctl(args->fduinput, UI_BEGIN_FF_ERASE, &erase);
        ioctl(args->fduinput, UI_END_FF_ERASE, &erase);
        printf("Event erased\n");
      }
    }
  }

  return NULL;
}

void *DeviceHandlerThreadUSB (void *attr) {
  // Open USB device
  libusb_device_handle *usbdev;
  int devtype = ((struct USBDeviceHandlerArgs *)attr)->devtype;
  int ret = USBOpenDevice((struct USBDeviceHandlerArgs *)attr, &usbdev);
  free(attr);
  if (ret < 0) {
    syslog(LOG_ERR, "Failed to open controller device");
    return NULL;
  }

  // Enable controller
  if (devtype == PS3_DEVICE) {
    if (PS3SetOperationalUSB(usbdev) < 0) {
      syslog(LOG_ERR, "Failed to enable PS3 controller");
      libusb_close(usbdev);
      return NULL;
    }
  }

  // Open Uinput device
  int fduinput = UinputInit();
  if (fduinput < 0) {
    syslog(LOG_ERR, "Uinput Init failed!");
    libusb_close(usbdev);
    return NULL;
  }

  // Launch thread to handle rumble events
  pthread_t tid_rumble;
  if (1) { // TODO: Make this configurable
    struct RumbleArgs rargs;
    rargs.fduinput = fduinput;
    rargs.usbdev = usbdev;
    rargs.devtype = devtype;
    pthread_create(&tid_rumble, NULL, &DeviceHandlerThreadRumble, (void *)&rargs);
  }

  // Main loop
  while(1) {
    struct XpadMsg msg_out;

    if (devtype == PS3_DEVICE)
      ret = PS3PollInputUSB(usbdev, &msg_out);
    else
      ret = PS4PollInputUSB(usbdev, &msg_out);

    if (ret < 0) {
      printf("    ERROR: Controller did not return values %d\n", ret);
      break;
    }

    UinputSendXpadMsg(fduinput, msg_out);
  }

  // Close rumble thread
  if (1) { // TODO: Make this configurable
    pthread_cancel(tid_rumble);
    pthread_join(tid_rumble, NULL);
  }

  // Close open devices
  libusb_close(usbdev);
  close(fduinput);
  return NULL;
}
