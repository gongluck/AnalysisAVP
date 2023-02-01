cmake_minimum_required(VERSION 3.10)

# 获取当前文件夹名
string(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})

project(${CURRENT_FOLDER}_analysis)

if(WIN32 OR ANDROID)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/../bin/debug)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/../bin/release)
endif()

# 添加源码文件到项目
file(GLOB header ./*.h)
aux_source_directory(. SRCS)
add_executable(${PROJECT_NAME} ${header} ${SRCS})

if(WIN32)
  if(CMAKE_CL_64)
    set_property(TARGET ${PROJECT_NAME} PROPERTY DEBUG_POSTFIX _x64_d)
    set_property(TARGET ${PROJECT_NAME} PROPERTY RELEASE_POSTFIX _x64_r)
  else()
    set_property(TARGET ${PROJECT_NAME} PROPERTY DEBUG_POSTFIX _win32_d)
    set_property(TARGET ${PROJECT_NAME} PROPERTY RELEASE_POSTFIX _win32_r)
  endif()
elseif(ANDROID)
  set_property(TARGET ${PROJECT_NAME} PROPERTY DEBUG_POSTFIX _${ANDROID_ABI}_d)
  set_property(TARGET ${PROJECT_NAME} PROPERTY RELEASE_POSTFIX _${ANDROID_ABI}_r)
else()
  set_property(TARGET ${PROJECT_NAME} PROPERTY DEBUG_POSTFIX _d)
  set_property(TARGET ${PROJECT_NAME} PROPERTY RELEASE_POSTFIX _r)
endif()
