#ifndef AIDL_GENERATED_ANDROID_BP_GRAPHIC_BUFFER_SOURCE_H_
#define AIDL_GENERATED_ANDROID_BP_GRAPHIC_BUFFER_SOURCE_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <android/IGraphicBufferSource.h>

namespace android {

class BpGraphicBufferSource : public ::android::BpInterface<IGraphicBufferSource> {
public:
  explicit BpGraphicBufferSource(const ::android::sp<::android::IBinder>& _aidl_impl);
  virtual ~BpGraphicBufferSource() = default;
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
};  // class BpGraphicBufferSource

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_BP_GRAPHIC_BUFFER_SOURCE_H_
