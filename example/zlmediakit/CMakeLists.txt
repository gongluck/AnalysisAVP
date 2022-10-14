cmake_minimum_required(VERSION 3.15)

# 获取当前文件夹名
string(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})

project(${CURRENT_FOLDER}_example)

# ##############################################################################

set(subject ${PROJECT_NAME}_h264server)

# 包含路径
include_directories(${subject}
                    ${PROJECT_SOURCE_DIR}/../../3rdparty/ZLMediaKit/include)

# windows
if(WIN32)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
      ${PROJECT_SOURCE_DIR}/../bin/windows/debug)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
      ${PROJECT_SOURCE_DIR}/../bin/windows/release)
  # win64
  if(CMAKE_CL_64)
    link_directories(
      ${subject}
      ${PROJECT_SOURCE_DIR}/../../3rdparty/ZLMediaKit/lib/windows/x64)
    link_directories(
      ${subject} ${PROJECT_SOURCE_DIR}/../../3rdparty/libsrtp/lib/windows/x64)
    link_directories(
      ${subject} ${PROJECT_SOURCE_DIR}/../../3rdparty/usrsctp/lib/windows/x64)
    link_directories(${subject} "C:/OpenSSL-Win64/lib")
    # win32
  else()
    link_directories(
      ${subject}
      ${PROJECT_SOURCE_DIR}/../../3rdparty/ZLMediaKit/lib/windows/win32)
    link_directories(
      ${subject} ${PROJECT_SOURCE_DIR}/../../3rdparty/libsrtp/lib/windows/win32)
    link_directories(
      ${subject} ${PROJECT_SOURCE_DIR}/../../3rdparty/usrsctp/lib/windows/win32)
    link_directories(${subject} "C:/OpenSSL-Win32/lib")
  endif()
  # android
elseif(ANDROID)
  # set(CMAKE_CXX_FLAGS "-Wno-deprecated-declarations")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
      ${PROJECT_SOURCE_DIR}/../bin/android/debug)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
      ${PROJECT_SOURCE_DIR}/../bin/android/release)
  link_directories(
    ${subject}
    ${PROJECT_SOURCE_DIR}/../../3rdparty/ZLMediaKit/lib/android/${ANDROID_ABI})
  link_directories(
    ${subject}
    ${PROJECT_SOURCE_DIR}/../../3rdparty/libsrtp/lib/android/${ANDROID_ABI}
    ${PROJECT_SOURCE_DIR}/../../3rdparty/usrsctp/lib/android/${ANDROID_ABI})
  if(ANDROID_ABI STREQUAL "armeabi-v7a")
    link_directories(
      ${subject}
      ${PROJECT_SOURCE_DIR}/../../3rdparty/openssl/lib/android/armeabi)
  else()
    link_directories(
      ${subject}
      ${PROJECT_SOURCE_DIR}/../../3rdparty/openssl/lib/android/${ANDROID_ABI})
  endif()
  # linux
elseif(UNIX)
  # set(CMAKE_CXX_FLAGS "-Wno-deprecated-declarations")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -Wall -g2 -ggdb")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall -g2 -ggdb")
  set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin/linux)
  link_directories(${subject}
                   ${PROJECT_SOURCE_DIR}/../../3rdparty/ZLMediaKit/lib/linux)
endif()

# 添加源码文件到项目
add_executable(${subject} h264_media_server.cpp)

target_link_libraries(
  ${subject}
  mk_api
  zlmediakit
  zltoolkit
  webrtc
  mpeg
  mov
  flv
  jsoncpp
  srtp2
  usrsctp)
if(WIN32)
  # windows静态库补充依赖
  target_link_libraries(${subject} openssl libcrypto) # ssl使用静态库
elseif(ANDROID)
  # android静态库补充依赖
  target_link_libraries(${subject} log libssl.a libcrypto.a) # ssl使用静态库
elseif(UNIX)
  # linux静态库补充依赖
endif()

# windows
if(WIN32)
  set_property(
    TARGET ${subject} PROPERTY MSVC_RUNTIME_LIBRARY
                               "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  if(CMAKE_CL_64)
    set_property(TARGET ${subject} PROPERTY DEBUG_POSTFIX _x64_d)
    set_property(TARGET ${subject} PROPERTY RELEASE_POSTFIX _x64_r)
  else()
    set_property(TARGET ${subject} PROPERTY DEBUG_POSTFIX _win32_d)
    set_property(TARGET ${subject} PROPERTY RELEASE_POSTFIX _win32_r)
  endif()
  # android
elseif(ANDROID)
  set_property(TARGET ${subject} PROPERTY DEBUG_POSTFIX _${ANDROID_ABI}_d)
  set_property(TARGET ${subject} PROPERTY RELEASE_POSTFIX _${ANDROID_ABI}_r)
elseif(UNIX)
  set_property(TARGET ${subject} PROPERTY DEBUG_POSTFIX _${ANDROID_ABI}_d)
  set_property(TARGET ${subject} PROPERTY RELEASE_POSTFIX _${ANDROID_ABI}_r)
endif()

# ##############################################################################
