/*
Copyright (C) 2003-2004 Nathan Wulf (jitspoe)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// === jitmenu / jitscores
// encode_bin_str.c -- Functions to encode/decode binary data stored as a string

#include "client.h"

#define MARKER_CHAR 255
#define MARKER_CHAR_0 1
#define MARKER_CHAR_SHORT 254
#define MARKER_CHAR_LONG 253

static unsigned char *outpos;


static void encode_addmarker (const unsigned char marker)
{
	*outpos = MARKER_CHAR;
	outpos++;
	*outpos = marker;
	outpos++;
}

static void encode_addchar (unsigned int c)
{
	c &= 255;

	if (c == 0)
	{
		*outpos = MARKER_CHAR;
		outpos++;
		*outpos = MARKER_CHAR_0;
	}
	else if (c == MARKER_CHAR)
	{
		*outpos = MARKER_CHAR;
		outpos++;
		*outpos = MARKER_CHAR;
	}
	else
	{
		*outpos = c;
	}

	outpos++;
}

static void encode_end (void)
{
	*outpos = '\0';
}


void encode_unsigned (unsigned int count, unsigned int *in, unsigned char *out)
{
	unsigned int i, out_i = 0;
	unsigned int j;
	unsigned char c;
	unsigned short s;

	outpos = out;

	for (i = 0; i < count; i++)
	{
		j = in[i] + 1; // since 0 requires an extra char and is common, we want to avoid it.
		s = (short)j;
		c = (char)j;

		if (c == j) // j < 256, just need 1 char to store it.
		{
			encode_addchar(c);
		}
		else if (s == j) // can fit in 2 bytes (short)
		{
			encode_addmarker(MARKER_CHAR_SHORT);
			encode_addchar(s>>8);
			encode_addchar(s);
		}
		else // full 32bit integer (4 bytes)
		{
			encode_addmarker(MARKER_CHAR_LONG);
			encode_addchar(j>>24);
			encode_addchar(j>>16);
			encode_addchar(j>>8);
			encode_addchar(j);
		}
	}
	encode_end();
}

char *decode_ptr;

static unsigned int decode_char_unsigned (void);

static unsigned int decode_short_unsigned (void)
{
	return (decode_char_unsigned() << 8) | decode_char_unsigned();
}

static unsigned int decode_long_unsigned (void)
{
	return (decode_short_unsigned() << 16) | decode_short_unsigned();
}

static unsigned int decode_char_unsigned (void)
{
	unsigned char c;
	
	c = *decode_ptr;
	decode_ptr++;

	if (c == MARKER_CHAR)
	{
		c = *decode_ptr;
		decode_ptr++;

		switch (c)
		{
			case MARKER_CHAR:
				return MARKER_CHAR;
				break;
			case MARKER_CHAR_0:
				return 0;
				break;
			case MARKER_CHAR_SHORT:
				return decode_short_unsigned();
				break;
			case MARKER_CHAR_LONG:
				return decode_long_unsigned();
				break;
			default:
				printf("ERROR: Bad encode!\n");
				return 0;
				break;
		}
	}
	else
	{
		return c;
	}
}

int decode_unsigned(unsigned char *in, unsigned int *out) // returns number of integers decoded
{
	int i = 0;

	decode_ptr = in;

	while (*decode_ptr)
	{
		out[i] = decode_char_unsigned() - 1; // - 1 is to compensate for 1 added in encode_unsigned
		i++;
	}

	return i;
}

//#define TESTSIZE 254
//int main ()
//{
//	int i, j;
//	int indata[TESTSIZE];
//	char encdata[TESTSIZE*5];
//	int outdata[TESTSIZE];
//
//	j = 0;
//	for (i=0; i<TESTSIZE; i++) {
//		indata[i] = j;
//		j+=1;
//	}
//
//	encode_unsigned(TESTSIZE, indata, encdata);
//
//	decode_unsigned(encdata, outdata);
//
//	for (i=0; i<TESTSIZE; i++) {
//		if (indata[i] != outdata[i])
//			printf("DOH! %d != %d\n", indata[i], outdata[i]);
//		printf(".");
//	}
//	printf("\n");
//	printf("str size: %d\n", strlen(encdata));
//
//	return 0;
//}

