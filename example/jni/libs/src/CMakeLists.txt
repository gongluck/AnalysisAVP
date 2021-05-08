# CMake最低版本要求
cmake_minimum_required(VERSION 3.0)

# 项目名称
project(add)

# 查找当前目录下所有源文件并保存到变量
aux_source_directory(. SRCS)

# 设置库文件的输出目录(经测试,linux环境有效)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../libs/${ANDROID_ABI})  

# 链接库
add_library(${PROJECT_NAME} SHARED ${SRCS})
#add_library(${PROJECT_NAME} STATIC ${SRCS})