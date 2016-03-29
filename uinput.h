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

struct XpadMsg {
  unsigned int btn_a;
  unsigned int btn_b;
  unsigned int btn_x;
  unsigned int btn_y;

  unsigned int btn_select;
  unsigned int btn_start;
  unsigned int btn_guide;

  unsigned int btn_ls;
  unsigned int btn_lb;

  unsigned int btn_rs;
  unsigned int btn_rb;

  signed int abs_lx;
  signed int abs_ly;
  signed int abs_rx;
  signed int abs_ry;

  unsigned int abs_lt;
  unsigned int abs_rt;

  signed int abs_dx;
  signed int abs_dy;
};

#define XPAD_TRIGGERMAX 255
#define XPAD_STICKMIN -32768
#define XPAD_STICKMAX 32767
#define XPAD_FUZZ 16
#define XPAD_FLAT 128

#define PS_FLAT 15
#define PS_STICKMAX 255

int UinputInit();
void UinputSendXpadMsg(int fd, struct XpadMsg msg);
