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

#import "ViewController.h"
#import <AVFoundation/AVFoundation.h>

//实现<AVCaptureVideoDataOutputSampleBufferDelegate>的接口以获取数据
@interface ViewController () <AVCaptureVideoDataOutputSampleBufferDelegate>
//属性
// nonatomic非原子保护，无需靠自生框架考虑多线程情况
// strong强引用，与shared_ptr类似
@property(nonatomic,strong)AVCaptureSession* captureSession;//采集会话
@property(nonatomic,strong)AVCaptureDeviceInput* input;
@property(nonatomic,strong)AVCaptureVideoDataOutput* output;
@property(nonatomic,strong)dispatch_queue_t dataCallbackQueue;
@end

#define FOURCC2STR(fourcc) [[NSString alloc] initWithUTF8String:(const char[]){*(((char*)&fourcc)+3), *(((char*)&fourcc)+2), *(((char*)&fourcc)+1), *(((char*)&fourcc)+0),0}]

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    
    NSError* error;
    //获取摄像头设备对象
    AVCaptureDevice * device;
    NSArray<AVCaptureDevice *> * devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for(AVCaptureDevice * dev in devices)
    {
        NSLog(@"device : %@", dev);
        device = dev;
        if([dev position] == AVCaptureDevicePositionFront)
        {
            //device = dev;
            //break;
        }
    }
    //设置摄像头帧率，作用不大
    CMTime frameDuration = CMTimeMake(1, 30);
    for (AVFrameRateRange *range in [device.activeFormat videoSupportedFrameRateRanges]) {
        NSLog(@"support framerate:%@", range);
        if (CMTIME_COMPARE_INLINE(frameDuration, >=, range.minFrameDuration) &&
            CMTIME_COMPARE_INLINE(frameDuration, <=, range.maxFrameDuration)) {
            NSLog(@"select framerate:%@", range);
            if ([device lockForConfiguration:&error]) {
                [device setActiveVideoMaxFrameDuration:range.minFrameDuration];
                [device setActiveVideoMinFrameDuration:range.maxFrameDuration];
                [device unlockForConfiguration];
            }
        }
    }
    
    //创建输入
    _input = [[AVCaptureDeviceInput alloc] initWithDevice: device error:&error];
    if (error) {
        NSLog(@"create input failed,%@",error);
        return;
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
    //设置输出格式
    // 获取输出对象所支持的像素格式
    NSArray *supportedPixelFormats = _output.availableVideoCVPixelFormatTypes;
    for (NSNumber *currentPixelFormat in supportedPixelFormats)  {
        NSLog(@"support format : %@", currentPixelFormat);
    }
    [_output setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:kCVPixelFormatType_420YpCbCr8BiPlanarFullRange] forKey:(id)kCVPixelBufferPixelFormatTypeKey]];

    //创建功能会话对象
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
    AVCaptureVideoPreviewLayer *previewLayer = [AVCaptureVideoPreviewLayer layerWithSession:_captureSession];
    previewLayer.frame = self.view.bounds;
    [self.view.layer  addSublayer:previewLayer];
    //提交配置变更
    [_captureSession commitConfiguration];
    
    [_captureSession startRunning];
//    if([_captureSession isRunning])
//    {
//        [_captureSession stopRunning];
//    }
}

#pragma mark - AVCaptureVideoDataOutputSampleBufferDelegate
- (void)captureOutput:(AVCaptureOutput *)output
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection
{
    static int64_t inc = 0;
    if(inc++ % 100 == 0)
    {
        NSLog(@"got frame");
        return;
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
//    NSFileHandle *fileHandle = [NSFileHandle fileHandleForUpdatingAtPath:filename];
//    [fileHandle seekToEndOfFile];
//    NSLog(@"filename : %@", filename);
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
//                NSData* ydata = [NSData dataWithBytes : Y length : stride * height];
//                NSData* uvdata = [NSData dataWithBytes : UV length : stride * height / 2];
//                NSLog(@"frame size %lu",stride * height*3/2);
//                [fileHandle writeData:ydata];
//                [fileHandle writeData:uvdata];
//            }
//        }
//    }else{
//        void* YUV = CVPixelBufferGetBaseAddress(imageBuffer);
//        size_t size = CVPixelBufferGetDataSize(imageBuffer);
//        NSLog(@"frame size %lu",size);
//        NSData* yuvdata = [NSData dataWithBytes : YUV length : size];
//        [fileHandle writeData:yuvdata];
//    }
//    //unlock
//    CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
//    [fileHandle closeFile];
}

@end
