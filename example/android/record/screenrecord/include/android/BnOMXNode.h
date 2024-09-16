#ifndef AIDL_GENERATED_ANDROID_BN_O_M_X_NODE_H_
#define AIDL_GENERATED_ANDROID_BN_O_M_X_NODE_H_

#include <binder/IInterface.h>
#include <android/IOMXNode.h>

namespace android {

class BnOMXNode : public ::android::BnInterface<IOMXNode> {
public:
  ::android::status_t onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags) override;
};  // class BnOMXNode

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_BN_O_M_X_NODE_H_
