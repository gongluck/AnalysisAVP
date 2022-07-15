/*
 * @Author: gongluck
 * @Date: 2022-07-15 10:01:56
 * @Last Modified by: gongluck
 * @Last Modified time: 2022-07-15 18:24:30
 */

//参考
// https://www.jianshu.com/p/cce99eb216db
// https://www.freesion.com/article/9734425469/

#import "ViewController.h"
#import <AudioToolbox/AudioToolbox.h>
#include <mutex>
#include <thread>

//参数
const int BUFCOUNT = 3;                           //音频缓冲区数
const int BUFSIZE = 1024;                         //音频缓冲区大小
const int INSAMPLERATE = 44100;                   //输入采样率
const int OUTSAMPLERATE = 44100;                  //输出采样率
const int INCHANNELS = 2;                         //输入通道数
const int OUTCHANNELS = 2;                        //输出通道数
const int BITRATE = 192000;                       //比特率
const int MAXPCMBUFSIZE = BUFSIZE * BUFCOUNT * 4; // PCM缓冲区最大容量

std::mutex pcmmutex;

@interface ViewController ()

@property(nonatomic) AudioStreamBasicDescription informat;  //输入格式
@property(nonatomic) AudioStreamBasicDescription outformat; //输出格式
@property(nonatomic) AudioConverterRef acoverter;           //编码器
@property(nonatomic) AudioQueueRef queue;                   //音频队列
@property(nonatomic) FILE *hpcm;                            // pcm文件句柄
@property(nonatomic) FILE *haac;                            // aac文件句柄
@property(nonatomic) int maxpackagesize;                    //最大包大小
@property(nonatomic) char *pcmbuf;                          // pcm缓冲区
@property(nonatomic) int readpos;
@property(nonatomic) int writepos;
@property(nonatomic) int datalen;
@property(nonatomic) void *aacbuf; // aac缓冲区
@property(nonatomic) bool stop;

@end

@implementation ViewController

#define PRINTSTATUS(ret)                                                       \
  if (ret != noErr) {                                                          \
    NSLog(@"error : %d in %s(%d)", ret, __FILE__, __LINE__);                   \
  }

- (NSData *)adtsDataForPacketLength:(NSUInteger)packetLength {
  int adtsLength = 7;
  char *packet = (char *)malloc(sizeof(char) * adtsLength);
  int profile = 2; // AAC LC
  int freqIdx = 4; // 44.1KHz
  int chanCfg = 2; // MPEG-4 Audio Channel Configuration.
  NSUInteger fullLength = adtsLength + packetLength;
  packet[0] = (char)0xFF; // 11111111     = syncword
  packet[1] = (char)0xF1; // 1111 0 00 1  = syncword MPEG-2 Layer CRC
  packet[2] = (char)(((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
  packet[3] = (char)(((chanCfg & 3) << 6) + (fullLength >> 11));
  packet[4] = (char)((fullLength & 0x7FF) >> 3);
  packet[5] = (char)(((fullLength & 7) << 5) + 0x1F);
  packet[6] = (char)0xFC;
  NSData *data = [NSData dataWithBytesNoCopy:packet
                                      length:adtsLength
                                freeWhenDone:YES];
  return data;
}

//填充编码输入数据
OSStatus AudioConverterComplexInputDataCB(
    AudioConverterRef inAudioConverter, UInt32 *ioNumberDataPackets,
    AudioBufferList *ioData,
    AudioStreamPacketDescription *__nullable *__nullable
        outDataPacketDescription,
    void *__nullable inUserData) {
  // NSLog(@"%s", __func__);
  ViewController *viewcontroller = (__bridge ViewController *)inUserData;

  int oldreadpos = viewcontroller.readpos;
  int len = 0;
  {
    std::lock_guard<std::mutex> __lockguard(pcmmutex);
    if (viewcontroller.readpos + *ioNumberDataPackets >
        MAXPCMBUFSIZE) { //注意是[start, end)区间，所以是>
      len = MAXPCMBUFSIZE - viewcontroller.readpos;
    } else {
      len = *ioNumberDataPackets;
    }
    len = std::min(len, viewcontroller.datalen);
    viewcontroller.datalen -= len;
    viewcontroller.readpos += len;
    viewcontroller.readpos %= MAXPCMBUFSIZE;
  }

  // NSLog(@"------readpos : %u, len : %u, datalen : %u", oldreadpos, len,
  //       viewcontroller.datalen);
  ioData->mNumberBuffers = 1;
  ioData->mBuffers[0].mNumberChannels = INCHANNELS;
  ioData->mBuffers[0].mData = viewcontroller.pcmbuf + oldreadpos;
  ioData->mBuffers[0].mDataByteSize = len;
  *ioNumberDataPackets = 1; //⚠️
  return noErr;
}

//获取麦克风采集回调
void AudioQueueInputCB(
    void *__nullable inUserData, AudioQueueRef inAQ,
    AudioQueueBufferRef inBuffer, const AudioTimeStamp *inStartTime,
    UInt32 inNumberPacketDescriptions,
    const AudioStreamPacketDescription *__nullable inPacketDescs) {
  // NSLog(@"%s", __func__);
  OSStatus ret;
  ViewController *viewcontroller = (__bridge ViewController *)inUserData;
  // NSLog(@"pcm len : %d", inBuffer->mAudioDataByteSize);
  fwrite(inBuffer->mAudioData, inBuffer->mAudioDataByteSize, 1,
         viewcontroller.hpcm);
  fflush(viewcontroller.hpcm);

  int len1 = 0;
  int len2 = 0;
  {
    std::lock_guard<std::mutex> __lockguard(pcmmutex);
    if (viewcontroller.writepos + inBuffer->mAudioDataByteSize >
        MAXPCMBUFSIZE) {
      len1 = MAXPCMBUFSIZE - viewcontroller.writepos;
      len2 = inBuffer->mAudioDataByteSize - len2;
    } else {
      len1 = inBuffer->mAudioDataByteSize;
    }
    // NSLog(@"++++++writepos : %u, len : %u, datalen : %u",
    //       viewcontroller.writepos, len1 + len2, viewcontroller.datalen);
    memcpy(viewcontroller.pcmbuf + viewcontroller.writepos,
           inBuffer->mAudioData, len1);
    memcpy(viewcontroller.pcmbuf, (char *)inBuffer->mAudioData + len1, len2);
    viewcontroller.datalen += inBuffer->mAudioDataByteSize;
    viewcontroller.writepos += len1 + len2;
    viewcontroller.writepos %= MAXPCMBUFSIZE;
  }

  ret = AudioQueueEnqueueBuffer(viewcontroller.queue, inBuffer, 0, nil);
  PRINTSTATUS(ret);
}

- (void)encoder {
  while (!_stop) {
    OSStatus ret;
    while (!_stop) {
      bool retry = false;
      {
        std::lock_guard<std::mutex> __lockguard(pcmmutex);
        if (_datalen <= 1024 * 4) {
          retry = true;
        }
      }
      if (retry) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        continue;
      } else {
        break;
      }
    }
    if (_aacbuf == nullptr) {
      _aacbuf = malloc(_maxpackagesize);
    }

    UInt32 outputDataPackets = 1;
    AudioBufferList outputData;
    outputData.mNumberBuffers = 1;
    outputData.mBuffers[0].mNumberChannels = 2; // 输出类型声道数
    outputData.mBuffers[0].mDataByteSize = _maxpackagesize;
    outputData.mBuffers[0].mData = _aacbuf;
    AudioStreamPacketDescription outformat = {0};
    ret = AudioConverterFillComplexBuffer(
        _acoverter, AudioConverterComplexInputDataCB, (__bridge void *)self,
        &outputDataPackets, &outputData, &outformat);
    PRINTSTATUS(ret);
    NSData *aac = [NSData dataWithBytes:outputData.mBuffers[0].mData
                                 length:outputData.mBuffers[0].mDataByteSize];
    if (aac.length > 0) {
      NSData *adts = [self adtsDataForPacketLength:aac.length];
      fwrite(adts.bytes, adts.length, 1, _haac);
      // NSLog(@"aac len : %lu", (unsigned long)aac.length);
      fwrite(aac.bytes, aac.length, 1, _haac);
      fflush(_haac);
    }
  }
}

- (int)initMicrophone {
  NSLog(@"%s", __func__);

  //文件备份
  NSArray *docpath = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
                                                         NSUserDomainMask, YES);
  NSString *wdocpath = [docpath lastObject];
  NSString *pcmfilename = [wdocpath stringByAppendingPathComponent:@"save.pcm"];
  NSLog(@"pcm filename:%@", pcmfilename);
  _hpcm = fopen([pcmfilename UTF8String], "wb");
  NSString *aacfilename = [wdocpath stringByAppendingPathComponent:@"save.aac"];
  NSLog(@"finename:%@", aacfilename);
  _haac = fopen([aacfilename UTF8String], "wb");

  // PCM数据缓冲区
  _pcmbuf = (char *)malloc(MAXPCMBUFSIZE);

  OSStatus ret;
  //设置输入pcm音频参数
  _informat.mSampleRate = INSAMPLERATE;             //采样率
  _informat.mFormatID = kAudioFormatLinearPCM;      //音频格式
  _informat.mChannelsPerFrame = INCHANNELS;         //通道数
  _informat.mBitsPerChannel = 8 * sizeof(uint16_t); //每通道数据位深度
  /*
  如果选择了kLinearPCMFormatFlagIsPacked,则mBytesPerFrame必须等于mChannelsPerFramem*mBitsPerChannel/8
  如果没有设置，则可以设置为任意n*mChannelsPerFrame*mBitsPerChannel/8,为一个完整声道的整数倍数
  */
  _informat.mFormatFlags =
      kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
  _informat.mBytesPerFrame =
      _informat.mChannelsPerFrame * _informat.mBitsPerChannel / 8; //每帧字节数
  _informat.mBytesPerPacket =
      _informat.mChannelsPerFrame * _informat.mBitsPerChannel / 8; //每包字节数
  _informat.mFramesPerPacket = 1; //每包帧数

  //设置输出aac音频编码参数
  _outformat.mFormatID = kAudioFormatMPEG4AAC;
  _outformat.mFormatFlags = kMPEG4Object_AAC_LC;
  _outformat.mSampleRate = OUTSAMPLERATE;
  _outformat.mChannelsPerFrame = OUTCHANNELS;
  _outformat.mFramesPerPacket =
      1024; //每个packet 的帧数 ，这是一个比较大的固定数值
  _outformat.mBytesPerFrame = 0; //每帧的大小  如果是压缩格式设置为0
  _outformat.mReserved = 0;      // 8字节对齐，填0

  uint32_t outformatsize = sizeof(_outformat);
  //填充输出的相关参数 测试 如果outputAudioDes参数填充完全的话,
  //是不需要调用下边的函数的
  ret = AudioFormatGetProperty(kAudioFormatProperty_FormatInfo, 0, nil,
                               &outformatsize, &_outformat);
  PRINTSTATUS(ret);
  //创建编码器
  ret = AudioConverterNew(&_informat, &_outformat, &_acoverter);
  PRINTSTATUS(ret);

  //设置码率
  uint32_t bitratesize = sizeof(BITRATE);
  ret = AudioConverterSetProperty(_acoverter, kAudioConverterEncodeBitRate,
                                  bitratesize, &BITRATE);
  PRINTSTATUS(ret);
  uint32_t maxpacketsizesize = sizeof(_maxpackagesize);
  //获取最大包大小
  AudioConverterGetProperty(_acoverter,
                            kAudioConverterPropertyMaximumOutputPacketSize,
                            &maxpacketsizesize, &_maxpackagesize);
  NSLog(@"Expected BitRate is %@, Max Output PacketSize is %d", @(BITRATE),
        _maxpackagesize);

  //创建音频队列
  ret = AudioQueueNewInput(&_informat, AudioQueueInputCB, (__bridge void *)self,
                           nil, kCFRunLoopCommonModes, 0, &_queue);
  PRINTSTATUS(ret);
  AudioQueueBufferRef buffer[BUFCOUNT];
  for (int i = 0; i < sizeof(buffer) / sizeof(buffer[0]); ++i) {
    NSLog(@"buffer %i create", i);
    //创建音频队列的音频缓冲区
    ret = AudioQueueAllocateBuffer(_queue, BUFSIZE, &buffer[i]);
    PRINTSTATUS(ret);
    //将音频队列缓冲区添加到缓冲区队列的末尾
    ret = AudioQueueEnqueueBuffer(_queue, buffer[i], 0, nil);
    PRINTSTATUS(ret);
  }
  //启动音频队列
  ret = AudioQueueStart(_queue, nil);
  PRINTSTATUS(ret);

  _stop = false;
  dispatch_queue_t encoderQueue =
      dispatch_queue_create("AAC Encoder Queue", DISPATCH_QUEUE_SERIAL);
  dispatch_async(encoderQueue, ^{
    [self encoder];
  });

  return 0;
}

- (int)uninitMicrophone {
  NSLog(@"%s", __func__);
  OSStatus ret;
  _stop = true;
  //停止音频队列
  ret = AudioQueueStop(_queue, true);
  PRINTSTATUS(ret);
  //销毁音频队列资源，包括缓冲区
  ret = AudioQueueDispose(_queue, true);
  PRINTSTATUS(ret);
  //销毁编码器
  ret = AudioConverterDispose(_acoverter);
  PRINTSTATUS(ret);

  fclose(_hpcm);
  fclose(_haac);
  return 0;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  // Do any additional setup after loading the view.
  [self initMicrophone];
}

- (void)viewWillDisappear:(BOOL)animated {
  NSLog(@"%s", __func__);
  [self uninitMicrophone];
}

@end
