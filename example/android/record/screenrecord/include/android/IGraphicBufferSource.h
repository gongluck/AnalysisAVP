#ifndef AIDL_GENERATED_ANDROID_I_GRAPHIC_BUFFER_SOURCE_H_
#define AIDL_GENERATED_ANDROID_I_GRAPHIC_BUFFER_SOURCE_H_

#include <android/IOMXNode.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <cstdint>
#include <utils/StrongPointer.h>

namespace android {

class IGraphicBufferSource : public ::android::IInterface {
public:
  DECLARE_META_INTERFACE(GraphicBufferSource)
  virtual ::android::binder::Status configure(const ::android::sp<::android::IOMXNode>& omxNode, int32_t dataSpace) = 0;
  virtual ::android::binder::Status setSuspend(bool suspend, int64_t suspendTimeUs) = 0;
  virtual ::android::binder::Status setRepeatPreviousFrameDelayUs(int64_t repeatAfterUs) = 0;
  virtual ::android::binder::Status setMaxFps(float maxFps) = 0;
  virtual ::android::binder::Status setTimeLapseConfig(double fps, double captureFps) = 0;
  virtual ::android::binder::Status setStartTimeUs(int64_t startTimeUs) = 0;
  virtual ::android::binder::Status setStopTimeUs(int64_t stopTimeUs) = 0;
  virtual ::android::binder::Status getStopTimeOffsetUs(int64_t* _aidl_return) = 0;
  virtual ::android::binder::Status setColorAspects(int32_t aspects) = 0;
  virtual ::android::binder::Status setTimeOffsetUs(int64_t timeOffsetsUs) = 0;
  virtual ::android::binder::Status signalEndOfInputStream() = 0;
};  // class IGraphicBufferSource

class IGraphicBufferSourceDefault : public IGraphicBufferSource {
public:
  ::android::IBinder* onAsBinder() override;
  ::android::binder::Status configure(const ::android::sp<::android::IOMXNode>& omxNode, int32_t dataSpace) override;
  ::android::binder::Status setSuspend(bool suspend, int64_t suspendTimeUs) override;
  ::android::binder::Status setRepeatPreviousFrameDelayUs(int64_t repeatAfterUs) override;
  ::android::binder::Status setMaxFps(float maxFps) override;
  ::android::binder::Status setTimeLapseConfig(double fps, double captureFps) override;
  ::android::binder::Status setStartTimeUs(int64_t startTimeUs) override;
  ::android::binder::Status setStopTimeUs(int64_t stopTimeUs) override;
  ::android::binder::Status getStopTimeOffsetUs(int64_t* _aidl_return) override;
  ::android::binder::Status setColorAspects(int32_t aspects) override;
  ::android::binder::Status setTimeOffsetUs(int64_t timeOffsetsUs) override;
  ::android::binder::Status signalEndOfInputStream() override;

};

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_I_GRAPHIC_BUFFER_SOURCE_H_
