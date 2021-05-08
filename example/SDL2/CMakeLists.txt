cmake_minimum_required(VERSION 3.15)

# 获取当前文件夹名
STRING(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})

project(${CURRENT_FOLDER}_example)

# 包含路径
include_directories(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/../../../3rdparty/SDL2/include)

if(WIN32)
# 分别设置了Debug版本和Release版本可执行文件的输出目录(经测试,windows环境有效)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/../bin/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/../bin/release)
# 设置编译参数
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
if(CMAKE_CL_64)
#win64
link_directories(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/../../../3rdparty/SDL2/lib/x64)
else()
#win32
link_directories(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/../../../3rdparty/SDL2/lib/win32)
endif(CMAKE_CL_64)
endif(WIN32)

# 添加源码文件到项目
FILE(GLOB header ./*.h)
aux_source_directory(. SRCS)
add_executable(${PROJECT_NAME} ${header} ${SRCS})

if(WIN32)
#windows
target_link_libraries(${PROJECT_NAME} SDL2.lib)
endif(WIN32)

set_property(TARGET ${PROJECT_NAME} PROPERTY DEBUG_POSTFIX _d)
