cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)

# 获取当前文件夹名
string(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})

project(${CURRENT_FOLDER})

# ##############################################################################
set(subject ${PROJECT_NAME})

include_directories(${subject} include/
  include/aidl

  ${ANDROID_PLATFORM_DIR}/system/core/include
  ${ANDROID_PLATFORM_DIR}/system/core/base/include

  ${ANDROID_PLATFORM_DIR}/frameworks/av/include
  ${ANDROID_PLATFORM_DIR}/frameworks/av/media/libstagefright
  ${ANDROID_PLATFORM_DIR}/frameworks/av/media/libstagefright/include
  ${ANDROID_PLATFORM_DIR}/frameworks/av/media/libmedia
  ${ANDROID_PLATFORM_DIR}/frameworks/av/media/libmedia/include

  ${ANDROID_PLATFORM_DIR}/frameworks/native/include
  ${ANDROID_PLATFORM_DIR}/frameworks/native/include/media/openmax
  ${ANDROID_PLATFORM_DIR}/frameworks/native/libs/math/include
  ${ANDROID_PLATFORM_DIR}/frameworks/native/libs/nativebase/include
  ${ANDROID_PLATFORM_DIR}/frameworks/native/libs/nativewindow/include
  ${ANDROID_PLATFORM_DIR}/frameworks/native/libs/binder/include
  ${ANDROID_PLATFORM_DIR}/frameworks/native/libs/gui/include
  ${ANDROID_PLATFORM_DIR}/frameworks/native/libs/ui/include

  ${ANDROID_PLATFORM_DIR}/prebuilts/vndk/v30/${PREBUILT}/include/system/libhidl/base/include
  ${ANDROID_PLATFORM_DIR}/prebuilts/vndk/v30/${PREBUILT}/include/hardware/libhardware/include
  ${ANDROID_PLATFORM_DIR}/prebuilts/vndk/v30/${PREBUILT}/include/system/libhidl/transport/token/1.0/utils/include

  ${ANDROID_PLATFORM_DIR}/prebuilts/vndk/v30/${PREBUILT}/include/generated-headers/hardware/interfaces/graphics/common/1.0/android.hardware.graphics.common@1.0_genc++_headers/gen
  ${ANDROID_PLATFORM_DIR}/prebuilts/vndk/v30/${PREBUILT}/include/generated-headers/hardware/interfaces/graphics/common/1.1/android.hardware.graphics.common@1.1_genc++_headers/gen
  ${ANDROID_PLATFORM_DIR}/prebuilts/vndk/v30/${PREBUILT}/include/generated-headers/hardware/interfaces/graphics/common/1.2/android.hardware.graphics.common@1.2_genc++_headers/gen

  ${ANDROID_PLATFORM_DIR}/prebuilts/vndk/v30/${PREBUILT}/include/generated-headers/hardware/interfaces/graphics/bufferqueue/1.0/android.hardware.graphics.bufferqueue@1.0_genc++_headers/gen
  ${ANDROID_PLATFORM_DIR}/prebuilts/vndk/v30/${PREBUILT}/include/generated-headers/hardware/interfaces/graphics/bufferqueue/2.0/android.hardware.graphics.bufferqueue@2.0_genc++_headers/gen
  ${ANDROID_PLATFORM_DIR}/prebuilts/vndk/v30/${PREBUILT}/include/generated-headers/system/libhidl/transport/base/1.0/android.hidl.base@1.0_genc++_headers/gen
  ${ANDROID_PLATFORM_DIR}/prebuilts/vndk/v30/${PREBUILT}/include/generated-headers/system/libhidl/transport/manager/1.0/android.hidl.manager@1.0_genc++_headers/gen
  ${ANDROID_PLATFORM_DIR}/prebuilts/vndk/v30/${PREBUILT}/include/generated-headers/hardware/interfaces/media/1.0/android.hardware.media@1.0_genc++_headers/gen
)
message(STATUS "Include directories:")
get_property(include_dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${include_dirs})
  message(STATUS " - ${dir}")
endforeach()

link_directories(${subject} lib/${ANDROID_ABI})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/bin/${platform}/${ANDROID_ABI}/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/bin/${platform}/${ANDROID_ABI}/release)

# 查找目录下所有源文件并保存到变量
# aux_source_directory(src SRCS)
# 搜索源码文件保存到变量
FILE(GLOB SRCS src/*.h src/*.cpp)

add_executable(${subject} ${SRCS})

target_compile_options(${subject} PRIVATE
  -fno-rtti
  -Wno-inconsistent-missing-override
)

target_link_options(${subject} PRIVATE
  -Wl,--allow-shlib-undefined
)

target_link_libraries(${subject}
  stagefright
  media
  media_omx
  utils
  binder
  stagefright_foundation
  jpeg
  ui
  gui
  cutils
  log
  EGL
  GLESv2
)

set_property(TARGET ${subject} PROPERTY DEBUG_POSTFIX _${ANDROID_ABI}_d)
set_property(TARGET ${subject} PROPERTY RELEASE_POSTFIX _${ANDROID_ABI}_r)

# ##############################################################################
