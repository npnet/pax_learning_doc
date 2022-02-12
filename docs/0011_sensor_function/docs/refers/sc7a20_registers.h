/*
The MIT License (MIT)
Copyright (c) 2017 Kionix Inc.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef __SC7A20_REGISTERS_H__
#define __SC7A20_REGISTERS_H__
/* registers */
// output register x
#define SC7A20_XOUT_L 0x28
#define SC7A20_XOUT_H 0x29
// output register y
#define SC7A20_YOUT_L 0x2A
#define SC7A20_YOUT_H 0x2B
// output register z
#define SC7A20_ZOUT_L 0x2C
#define SC7A20_ZOUT_H 0x2D


// This register can be used for supplier recognition, as it can be factory written to a known byte value.
#define SC7A20_WHO_AM_I 0x0F
// Read/write control register that controls the main feature set
#define SC7A20_CTRL_REG1 0x20     //djq, 2019-05-28
// Read/write control register that provides more feature set control
#define SC7A20_CTRL_REG4 0x23

/* registers bits */
// WHO_AM_I -value
#define SC7A20_WHO_AM_I_WIA_ID (0x11 << 0)  //djq, 2019-05-20
#define SC7A20_WHO_AM_I_WIA_MASK 0xFF

#endif

