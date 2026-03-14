cmake_minimum_required(VERSION 3.10)
project(OpenglGraph LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============ 全局构建类型设置 ============
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ============ 全局输出目录（统一管理） ============
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})
#set(ENGINE_DIR ${CMAKE_SOURCE_DIR}/Engine)
# ============ 第三方库路径（统一管理） ============
set(THIRDPARTY_DIR ${CMAKE_SOURCE_DIR}/thirdparty CACHE PATH "Thirdparty libraries directory")
set(GLFW_DIR ${THIRDPARTY_DIR}/GLFW)
set(GLAD_DIR ${THIRDPARTY_DIR}/GLAD)
set(GLM_DIR ${THIRDPARTY_DIR}/GLM)
set(ASSIMP_DIR ${THIRDPARTY_DIR}/assimp)

# ============ 平台检测 ============
if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android")
    message(STATUS "Building for Android platform")
    
    # ============ Engine 子项目（核心库） ============
    add_subdirectory(Engine)
    
    # ============ Android JNI 应用 ============
    # add_subdirectory(AppProject_Android)
    
else()
    message(STATUS "Building for Desktop platform")
    
    # ============ Engine 子项目（核心库） ============
    add_subdirectory(Engine)
    
    # ============ 桌面应用 ============
    add_subdirectory(WindowsEngineApp)
endif()

# ============ 可选：安装规则 ============
install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ DESTINATION bin)