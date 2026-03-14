# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("androidengineapp")
# 【新增】定义 Android 平台宏
add_definitions(-DPLATFORM_ANDROID)
add_definitions(-DANDROID)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 【关键修改】使用 ANDROID_NDK 变量，而不是硬编码路径
if(NOT DEFINED ANDROID_NDK)
    message(FATAL_ERROR "ANDROID_NDK is not defined! Please check your NDK configuration.")
endif()

message(STATUS "Android NDK path: ${ANDROID_NDK}")
#set(NATIVE_APP_GLUE_DIR ${ANDROID_NDK_PATH}/sources/android/native_app_glue)
#message(STATUS "Native app glue dir: ${NATIVE_APP_GLUE_DIR}")
#if(EXISTS ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c)
#    list(APPEND ENGINE_SOURCES ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c)
#    message(STATUS "Added native_app_glue.c to sources")
#else()
#    message(WARNING "native_app_glue.c not found at: ${NATIVE_APP_GLUE_DIR}")
#endif()
set(NDK_GLUE_DIR ${ANDROID_NDK}/sources/android/native_app_glue)
message(STATUS "NDK glue dir: ${NDK_GLUE_DIR}")

set(GLM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/GLM)

# 验证 GLM 是否存在
if(NOT EXISTS ${GLM_INCLUDE_DIR}/glm/glm.hpp)
    message(FATAL_ERROR "GLM not found at ${GLM_INCLUDE_DIR}/glm/glm.hpp")
endif()
message(STATUS "Using GLM from: ${GLM_INCLUDE_DIR}")

 set(ASSIMP_INCLUDE_DIR "D:/work_code/OpenglGraph/thirdparty/assimp/include")
 if(NOT EXISTS ${ASSIMP_INCLUDE_DIR}/assimp/Importer.hpp)
     message(FATAL_ERROR "ASSIMP Importer.hpp not found at: ${ASSIMP_INCLUDE_DIR}")
 endif()


set(STB_IMAGE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/stb_image.h)
if(NOT EXISTS ${STB_IMAGE_PATH})
    message(FATAL_ERROR "stb_image.h not found at ${STB_IMAGE_PATH}")
endif()

message(STATUS "Found stb_image.h at: ${STB_IMAGE_PATH}")
# 打印信息

message(STATUS "Source dir: ${CMAKE_CURRENT_SOURCE_DIR}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include
        ${GLM_INCLUDE_DIR}  # 添加 GLM 路径
        ${ASSIMP_INCLUDE_DIR}  # 添加 ASSIMP 头文件路径
        ${CMAKE_CURRENT_SOURCE_DIR}  # 添加当前目录，让 stb_image.h 可以被找到
        ${NDK_GLUE_DIR}
)
set(GLUE_SOURCE ${NDK_GLUE_DIR}/android_native_app_glue.c)
add_library(native_app_glue STATIC ${GLUE_SOURCE})

# 【新增】添加 stb_image 源文件
add_library(stb_image STATIC stb_image.cpp)
# 检查静态库是否存在
#set(ENGINE_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libengine.a)
set(ENGINE_LIB_PATH "D:/work_code/OpenglGraph/build_android/libs/arm64-v8a/libengine.a") #临时设置
if(NOT EXISTS ${ENGINE_LIB_PATH})
    message(FATAL_ERROR "Engine library not found at: ${ENGINE_LIB_PATH}")
endif()

# 检查静态库是否存在
# set(ASSIMP_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/arm64-v8a/libassimp.so)
set(ASSIMP_LIB_PATH "D:/work_code/OpenglGraph/thirdparty/assimp/lib/libassimp.so") #临时写死
if(NOT EXISTS ${ASSIMP_LIB_PATH})
    message(FATAL_ERROR "ASSIMP library not found at: ${ASSIMP_LIB_PATH}")
endif()


add_library(androidengineapp SHARED native-lib.cpp)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.


# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(androidengineapp
        # List libraries link to the target library
        ${ENGINE_LIB_PATH}  # 直接使用完整路径
        ${ASSIMP_LIB_PATH}
        native_app_glue
        android
        EGL
        GLESv3
        log)

# 设置输出目录
set_target_properties(androidengineapp PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
)

message(STATUS "Configuration complete")