cmake_minimum_required(VERSION 3.10)

# 获取当前文件夹名
STRING(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})

project(${CURRENT_FOLDER}_analysis)

if(WIN32)
# 分别设置了Debug版本和Release版本可执行文件的输出目录(经测试,windows环境有效)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/../bin/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/../bin/release)
elseif(UNIX)
# 设置库文件的输出目录(经测试,linux环境有效)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin)
endif(WIN32)

# 添加源码文件到项目
FILE(GLOB header ./*.h ../aac/*.h ../common.h)
aux_source_directory(. SRCS)
add_executable(${PROJECT_NAME} ${header} ${SRCS} ../aac/aac.cpp)

set_property(TARGET ${PROJECT_NAME} PROPERTY DEBUG_POSTFIX _d)
