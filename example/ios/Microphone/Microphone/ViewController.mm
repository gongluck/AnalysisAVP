//
//  ViewController.m
//  Microphone
//
//  Created by ldyun-mac on 2022/6/15.
//

//参考https://www.jianshu.com/p/cce99eb216db

#import "ViewController.h"
#import <AudioToolbox/AudioToolbox.h>
#include <thread>

@interface ViewController ()

@end

@implementation ViewController

AudioQueueRef queue;//音频队列
FILE* hpcm = nullptr;//文件句柄
const int BUFCOUNT = 3;//音频缓冲区数
const int BUFSIZE = 1024;//音频缓冲区大小

#define PRINTSTATUS(ret) \
if(ret != noErr) \
{\
NSLog(@"error : %d in %s(%d)", ret, __FILE__, __LINE__);\
}

void AudioQueueInputCB(
                       void * __nullable               inUserData,
                       AudioQueueRef                   inAQ,
                       AudioQueueBufferRef             inBuffer,
                       const AudioTimeStamp *          inStartTime,
                       UInt32                          inNumberPacketDescriptions,
                       const AudioStreamPacketDescription * __nullable inPacketDescs)
{
    //NSLog(@"%s", __func__);
    if(hpcm == nullptr)
    {
        NSArray *docpath = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString *wdocpath = [docpath lastObject];
        NSString *filename = [wdocpath stringByAppendingPathComponent:@"save.pcm"];
        NSLog(@"finename:%@", filename);
        hpcm = fopen([filename UTF8String], "wb");
    }
    fwrite(inBuffer->mAudioData, inBuffer->mAudioDataByteSize, 1, hpcm);
    fflush(hpcm);
    
    OSStatus ret = AudioQueueEnqueueBuffer(queue, inBuffer, 0, nil);
    PRINTSTATUS(ret);
}

-(int)initMicrophone {
    NSLog(@"%s", __func__);
    OSStatus ret;
    //设置音频参数
    AudioStreamBasicDescription format;
    format.mSampleRate = 44100;//采样率
    format.mFormatID = kAudioFormatLinearPCM;//音频格式
    format.mChannelsPerFrame = 2;//通道数
    format.mBitsPerChannel = 16;//每通道数据位深度
    format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    format.mBytesPerFrame = format.mChannelsPerFrame * format.mBitsPerChannel / 8;//每帧字节数
    format.mBytesPerPacket = format.mChannelsPerFrame * format.mBitsPerChannel / 8;//每包字节数
    format.mFramesPerPacket = 1;//每包帧数
    //创建音频队列
    ret = AudioQueueNewInput(&format, AudioQueueInputCB, nil, nil, kCFRunLoopCommonModes, 0, &queue);
    PRINTSTATUS(ret);
    AudioQueueBufferRef buffer[BUFCOUNT];
    for (int i=0;i<sizeof(buffer) / sizeof(buffer[0]);++i)
    {
        NSLog(@"buffer %i create", i);
        //创建音频队列的音频缓冲区
        ret = AudioQueueAllocateBuffer(queue, BUFSIZE, &buffer[i]);
        PRINTSTATUS(ret);
        //将音频队列缓冲区添加到缓冲区队列的末尾
        ret = AudioQueueEnqueueBuffer(queue, buffer[i], 0, nil);
        PRINTSTATUS(ret);
    }
    //启动音频队列
    ret = AudioQueueStart(queue, nil);
    PRINTSTATUS(ret);
    return 0;
}

-(int)uninitMicrophone{
    NSLog(@"%s", __func__);
    OSStatus ret;
    //停止音频队列
    ret = AudioQueueStop(queue, true);
    PRINTSTATUS(ret);
    //销毁音频队列资源，包括缓冲区
    ret = AudioQueueDispose(queue, true);
    PRINTSTATUS(ret);
    return 0;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    [self initMicrophone];
}

-(void)viewWillDisappear:(BOOL)animated{
    NSLog(@"%s", __func__);
    [self uninitMicrophone];
}

@end
