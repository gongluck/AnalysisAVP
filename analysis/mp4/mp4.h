/*
 * @Author: gongluck
 * @Date: 2020-11-02 23:16:05
 * @Last Modified by: gongluck
 * @Last Modified time: 2021-05-18 18:47:22
 */

#pragma once

#include "../common.h"

typedef GINT8 MP4INT8;
typedef GINT16 MP4INT16;
typedef GINT24 MP4INT24;
typedef GINT32 MP4INT32;
typedef GINT64 MP4INT64;

//mp4的box名称
const MP4INT32 MP4BOXTYPE_FTYP[4] = {'f', 't', 'y', 'p'};
const MP4INT32 MP4BOXTYPE_MOOV[4] = {'m', 'o', 'o', 'v'};

const MP4INT32 MP4BOXTYPE_MVHD[4] = {'m', 'v', 'h', 'd'};
const MP4INT32 MP4BOXTYPE_TRAK[4] = {'t', 'r', 'a', 'k'};

const MP4INT32 MP4BOXTYPE_TKHD[4] = {'t', 'k', 'h', 'd'};
const MP4INT32 MP4BOXTYPE_MDIA[4] = {'m', 'd', 'i', 'a'};

const MP4INT32 MP4BOXTYPE_MDHD[4] = {'m', 'd', 'h', 'd'};
const MP4INT32 MP4BOXTYPE_HDLR[4] = {'h', 'd', 'l', 'r'};
const MP4INT32 MP4BOXTYPE_MINF[4] = {'m', 'i', 'n', 'f'};

const MP4INT32 MP4BOXTYPE_DINF[4] = {'d', 'i', 'n', 'f'};
const MP4INT32 MP4BOXTYPE_STBL[4] = {'s', 't', 'b', 'l'};

const MP4INT32 MP4BOXTYPE_DREF[4] = {'d', 'r', 'e', 'f'};

const MP4INT32 MP4BOXTYPE_STSD[4] = {'s', 't', 's', 'd'};
const MP4INT32 MP4BOXTYPE_STTS[4] = {'s', 't', 't', 's'};
const MP4INT32 MP4BOXTYPE_STSC[4] = {'s', 't', 's', 'c'};
const MP4INT32 MP4BOXTYPE_STCO[4] = {'s', 't', 'c', 'o'};

#pragma pack(1)

//mp4的box头
typedef struct __BASEBOXHEADER
{
	MP4INT32 size; //box大小
	MP4INT32 name; //box名称
} baseboxheader;
typedef struct __FULLBOXHEADER
{
	baseboxheader baseheader;
	MP4INT8 version; //box版本，0或1，一般为0
	MP4INT24 flags;
} fullboxheader;

//基础重复结构
typedef struct __BASEBOX
{
	baseboxheader header; //box头
	MP4INT8 data[0];	  //子box数据
} basebox;
typedef struct __FULLBOX
{
	fullboxheader header; //box头
	MP4INT8 data[0];	  //子box数据
} fullbox;
typedef struct __FULLTABLEBOX
{
	fullboxheader header; //box头
	MP4INT32 count;		  //表节点数
	MP4INT8 data[0];	  //表
} fulltablebox;
typedef struct __TIMEGROUP0
{
	MP4INT32 creationtime;	   //创建时间（相对于UTC时间1904-01-01零点的秒数）
	MP4INT32 modificationtime; //修改时间
	MP4INT32 timescale;		   //文件媒体在1秒时间内的刻度值，可以理解为1秒长度的时间单元数
	MP4INT32 duration;		   //该track的时间长度，用duration和timescale值可以计算track时长
} timegroup0;
typedef struct __TIMEGROUP1
{
	MP4INT64 creationtime;	   //创建时间（相对于UTC时间1904-01-01零点的秒数）
	MP4INT64 modificationtime; //修改时间
	MP4INT32 timescale;		   //文件媒体在1秒时间内的刻度值，可以理解为1秒长度的时间单元数
	MP4INT64 duration;		   //该track的时间长度，用duration和timescale值可以计算track时长
} timegroup1;

/************************************************************************************************/

// root/ftype
//ftyp相当于就是该mp4的纲领性说明。即，告诉demuxer它的基本解码版本，兼容格式。简而言之，就是用来告诉客户端，该MP4的使用的解码标准。通常，ftyp都是放在MP4的开头。
typedef struct __FTYPBOX
{
	baseboxheader header;		   //box头
	MP4INT32 majorbrand;		   //主要标识
	MP4INT32 minorversion;		   //次要版本
	MP4INT32 compatible_brands[0]; //兼容标识
} ftypbox;

// root/moov
//moovbox主要是作为一个很重要的容器盒子存在的，它本身的实际内容并不重要。moov主要是存放相关的trak。
typedef basebox moovbox;

// root/moov/mvhd
//mvhd是moov下的第一个box，用来描述media的相关信息。
typedef fullbox mvhdboxheader; //data->MVHDBOXBODY_X数据
//mvhdboxheader.version == 0
typedef struct __MVHDBOXBODY_0
{
	timegroup0 time;
	MP4INT32 rate;		  //推荐播放速率，高16位和低16位分别为小数点整数部分和小数部分，即[16.16]格式，该值为1.0（0x00010000）表示正常前向播放
	MP4INT16 volume;	  //与rate类似，[8.8]格式，1.0（0x0100）表示最大音量
	MP4INT8 reserved[10]; //保留位
	MP4INT8 matrix[36];	  //视频变换矩阵
	MP4INT8 predefined[24];
	MP4INT32 nexttrackid; //下一个track使用的id号
} mvhdboxbody_0;
//mvhdboxheader.version == 1
typedef struct __MVHDBOXBODY_1
{
	timegroup1 time;
	MP4INT32 rate;		  //推荐播放速率，高16位和低16位分别为小数点整数部分和小数部分，即[16.16]格式，该值为1.0（0x00010000）表示正常前向播放
	MP4INT16 volume;	  //与rate类似，[8.8]格式，1.0（0x0100）表示最大音量
	MP4INT8 reserved[10]; //保留位
	MP4INT8 matrix[36];	  //视频变换矩阵
	MP4INT8 predefined[24];
	MP4INT32 nexttrackid; //下一个track使用的id号
} mvhdboxbody_1;

// root/moov/trak
//trakbox就是主要存放相关mediastream的内容。
typedef basebox trakbox;

// root/moov/trak/tkhd
//tkhd是trakbox的子一级box的内容。主要是用来描述该特定trak的相关内容信息。
typedef fullbox tkhdboxheader; //tkhdboxheader.flags:
							   //0x000001 track_enabled，否则该track不被播放；
							   //0x000002 track_in_movie，表示该track在播放中被引用；
							   //0x000004 track_in_preview，表示该track在预览时被引用。
							   //一般该值为7，如果一个媒体所有track均未设置track_in_movie和track_in_preview，将被理解为所有track均设置了这两项；对于hint track，该值为0
//tkhdboxheader.version == 0
typedef struct __TKHDBOXBODY_0
{
	MP4INT32 creationtime;	   //创建时间（相对于UTC时间1904-01-01零点的秒数）
	MP4INT32 modificationtime; //修改时间
	MP4INT32 trackid;		   //id号，不能重复且不能为0
	MP4INT32 reserved1;		   //保留位
	MP4INT32 duration;		   //track的时间长度
	MP4INT64 reserved2;		   //保留位
	MP4INT16 layer;			   //视频层，默认为0，值小的在上层
	MP4INT16 alternategroup;   //track分组信息，默认为0表示该track未与其他track有群组关系
	MP4INT16 volume;		   //[8.8] 格式，如果为音频track，1.0（0x0100）表示最大音量；否则为0
	MP4INT16 reserved3;		   //保留位
	MP4INT8 matrix[36];		   //视频变换矩阵
	MP4INT32 width;			   //宽 为[16.16] 格式值，与sample描述中的实际画面大小比值，用于播放时的展示宽
	MP4INT32 height;		   //高 为[16.16] 格式值，与sample描述中的实际画面大小比值，用于播放时的展示高
} tkhdboxbody_0;
//tkhdboxheader.verflags.version == 1
typedef struct __TKHDBOXBODY_1
{
	MP4INT64 creationtime;	   //创建时间（相对于UTC时间1904-01-01零点的秒数）
	MP4INT64 modificationtime; //修改时间
	MP4INT32 trackid;		   //id号，不能重复且不能为0
	MP4INT32 reserved1;		   //保留位
	MP4INT64 duration;		   //track的时间长度
	MP4INT64 reserved2;		   //保留位
	MP4INT16 layer;			   //视频层，默认为0，值小的在上层
	MP4INT16 alternategroup;   //track分组信息，默认为0表示该track未与其他track有群组关系
	MP4INT16 volume;		   //[8.8] 格式，如果为音频track，1.0（0x0100）表示最大音量；否则为0
	MP4INT16 reserved3;		   //保留位
	MP4INT8 matrix[36];		   //视频变换矩阵
	MP4INT32 width;			   //宽 为[16.16] 格式值，与sample描述中的实际画面大小比值，用于播放时的展示宽
	MP4INT32 height;		   //高 为[16.16] 格式值，与sample描述中的实际画面大小比值，用于播放时的展示高
} tkhdboxbody_1;

// root/moov/trak/mdia
//mdia主要用来包裹相关的media信息。
typedef basebox bdiabox;

// root/moov/trak/mdia/mdhd
//mdhd和tkhd来说，内容大致都是一样的。不过，tkhd通常是对指定的track设定相关属性和内容。而mdhd是针对于独立的media来设置的。不过事实上，两者一般都是一样的。
typedef fullbox mdhdboxheader;
//mdhdboxheader.version == 0
typedef struct __MDHDBOXBODY_0
{
	timegroup0 time;
	MP4INT16 language; //媒体语言码。最高位为0，后面15位为3个字符（见ISO 639-2/T标准中定义）
	MP4INT16 predefined;
} mdhdboxbody_0;
//tkhdboxheader.version == 1
typedef struct __MDHDBOXBODY_1
{
	timegroup1 time;
	MP4INT16 language; //媒体语言码。最高位为0，后面15位为3个字符（见ISO 639-2/T标准中定义）
	MP4INT16 predefined;
} mdhdboxbody_1;

// root/moov/trak/mdia/hdlr
//hdlr是用来设置不同trak的处理方式的。
typedef struct __HDLRBOX
{
	fullboxheader header;
	MP4INT32 predefined;
	MP4INT32 handlertype; //在media box中，该值为4个字符：
						  //vide — video track
						  //soun — audio track
						  //hint — hint track
	MP4INT8 reserved[12];
	MP4INT8 name[0]; //track type name，以'\0'结尾的字符串
} hdlrbox;

// root/moov/trak/mdia/minf
//minf是子属内容中，重要的容器box，用来存放当前track的基本描述信息。
typedef basebox minfbox;

// root/moov/trak/mdia/minf/dinf
//dinf是用来说明在trak中，media描述信息的位置。
typedef basebox dinfbox;

// root/moov/trak/mdia/minf/stbl
//MP4box根据trak中的stbl下的stts、stsc等基本box来完成在mdatbox中的索引。
typedef basebox stblbox;

// root/moov/trak/mdia/minf/dinf/dref
//dref是用来设置当前Box描述信息的data_entry。
typedef fulltablebox drefbox;

// root/moov/trak/mdia/minf/stbl/stsd
typedef fulltablebox stsdbox;

// root/moov/trak/mdia/minf/stbl/stts
//stts主要是用来存储refSampleDelta。即，相邻两帧间隔的时间。
typedef fulltablebox sttsboxheader;
typedef struct __STTSENTRY
{
	MP4INT32 count;
	MP4INT32 duration;
} sttsentry;

// root/moov/trak/mdia/minf/stbl/stsc
typedef fulltablebox stscboxheader;
typedef struct __STSCENTRY
{
	MP4INT32 firstchunk;	   //每一个entry开始的chunk位置
	MP4INT32 samplesperchunk;  //每一个chunk里面包含多少的 sample
	MP4INT32 descriptionindex; //每一个sample的描述。一般可以默认设置为1
} stscentry;

// root/moov/trak/mdia/minf/stbl/stco
//stco是stbl包里面一个非常关键的Box。它用来定义每一个sample在mdat具体的位置。
typedef fulltablebox stcoboxheader;
typedef struct __STCOENTRY
{
	MP4INT32 offset;
} stcoentry;

#pragma pack()
