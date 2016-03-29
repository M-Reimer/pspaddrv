DESTDIR=
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin

CC ?= gcc
CFLAGS ?= -g -O3 -Wall

INCLUDES = $(shell pkg-config --cflags libusb-1.0)
LIBS = -ludev -lpthread $(shell pkg-config --libs --cflags libusb-1.0)
OBJS = main.o device-handler.o ps3-device.o ps4-device.o uinput.o usb.o

all: pspaddrv

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

pspaddrv: $(OBJS)
	$(CC) $(CFLAGS) -rdynamic $(LDFLAGS) $(OBJS) $(LIBS) -o pspaddrv

install: all
	install -D -m 755 pspaddrv $(DESTDIR)$(BINDIR)/pspaddrv

clean:
	@rm -f $(OBJS) pspaddrv
