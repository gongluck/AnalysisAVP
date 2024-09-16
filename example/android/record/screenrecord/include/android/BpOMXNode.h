#ifndef AIDL_GENERATED_ANDROID_BP_O_M_X_NODE_H_
#define AIDL_GENERATED_ANDROID_BP_O_M_X_NODE_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <android/IOMXNode.h>

namespace android {

class BpOMXNode : public ::android::BpInterface<IOMXNode> {
public:
  explicit BpOMXNode(const ::android::sp<::android::IBinder>& _aidl_impl);
  virtual ~BpOMXNode() = default;
};  // class BpOMXNode

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_BP_O_M_X_NODE_H_
