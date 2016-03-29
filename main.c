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

#include <libudev.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <syslog.h>
#include "device-handler.h"
#include "usb.h"

#define SONY_VENDOR_ID   "054c"
#define PS3_PRODUCT_ID   "0268"
#define PS4_PRODUCT_ID   "05c4"

// This function starts a new thread to handle one game controller
void StartUSBDeviceHandler(struct USBDeviceHandlerArgs *args) {
  pthread_attr_t tattr;
  pthread_t tid;
  if (pthread_attr_init(&tattr) != 0) {
    syslog(LOG_ERR, "StartDeviceHandler: pthread_attr_init failed!");
    return;
  }
  if (pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED) != 0) {
    syslog(LOG_ERR, "StartDeviceHandler: pthread_attr_setdetachstate failed!");
    return;
  }

  if (pthread_create(&tid, &tattr, &DeviceHandlerThreadUSB, (void *)args) != 0)
    syslog(LOG_ERR, "StartDeviceHandler: Failed to start new thread!");
}

// Prefiltered events from udev land here
void DeviceAdded(struct udev_device *dev) {
  // Get product ID
  const char *product = NULL;
  product = udev_device_get_property_value(dev, "ID_MODEL_ID");
  if (!product)
    return;

  // Only PS3/PS4 controller supported
  int devtype;
  if (strcasecmp(product, PS3_PRODUCT_ID) == 0)
    devtype = PS3_DEVICE;
  else if (strcasecmp(product, PS4_PRODUCT_ID) == 0)
    devtype = PS4_DEVICE;
  else
    return;

  // Get bus and device number
  const char *cbusnum = NULL;
  const char *cdevnum = NULL;
  cbusnum = udev_device_get_property_value(dev, "BUSNUM");
  cdevnum = udev_device_get_property_value(dev, "DEVNUM");
  if (!cbusnum || !cdevnum)
    return;
  struct USBDeviceHandlerArgs *args = malloc(sizeof(struct USBDeviceHandlerArgs));
  if (args == NULL)
    return;
  args->busnum = atoi(cbusnum);
  args->devnum = atoi(cdevnum);
  args->devtype = devtype;

  StartUSBDeviceHandler(args);
}

int main (void) {
  struct udev *udev;
  struct udev_enumerate *enumerate;
  struct udev_list_entry *devices, *dev_list_entry;
  struct udev_device *dev;

  struct udev_monitor *mon;

  // Init syslog
  openlog("pspaddrv", LOG_PID, LOG_DAEMON);

  // Init libusb
  libusb_init(NULL);

  // Create a new session for our daemon
  /*  if (daemon(0, 1) == -1) {
    syslog(LOG_ERR, "Can't create new session");
    exit(1);
    }*/

  /* Create the udev object */
  udev = udev_new();
  if (!udev) {
    syslog(LOG_ERR, "Can't create udev");
    exit(1);
  }

  /* Set up a monitor to monitor USB devices */
  mon = udev_monitor_new_from_netlink(udev, "udev");
  udev_monitor_filter_add_match_subsystem_devtype(mon, "usb", "usb_device");
  udev_monitor_enable_receiving(mon);
  /* Get the file descriptor (fd) for the monitor.
     This fd will get passed to select() */
  int udev_monitor_fd = udev_monitor_get_fd(mon);

  /* Create a list of the devices in the 'input' subsystem. */
  enumerate = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(enumerate, "usb");
  udev_enumerate_add_match_property(enumerate, "ID_VENDOR_ID", SONY_VENDOR_ID);
  udev_enumerate_scan_devices(enumerate);
  devices = udev_enumerate_get_list_entry(enumerate);
  udev_list_entry_foreach(dev_list_entry, devices) {
    const char *path;
    printf("found\n");
    /* Get the filename of the /sys entry for the device
       and create a udev_device object (dev) representing it */
    path = udev_list_entry_get_name(dev_list_entry);
    dev = udev_device_new_from_syspath(udev, path);

    DeviceAdded(dev);

    udev_device_unref(dev);
  }
  /* Free the enumerator object */
  udev_enumerate_unref(enumerate);

  /* Begin polling for udev events. */
  while (1) {
    fd_set fds;
    int ret;

    FD_ZERO(&fds);
    FD_SET(udev_monitor_fd, &fds);

    ret = select(udev_monitor_fd+1, &fds, NULL, NULL, NULL);

    /* Check if our file descriptor has received data. */
    if (ret > 0 && FD_ISSET(udev_monitor_fd, &fds)) {
      /* Make the call to receive the device.
         select() ensured that this will not block. */
      dev = udev_monitor_receive_device(mon);
      if (dev) {
        const char *action = NULL;
        action = udev_device_get_property_value(dev, "ACTION");
        const char *vendor = NULL;
        vendor = udev_device_get_property_value(dev, "ID_VENDOR_ID");
        if (strcmp(action, "add") == 0 &&
            strcmp(vendor, SONY_VENDOR_ID) == 0)
          DeviceAdded(dev);

        udev_device_unref(dev);
      }
      else {
        syslog(LOG_ERR, "No Device from receive_device().");
      }
    }
  }

  udev_unref(udev);
  return 0;
}
