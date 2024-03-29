﻿cmake_minimum_required(VERSION 3.15)

# 获取当前文件夹名
string(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})

project(${CURRENT_FOLDER}_example)

# 包含路径
include_directories(${PROJECT_NAME}
                    ${PROJECT_SOURCE_DIR}/../../3rdparty/libopus/include)

if(WIN32)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
      ${PROJECT_SOURCE_DIR}/../bin/windows/debug)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
      ${PROJECT_SOURCE_DIR}/../bin/windows/release)
  if(CMAKE_CL_64)
    # win64
    link_directories(
      ${PROJECT_NAME}
      ${PROJECT_SOURCE_DIR}/../../3rdparty/libopus/lib/windows/x64)
  else()
    # win32
    link_directories(
      ${PROJECT_NAME}
      ${PROJECT_SOURCE_DIR}/../../3rdparty/libopus/lib/windows/win32)
  endif()
elseif(ANDROID)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
      ${PROJECT_SOURCE_DIR}/../bin/android/debug)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
      ${PROJECT_SOURCE_DIR}/../bin/android/release)
  link_directories(
    ${PROJECT_NAME}
    ${PROJECT_SOURCE_DIR}/../../3rdparty/libopus/lib/android/${ANDROID_ABI})
elseif(APPLE)
  if(IOS)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
      ${PROJECT_SOURCE_DIR}/../bin/apple/ios/debug)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
      ${PROJECT_SOURCE_DIR}/../bin/apple/ios/release)
    link_directories(
      ${PROJECT_NAME}
      ${PROJECT_SOURCE_DIR}/../../3rdparty/libopus/lib/apple/ios)
  else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
      ${PROJECT_SOURCE_DIR}/../bin/apple/osx/debug)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
      ${PROJECT_SOURCE_DIR}/../bin/apple/osx/release)
    link_directories(
      ${PROJECT_NAME}
      ${PROJECT_SOURCE_DIR}/../../3rdparty/libopus/lib/apple/osx)
  endif()
endif()

# 添加源码文件到项目
file(GLOB header ./*.h)
aux_source_directory(. SRCS)
add_executable(${PROJECT_NAME} ${header} ${SRCS})

target_link_libraries(${PROJECT_NAME} opus)

# windows
if(WIN32)
  if(CMAKE_CL_64)
    set_property(TARGET ${PROJECT_NAME} PROPERTY DEBUG_POSTFIX _x64_d)
    set_property(TARGET ${PROJECT_NAME} PROPERTY RELEASE_POSTFIX _x64_r)
  else()
    set_property(TARGET ${PROJECT_NAME} PROPERTY DEBUG_POSTFIX _win32_d)
    set_property(TARGET ${PROJECT_NAME} PROPERTY RELEASE_POSTFIX _win32_r)
  endif()
# android
elseif(ANDROID)
  set_property(TARGET ${PROJECT_NAME} PROPERTY DEBUG_POSTFIX _${ANDROID_ABI}_d)
  set_property(TARGET ${PROJECT_NAME} PROPERTY RELEASE_POSTFIX _${ANDROID_ABI}_r)
# apple
elseif(APPLE)
  set_property(TARGET ${PROJECT_NAME} PROPERTY DEBUG_POSTFIX _d)
  set_property(TARGET ${PROJECT_NAME} PROPERTY RELEASE_POSTFIX _r)
endif()
