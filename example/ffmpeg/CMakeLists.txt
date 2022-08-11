cmake_minimum_required(VERSION 3.15)

# 获取当前文件夹名
STRING(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})

project(${CURRENT_FOLDER}_example)

################################################################################

set(subject ${PROJECT_NAME}_mux)

# 包含路径
include_directories(${subject} ${PROJECT_SOURCE_DIR}/../../3rdparty/ffmpeg/include)

if(WIN32)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/../bin/windows/debug)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/../bin/windows/release)
  if(CMAKE_CL_64)
    link_directories(${subject} ${PROJECT_SOURCE_DIR}/../../3rdparty/ffmpeg/lib/windows/x64)
  else()
    link_directories(${subject} ${PROJECT_SOURCE_DIR}/../../3rdparty/ffmpeg/lib/windows/win32)
  endif()
elseif(ANDROID)
  #set(CMAKE_CXX_FLAGS "-Wno-deprecated-declarations")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/../bin/android/debug)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/../bin/android/release)
  link_directories(${subject} ${PROJECT_SOURCE_DIR}/../../3rdparty/ffmpeg/lib/android/${ANDROID_ABI})
elseif(UNIX)
  #set(CMAKE_CXX_FLAGS "-Wno-deprecated-declarations")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -Wall -g2 -ggdb")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall -g2 -ggdb")
  set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin/linux)
  link_directories(${subject} ${PROJECT_SOURCE_DIR}/../../3rdparty/ffmpeg/lib/linux)
endif()

# 添加源码文件到项目
add_executable(${subject} 
mux.cpp
)

target_link_libraries(${subject} avformat avcodec swresample avutil)
if(WIN32)
  #windows静态库补充依赖
  target_link_libraries(${subject} Ws2_32 Secur32 Mfuuid Mfplat Strmiids Bcrypt)
elseif(ANDROID)
  #android静态库补充依赖
  target_link_libraries(${subject} z)
elseif(UNIX)
  #linux静态库补充依赖
  target_link_libraries(${subject} z pthread lzma va va-drm bz2)
endif()

#windows
if(WIN32)
  if(CMAKE_CL_64)
    set_property(TARGET ${subject} PROPERTY DEBUG_POSTFIX _x64_d)
    set_property(TARGET ${subject} PROPERTY RELEASE_POSTFIX _x64_r)
  else()
    set_property(TARGET ${subject} PROPERTY DEBUG_POSTFIX _win32_d)
    set_property(TARGET ${subject} PROPERTY RELEASE_POSTFIX _win32_r)
  endif()
#android
elseif(ANDROID)
  set_property(TARGET ${subject} PROPERTY DEBUG_POSTFIX _${ANDROID_ABI}_d)
  set_property(TARGET ${subject} PROPERTY RELEASE_POSTFIX _${ANDROID_ABI}_r)
elseif(UNIX)
  set_property(TARGET ${subject} PROPERTY DEBUG_POSTFIX _${ANDROID_ABI}_d)
  set_property(TARGET ${subject} PROPERTY RELEASE_POSTFIX _${ANDROID_ABI}_r)
endif()

################################################################################

set(subject ${PROJECT_NAME}_resample)

# 包含路径
include_directories(${subject} ${PROJECT_SOURCE_DIR}/../../3rdparty/ffmpeg/include)

if(WIN32)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/../bin/windows/debug)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/../bin/windows/release)
  if(CMAKE_CL_64)
    link_directories(${subject} ${PROJECT_SOURCE_DIR}/../../3rdparty/ffmpeg/lib/windows/x64)
  else()
    link_directories(${subject} ${PROJECT_SOURCE_DIR}/../../3rdparty/ffmpeg/lib/windows/win32)
  endif()
elseif(ANDROID)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/../bin/android/debug)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/../bin/android/release)
  link_directories(${subject} ${PROJECT_SOURCE_DIR}/../../3rdparty/ffmpeg/lib/android/${ANDROID_ABI})
elseif(UNIX)
  #set(CMAKE_CXX_FLAGS "-Wno-deprecated-declarations")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -Wall -g2 -ggdb")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall -g2 -ggdb")
  set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin/linux)
  link_directories(${subject} ${PROJECT_SOURCE_DIR}/../../3rdparty/ffmpeg/lib/linux)
endif()

# 添加源码文件到项目
add_executable(${subject} 
resample.cpp
)

target_link_libraries(${subject} swresample avutil)
if(WIN32)
  #windows静态库补充依赖
  target_link_libraries(${subject} Ws2_32 Secur32 Mfuuid Mfplat Strmiids Bcrypt)
elseif(ANDROID)
  #android静态库补充依赖
  target_link_libraries(${subject} z)
elseif(UNIX)
  #linux静态库补充依赖
  target_link_libraries(${subject} z pthread lzma va va-drm bz2)
endif()

#windows
if(WIN32)
  if(CMAKE_CL_64)
    set_property(TARGET ${subject} PROPERTY DEBUG_POSTFIX _x64_d)
    set_property(TARGET ${subject} PROPERTY RELEASE_POSTFIX _x64_r)
  else()
    set_property(TARGET ${subject} PROPERTY DEBUG_POSTFIX _win32_d)
    set_property(TARGET ${subject} PROPERTY RELEASE_POSTFIX _win32_r)
  endif()
#android
elseif(ANDROID)
  set_property(TARGET ${subject} PROPERTY DEBUG_POSTFIX _${ANDROID_ABI}_d)
  set_property(TARGET ${subject} PROPERTY RELEASE_POSTFIX _${ANDROID_ABI}_r)
elseif(UNIX)
  set_property(TARGET ${subject} PROPERTY DEBUG_POSTFIX _${ANDROID_ABI}_d)
  set_property(TARGET ${subject} PROPERTY RELEASE_POSTFIX _${ANDROID_ABI}_r)
endif()
