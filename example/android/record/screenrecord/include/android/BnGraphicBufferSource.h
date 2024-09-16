#ifndef AIDL_GENERATED_ANDROID_BN_GRAPHIC_BUFFER_SOURCE_H_
#define AIDL_GENERATED_ANDROID_BN_GRAPHIC_BUFFER_SOURCE_H_

#include <binder/IInterface.h>
#include <android/IGraphicBufferSource.h>

namespace android {

class BnGraphicBufferSource : public ::android::BnInterface<IGraphicBufferSource> {
public:
  ::android::status_t onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags) override;
};  // class BnGraphicBufferSource

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_BN_GRAPHIC_BUFFER_SOURCE_H_
