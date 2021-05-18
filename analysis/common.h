/*
 * @Author: gongluck 
 * @Date: 2021-05-18 10:26:54 
 * @Last Modified by: gongluck
 * @Last Modified time: 2021-05-18 18:54:03
 */

#pragma once

#include <stdint.h>

#pragma pack(1)

typedef uint8_t GINT8;
typedef struct __GINT16
{
	uint8_t data1;
	uint8_t data2;
} GINT16;
typedef struct __GINT24
{
	uint8_t data1;
	uint8_t data2;
	uint8_t data3;
} GINT24;
typedef struct __GINT32
{
	uint8_t data1;
	uint8_t data2;
	uint8_t data3;
	uint8_t data4;
} GINT32;
typedef struct __GINT64
{
	uint8_t data1;
	uint8_t data2;
	uint8_t data3;
	uint8_t data4;
	uint8_t data5;
	uint8_t data6;
	uint8_t data7;
	uint8_t data8;
} GINT64;

#define GINT16TOINT(x) (x.data1 << 8 | x.data2)
#define GINT24TOINT(x) (x.data1 << 16 | x.data2 << 8 | x.data3)
#define GINT32TOINT(x) (x.data1 << 24 | x.data2 << 16 | x.data3 << 8 | x.data4)

#define GINT16TODOUBLE(x) (x.data1 * 1.0 + x.data2 * 1.0 / 0xFF)
#define GINT32TODOUBLE(x) (GINT16TOINT(x) * 1.0 + (x.data3 << 8 | x.data4) * 1.0 / 0xFFFF)

#pragma pack()
