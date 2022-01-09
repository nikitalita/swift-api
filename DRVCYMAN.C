/* Logitech Cyberman support for AVRIL */
/* 

   Copyright 1994, 1995, 1996, 1997, 1998, 1999  by Bernie Roehl 

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA,
   02111-1307, USA.

   For more information, contact Bernie Roehl <broehl@uwaterloo.ca> 
   or at Bernie Roehl, 68 Margaret Avenue North, Waterloo, Ontario, 
   N2J 3P7, Canada

*/

#include "avril.h"

static vrl_DeviceChannel cybchannel_defaults[] =
	{
		/* center, deadzone, range, scale, accum */
		{ 0, 5,  128, float2scalar(1000), 1 },  /* trans X */
		{ 0, 0,    1, float2scalar(1000), 1 },  /* trans Y */
		{ 0, 5, -128, float2scalar(1000), 1 },  /* trans Z */
		{ 0, 0,   -1, float2angle(20), 1 },  /* rot X */
		{ 0, 0,   -1, float2angle(20), 1 },  /* rot Y */
		{ 0, 0,    1, float2angle(20), 1 }   /* rot Z */
	};

static void reset_cyberman(vrl_SerialPort *port)
	{
	vrl_SerialSetDTR(port, 1);  /* DTR high */
	vrl_SerialSetRTS(port, 0);  /* RTS low */
	vrl_TimerDelay(200);        /* spec says 100, but let's be generous */
	vrl_SerialSetRTS(port, 1);  /* RTS high */
	vrl_TimerDelay(400);        /* 110 for first char, 100 for second */
	}

static int values[] = { 0, 1, -1, 0 };

int vrl_CybermanOutput(vrl_Device *dev, int parm1, vrl_Scalar parm2)
	{
	unsigned char val = ((vrl_32bit) scalar2float(parm2)) & 0xFF;
	if (parm1) return -1;
	vrl_SerialPutc('!', dev->port);
	vrl_SerialPutc('T', dev->port);
	vrl_SerialPutc(val, dev->port);
	vrl_SerialPutc(255 - val, dev->port);
	vrl_SerialPutc((val == 0) ? 0 : 7, dev->port);  /* 280 ms */
	return 0;
	}

int vrl_CybermanDevice(vrl_DeviceCommand cmd, vrl_Device *device)
	{
	switch (cmd)
		{
		case VRL_DEVICE_INIT:
			if (device->port == NULL) return -4;
			device->nchannels = 6;
			device->channels = vrl_calloc(device->nchannels, sizeof(vrl_DeviceChannel));
			if (device->channels == NULL) return -1;
			device->localdata = vrl_DeviceCreatePacketBuffer(5);
			if (device->localdata == NULL)
				{
				vrl_free(device->channels);
				device->channels = NULL;
				return -1;
				}
			device->nbuttons = 3;
			device->noutput_channels = 1;
			device->outfunc = vrl_CybermanOutput;
			device->desc = "Logitech Cyberman, direct serial";
			device->version = 1;
		case VRL_DEVICE_RESET:
			vrl_SerialSetParameters(device->port, 1200, VRL_PARITY_NONE, 7, 1);
			reset_cyberman(device->port);
			/* look for the 'M3' identification sequence */
			if (!vrl_SerialCheck(device->port)) return -2;
			if (vrl_SerialGetc(device->port) != 'M') return -3;
			if (!vrl_SerialCheck(device->port)) return -2;
			if (vrl_SerialGetc(device->port) != '3') return -3;
			/* enter Swift mode */
			vrl_SerialPutc('*', device->port);  vrl_SerialPutc('S', device->port);
			vrl_TimerDelay(500);
			vrl_SerialSetParameters(device->port, 4800, VRL_PARITY_NONE, 8, 1);
			vrl_TimerDelay(200);
			vrl_SerialFlush(device->port);
			/* request a Static Device Status report */
			vrl_SerialPutc('!', device->port);  vrl_SerialPutc('S', device->port);
			vrl_TimerDelay(500);  /* wait for the report to come in */
			while (vrl_SerialCheck(device->port))
				if ((vrl_SerialGetc(device->port) & 0xE0) == 0xA0)
					break;
			if (!vrl_SerialCheck(device->port))
				{
				reset_cyberman(device->port);
				return -2;  /* no report */
				}
			vrl_SerialFlush(device->port);
			memcpy(device->channels, cybchannel_defaults, sizeof(cybchannel_defaults));
			return 0;
		case VRL_DEVICE_POLL:
			while (vrl_DeviceGetPacket(device->port, device->localdata))
				{
				unsigned char *p = vrl_DevicePacketGetBuffer(device->localdata);
				int x, y, i;
				unsigned int b;
				if ((p[0] & 0xE0) != 0x80) continue;  /* ignore non-data packets */
				b = p[0] & 0x1F;
				device->bchanged = b ^ device->buttons;
				device->buttons = b;
				x = (p[1] << 1) | ((p[2] >> 6) & 0x01);
				y = (p[2] << 2) | ((p[3] >> 5) & 0x03);
				if (x & 0x80) x |= 0xFF00;  /* sign extend */
				else x &= 0xFF;
				if (y & 0x80) y |= 0xFF00;  /* sign extend */
				else y &= 0xFF;
				device->channels[X].rawvalue = x;
				device->channels[Y].rawvalue = values[(p[3] >> 3) & 0x03];
				device->channels[Z].rawvalue = y;
				device->channels[XROT].rawvalue = values[(p[3] >> 1) & 0x03];
				device->channels[YROT].rawvalue = values[(p[4] >> 4) & 0x03];
				device->channels[ZROT].rawvalue = values[((p[3] << 1) | ((p[4] >> 6) & 0x01)) & 0x03];
				for (i = 0; i < 6; ++i)
					device->channels[i].changed = 1;
				}
			return 0;
		case VRL_DEVICE_QUIT:
			reset_cyberman(device->port);
			if (device->localdata)
				{
				vrl_DeviceDestroyPacketBuffer(device->localdata);
				device->localdata = NULL;
				}
			if (device->channels)
				{
				vrl_free(device->channels);
				device->channels = NULL;
				}
			return 0;
		default: break;
		}
	return 1;  /* function not implemented */
	}
