//
//  ViewController.m
//  iosCamera
//
//  Created by ldyun-mac on 2021/12/22.
//

//参考
//https://www.cnblogs.com/lijinfu-software/articles/11451340.html
//https://www.jianshu.com/p/e75d7b573ae5?utm_campaign=maleskine&utm_content=note&utm_medium=seo_notes&utm_source=recommendation
//https://www.jianshu.com/p/a0e2d7b3b8a7
//https://www.jianshu.com/p/f5f3f94f36c5
//https://www.cnblogs.com/ziyi--caolu/p/8038968.html

#import "ViewController.h"
#import <AVFoundation/AVFoundation.h>
#import <VideoToolbox/VideoToolbox.h>

//实现<AVCaptureVideoDataOutputSampleBufferDelegate>的接口以获取数据
@interface ViewController () <AVCaptureVideoDataOutputSampleBufferDelegate>
//属性
// nonatomic非原子保护，不靠自生框架考虑多线程情况
// strong强引用，与shared_ptr类似
@property(nonatomic)int32_t width;
@property(nonatomic)int32_t height;
@property(nonatomic)int32_t fps;
@property(nonatomic)int32_t bitrate;
@property(nonatomic)NSInteger lastime;//记录采集时间
@property(nonatomic)NSInteger firstime;//记录编码首帧时间
@property(nonatomic)bool facing;//是否使用前置摄像头
@property(nonatomic,strong)AVCaptureSession* captureSession;//采集会话
@property(nonatomic,strong)AVCaptureDeviceInput* input;
@property(nonatomic,strong)AVCaptureVideoDataOutput* output;
@property(nonatomic,strong)dispatch_queue_t dataCallbackQueue;
// assign简单赋值，不更改引用计数，不考虑内存管理；常常用于基础数据类型
@property(nonatomic,assign)VTCompressionSessionRef compressionSession;//编码会话
@property(nonatomic)FILE* hfile;//保存文件
@end

//FOURCHARCODE to NSString
#define FOURCC2STR(fourcc) [[NSString alloc] initWithUTF8String:(const char[]){*(((char*)&fourcc)+3), *(((char*)&fourcc)+2), *(((char*)&fourcc)+1), *(((char*)&fourcc)+0),0}]

@implementation ViewController

//采集参数设置
-(int)doCapturePrepare{
    NSError* error;
    //获取摄像头设备对象
    AVCaptureDevice * device;
    NSArray<AVCaptureDevice *> *devices;
    AVCaptureDevicePosition position = _facing ? AVCaptureDevicePositionFront : AVCaptureDevicePositionBack;
    if (@available(iOS 10.0, *)) {
        AVCaptureDeviceDiscoverySession *deviceDiscoverySession =  [AVCaptureDeviceDiscoverySession discoverySessionWithDeviceTypes:@[AVCaptureDeviceTypeBuiltInWideAngleCamera] mediaType:AVMediaTypeVideo position:position];
        devices = deviceDiscoverySession.devices;
    } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
#pragma clang diagnostic pop
    }
    for(AVCaptureDevice * dev in devices)
    {
        NSLog(@"device : %@", dev);
        if([dev position] == position)
        {
            device = dev;
            break;
        }
    }
    //设置摄像头帧率，作用不大
    CMTime frameDuration = CMTimeMake(1, 30);
    for (AVFrameRateRange *range in [device.activeFormat videoSupportedFrameRateRanges]) {
        NSLog(@"support framerate:%@", range);
        if (CMTIME_COMPARE_INLINE(frameDuration, >=, range.minFrameDuration) &&
            CMTIME_COMPARE_INLINE(frameDuration, <=, range.maxFrameDuration)) {
            if ([device lockForConfiguration:&error]) {
                [device setActiveVideoMaxFrameDuration:range.minFrameDuration];
                [device setActiveVideoMinFrameDuration:range.maxFrameDuration];
                [device unlockForConfiguration];
                NSLog(@"select framerate:%@", range);
            }
        }
    }
    //创建输入
    _input = [[AVCaptureDeviceInput alloc] initWithDevice: device error:&error];
    if (error) {
        NSLog(@"create input failed,%@",error);
        return -1;
    }else{
        NSLog(@"create input succeed.");
    }
    //创建输出队列
    //DISPATCH_QUEUE_SERIAL串行队列
    //_dataCallbackQueue = dispatch_queue_create("dataCallbackQueue", DISPATCH_QUEUE_SERIAL);
    //创建数据获取线程
    _dataCallbackQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    //创建输出
    _output = [[AVCaptureVideoDataOutput alloc] init];
    //绑定输出队列和代理到输出对象
    [_output setSampleBufferDelegate:self queue:_dataCallbackQueue];
    //抛弃过期帧，保证实时性
    [_output setAlwaysDiscardsLateVideoFrames:YES];
    //获取输出对象所支持的像素格式
    NSArray *supportedPixelFormats = _output.availableVideoCVPixelFormatTypes;
    for (NSNumber *currentPixelFormat in supportedPixelFormats)  {
        NSLog(@"support format : %@", currentPixelFormat);
    }
    //设置输出格式
    [_output setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:kCVPixelFormatType_420YpCbCr8BiPlanarFullRange] forKey:(id)kCVPixelBufferPixelFormatTypeKey]];
    //创建采集功能会话对象
    _captureSession = [[AVCaptureSession alloc] init];
    // 改变会话的配置前一定要先开启配置，配置完成后提交配置改变
    [_captureSession beginConfiguration];
    //设置采集参数
    if([_captureSession canSetSessionPreset:AVCaptureSessionPreset640x480])
    {
        [_captureSession setSessionPreset:AVCaptureSessionPreset640x480];
    }
    //绑定input和output到session
    NSLog(@"input : %@", _input);
    if([_captureSession canAddInput:_input])
    {
        [_captureSession addInput:_input];
    }
    NSLog(@"output : %@", _output);
    if([_captureSession canAddOutput:_output])
    {
        [_captureSession addOutput: _output];
    }
    //显示输出画面
    AVCaptureVideoPreviewLayer *previewLayer = [AVCaptureVideoPreviewLayer layerWithSession:_captureSession];
    previewLayer.frame = CGRectMake(0, 50, self.view.frame.size.width, self.view.frame.size.height - 50);
    [self.view.layer  addSublayer:previewLayer];
    //提交配置变更
    [_captureSession commitConfiguration];
    return 0;
}
//开始采集
-(int)doStartCapture{
    if(_captureSession != NULL && ![_captureSession isRunning])
    {
        [_captureSession startRunning];
        if([_captureSession isRunning])
        {
            NSLog(@"start capture succeed.");
            return 0;
        }
        else
        {
            return -1;
        }
    }
    return 0;
}
//停止采集
-(int)doStopCapture{
    if(_captureSession != NULL && [_captureSession isRunning])
    {
        [_captureSession stopRunning];
        if(![_captureSession isRunning])
        {
            _captureSession = NULL;
            NSLog(@"stop capture succeed.");
            return 0;
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

//编码参数设置
-(int)doEncodePrepare{
    if([self doEncodeDestroy]!=0)
    {
        NSLog(@"doEncodeDestroy failed.");
        return -1;
    }
    //创建CompressionSession对象,该对象用于对画面进行编码
    OSStatus status = VTCompressionSessionCreate(NULL,      // 会话的分配器。传递NULL以使用默认分配器。
                                                 _width,    // 帧的宽度，以像素为单位。
                                                 _height,   // 帧的高度，以像素为单位。
                                                 kCMVideoCodecType_H264,   // 编解码器的类型,表示使用h.264进行编码
                                                 NULL,      // 指定必须使用的特定视频编码器。传递NULL让视频工具箱选择编码器。
                                                 NULL,      // 源像素缓冲区所需的属性，用于创建像素缓冲池。如果不希望视频工具箱为您创建一个，请传递NULL
                                                 NULL,      // 编码数据的分配器。传递NULL以使用默认分配器。
                                                 VTCompressionOutputCallbackH264,   // 当一次编码结束会在该函数进行回调,可以在该函数中将数据,写入文件中
                                                 (__bridge  void*)self,  // outputCallbackRefCon
                                                 &_compressionSession);    // 指向一个变量以接收的编码会话。
    if (status != noErr){
        NSLog(@"VTCompressionSessionCreate failed : %d", status);
        return -1;
    }
    //设置实时编码输出（直播必然是实时输出,否则会有延迟）
    status = VTSessionSetProperty(_compressionSession, kVTCompressionPropertyKey_RealTime, kCFBooleanTrue);
    if (status != noErr){
        NSLog(@"kVTCompressionPropertyKey_RealTime failed : %d", status);
        return -1;
    }
    //设置profile
    status = VTSessionSetProperty(_compressionSession, kVTCompressionPropertyKey_ProfileLevel, kVTProfileLevel_H264_Baseline_AutoLevel);
    if (status != noErr){
        NSLog(@"kVTCompressionPropertyKey_ProfileLevel failed : %d", status);
        return -1;
    }
    //关闭重排,可以关闭B帧。
    status = VTSessionSetProperty(_compressionSession, kVTCompressionPropertyKey_AllowFrameReordering, kCFBooleanFalse);
    if (status != noErr){
        NSLog(@"kVTCompressionPropertyKey_AllowFrameReordering failed : %d", status);
        return -1;
    }
    //设置gop
    status = VTSessionSetProperty(_compressionSession, kVTCompressionPropertyKey_MaxKeyFrameInterval, (__bridge CFTypeRef)(@(_fps*10)));
    if (status != noErr){
        NSLog(@"kVTCompressionPropertyKey_MaxKeyFrameInterval failed : %d", status);
        return -1;
    }
    //设置帧率
    status = VTSessionSetProperty(_compressionSession, kVTCompressionPropertyKey_ExpectedFrameRate, (__bridge CFTypeRef)(@(_fps)));
    if (status != noErr){
        NSLog(@"kVTCompressionPropertyKey_ExpectedFrameRate failed : %d", status);
        return -1;
    }
    //设置码率kVTCompressionPropertyKey_AverageBitRate/kVTCompressionPropertyKey_DataRateLimits
    status = VTSessionSetProperty(_compressionSession, kVTCompressionPropertyKey_AverageBitRate, (__bridge CFTypeRef)(@(_bitrate)));
    if (status != noErr){
        NSLog(@"kVTCompressionPropertyKey_AverageBitRate failed : %d", status);
        return -1;
    }
    status = VTSessionSetProperty(_compressionSession, kVTCompressionPropertyKey_DataRateLimits, (__bridge CFArrayRef)@[@1.0]);
    if (status != noErr){
        NSLog(@"kVTCompressionPropertyKey_DataRateLimits failed : %d", status);
        return -1;
    }
    //基本设置结束, 准备进行编码
    status = VTCompressionSessionPrepareToEncodeFrames(_compressionSession);
    if (status != noErr){
        NSLog(@"VTCompressionSessionPrepareToEncodeFrames failed : %d", status);
        return -1;
    }
    return 0;
}
//销毁编码设置
-(int)doEncodeDestroy{
    if(_compressionSession != NULL)
    {
        VTCompressionSessionInvalidate(_compressionSession);
        CFRelease(_compressionSession);
        _compressionSession = NULL;
    }
    return 0;
}

- (IBAction)touchbtn:(UIButton *)sender {
    if([sender.titleLabel.text  isEqual: @"start"])
    {
        if([self doStopCapture] != 0 || [self doEncodePrepare] != 0 || [self doCapturePrepare] != 0 || [self doStartCapture] != 0)
        {
            return;
        }
        if(_hfile == NULL)
        {
            NSArray *docpath = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
            NSString *wdocpath = [docpath lastObject];
            NSString *filename = [wdocpath stringByAppendingPathComponent:@"save.h264"];
            NSLog(@"finename:%@", filename);
            _hfile = fopen([filename UTF8String], "wb");
        }
        [sender setTitle:@"stop" forState:UIControlStateNormal];
    }
    else if([sender.titleLabel.text  isEqual: @"stop"])
    {
        if([self doStopCapture] != 0 || [self doEncodeDestroy] != 0)
        {
            return;
        }
        _lastime = -1;
        _firstime = -1;
        if(_hfile != NULL)
        {
            fclose(_hfile);
            _hfile = NULL;
        }
        [sender setTitle:@"start" forState:UIControlStateNormal];
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    _width = 640;
    _height = 480;
    _fps = 30;
    _bitrate = _width * _height;
    _lastime = -1;
    _firstime = -1;
    _facing = true;
    _hfile = NULL;
}

#pragma mark - AVCaptureVideoDataOutputSampleBufferDelegate
- (void)captureOutput:(AVCaptureOutput *)output
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection
{
    int64_t cur = CFAbsoluteTimeGetCurrent() * 1000;//ms
    if(_lastime == -1)
    {
        _lastime = cur;
    }
    int64_t went = cur - _lastime;
    int64_t duration = 1000/_fps;
    //NSLog(@"duration:%ld", duration);
    //NSLog(@"last:%ld,cur:%ld,went:%ld", last, cur, went);
    if(went < duration)
    {
        NSLog(@"drop");
        return;
    }else{
        _lastime = cur - went % duration;
    }
    //NSLog(@"captureOutput:%@,%@,%@", output, sampleBuffer, connection);
    CMVideoFormatDescriptionRef description = CMSampleBufferGetFormatDescription(sampleBuffer);
    //NSLog(@"captureOutput:%@", description);
    if(CMFormatDescriptionGetMediaType(description) != kCMMediaType_Video)
    {
        return;
    }
    //FourCharCode codectype = CMVideoFormatDescriptionGetCodecType(description);
    //NSString *scodectype = FOURCC2STR(codectype);
    //NSLog(@"codec type:%@", scodectype);
    
    //    NSArray *docpath = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    //    NSString *wdocpath = [docpath lastObject];
    //    NSString *filename = [wdocpath stringByAppendingPathComponent:@"save.yuv"];
    //    NSLog(@"filename : %@", filename);
    //    //使用C函数写文件
    //    FILE* hfile = fopen([filename UTF8String], "ab+");
    //    //lock
    //    //CVPixelBufferRef是CVImageBufferRef的别名，两者操作几乎一致。
    //    CVPixelBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    //    //CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    //    //需先用CVPixelBufferLockBaseAddress()锁定地址才能从主存访问，否则调用CVPixelBufferGetBaseAddressOfPlane等函数则返回NULL或无效值。
    //    CVPixelBufferLockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);
    //    //NSLog(@"imageBuffer:%@", imageBuffer);
    //    if(CVPixelBufferIsPlanar(imageBuffer))
    //    {
    //        size_t planars = CVPixelBufferGetPlaneCount(imageBuffer);
    //        if(planars == 2)
    //        {
    //            size_t stride = CVPixelBufferGetBytesPerRowOfPlane(imageBuffer, 0);
    //            size_t width = CVPixelBufferGetWidthOfPlane(imageBuffer, 0);
    //            size_t height = CVPixelBufferGetHeightOfPlane(imageBuffer, 0);
    //            NSLog(@"buffer stride : %ld, w : %ld, h : %ld", stride, width, height);
    //            void* Y = CVPixelBufferGetBaseAddressOfPlane(imageBuffer, 0);
    //            void* UV = CVPixelBufferGetBaseAddressOfPlane(imageBuffer, 1);
    //            if(Y != nil && UV != nil)
    //            {
    //                NSLog(@"frame size %lu",stride * height*3/2);
    //                fwrite(Y, 1, stride * height, hfile);
    //                fwrite(UV, 1, stride * height / 2, hfile);
    //            }
    //        }
    //    }else{
    //        void* YUV = CVPixelBufferGetBaseAddress(imageBuffer);
    //        size_t size = CVPixelBufferGetDataSize(imageBuffer);
    //        NSLog(@"frame size %lu",size);
    //        fwrite(YUV, 1, size, hfile);
    //    }
    //    fclose(hfile);
    //    //unlock
    //    CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
    
    //编码264
    //将sampleBuffer转成imageBuffer
    CVImageBufferRef imageBuffer = (CVImageBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
    if(_firstime == -1)
    {
        _firstime = cur;
    }
    //创建CMTime的pts和duration
    CMTime pts = CMTimeMake(cur - _firstime, 1000);//ms
    CMTime dur = CMTimeMake(1, _fps);//ms
    VTEncodeInfoFlags flags;
    //NSLog(@"input pts : %lf", CMTimeGetSeconds(pts));
    //开始编码该帧数据
    OSStatus statusCode = VTCompressionSessionEncodeFrame(
                                                          _compressionSession,
                                                          imageBuffer,
                                                          pts,
                                                          dur,
                                                          NULL,
                                                          NULL,
                                                          &flags
                                                          );
    if (statusCode != noErr) {
        NSLog(@"VTCompressionSessionEncodeFrame failed %d", statusCode);
        [self doEncodeDestroy];
    }
}

void VTCompressionOutputCallbackH264(void * CM_NULLABLE outputCallbackRefCon,       //自定义回调参数
                                     void * CM_NULLABLE sourceFrameRefCon,
                                     OSStatus status,
                                     VTEncodeInfoFlags infoFlags,
                                     CM_NULLABLE CMSampleBufferRef sampleBuffer)
{
    if (status != noErr) {
        NSLog(@"encode error : %d", status);
        return;
    }
    if (!CMSampleBufferDataIsReady(sampleBuffer)) {
        NSLog(@"sampleBuffer data is not ready ");
        return;
    }
    ViewController* _self = (__bridge ViewController*)outputCallbackRefCon;
    //判断是否是关键帧
    bool isKeyframe = !CFDictionaryContainsKey((CFArrayGetValueAtIndex(CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, true), 0)), kCMSampleAttachmentKey_NotSync);
    if (isKeyframe)
    {
        // 获取编码后的信息（存储于CMFormatDescriptionRef中）
        CMFormatDescriptionRef format = CMSampleBufferGetFormatDescription(sampleBuffer);
        // 获取SPS信息
        size_t sparameterSetSize, sparameterSetCount;
        const uint8_t *sparameterSet;
        CMVideoFormatDescriptionGetH264ParameterSetAtIndex(format, 0, &sparameterSet, &sparameterSetSize, &sparameterSetCount, 0);
        // 获取PPS信息
        size_t pparameterSetSize, pparameterSetCount;
        const uint8_t *pparameterSet;
        CMVideoFormatDescriptionGetH264ParameterSetAtIndex(format, 1, &pparameterSet, &pparameterSetSize, &pparameterSetCount, 0);
        //NSLog(@"sps size : %d", sparameterSetSize);
        //NSLog(@"pps size : %d", pparameterSetSize);
        if(_self.hfile != NULL)
        {
            //NSLog(@"write file");
            char naluhead[4] = {0x00, 0x00, 0x00, 0x01};
            fwrite(naluhead, 1, 4, _self.hfile);
            fwrite(sparameterSet, 1, sparameterSetSize, _self.hfile);
            fwrite(naluhead, 1, 4, _self.hfile);
            fwrite(pparameterSet, 1, pparameterSetSize, _self.hfile);
            fflush(_self.hfile);
        }
    }
    //Float64 pts = CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(sampleBuffer));
    //NSLog(@"output pts : %lf", pts);
    // 获取数据块
    CMBlockBufferRef dataBuffer = CMSampleBufferGetDataBuffer(sampleBuffer);
    size_t length, total;
    char *data;
    OSStatus ret = CMBlockBufferGetDataPointer(dataBuffer, 0, &length, &total, &data);
    if (ret == noErr) {
        size_t offset = 0;
        static const int AVCCHeaderLength = 4; // 返回的nalu数据前四个字节不是0001的startcode，而是大端模式的帧长度length
        // 循环获取nalu数据
        while (offset < total - AVCCHeaderLength) {
            uint32_t nalulen = 0;
            // Read the NAL unit length
            memcpy(&nalulen, data + offset, AVCCHeaderLength);
            // 从大端转系统端
            nalulen = CFSwapInt32BigToHost(nalulen);
            //NSLog(@"nalu size : %d", nalulen);
            if(_self.hfile != NULL)
            {
                char naluhead[4] = {0x00, 0x00, 0x00, 0x01};
                fwrite(naluhead, 1, 4, _self.hfile);
                fwrite(data + offset + AVCCHeaderLength, 1, nalulen, _self.hfile);
                fflush(_self.hfile);
            }
            // 移动到写一个块，转成NALU单元
            // Move to the next NAL unit in the block buffer
            offset += AVCCHeaderLength + nalulen;
        }
    }
}

@end
