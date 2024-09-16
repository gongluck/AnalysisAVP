#ifndef AIDL_GENERATED_ANDROID_I_O_M_X_NODE_H_
#define AIDL_GENERATED_ANDROID_I_O_M_X_NODE_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <utils/StrongPointer.h>

namespace android {

class IOMXNode : public ::android::IInterface {
public:
  DECLARE_META_INTERFACE(OMXNode)
};  // class IOMXNode

class IOMXNodeDefault : public IOMXNode {
public:
  ::android::IBinder* onAsBinder() override;
  
};

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_I_O_M_X_NODE_H_
