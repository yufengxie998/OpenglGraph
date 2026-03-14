# Engine - DLL项目
project(Engine)

# 调试符号配置（关键！）
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -DDEBUG")

# 检测平台
if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android" OR ANDROID_BUILD)
    message(STATUS "Building Engine for Android")
    add_definitions(-DPLATFORM_ANDROID)
    
    # Android特定的编译选项
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fexceptions -frtti")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

    # Android上构建为静态库更方便
    set(LIB_TYPE STATIC)

    set(ANDROID_NDK_PATH "C:/Users/zl-xiexingguo/AppData/Local/Android/Sdk/ndk/29.0.14206865")
    message(STATUS "Android NDK path: ${ANDROID_NDK_PATH}")
    set(NATIVE_APP_GLUE_DIR ${ANDROID_NDK_PATH}/sources/android/native_app_glue)
    message(STATUS "Native app glue dir: ${NATIVE_APP_GLUE_DIR}")
    if(EXISTS ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c)
        list(APPEND ENGINE_SOURCES ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c)
        message(STATUS "Added native_app_glue.c to sources")
    else()
        message(WARNING "native_app_glue.c not found at: ${NATIVE_APP_GLUE_DIR}")
    endif()

    # 设置 Android ABI 输出目录
    if(ANDROID_ABI)
        set(ANDROID_LIB_OUTPUT ${CMAKE_BINARY_DIR}/libs/${ANDROID_ABI})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ANDROID_LIB_OUTPUT})
        message(STATUS "Android ABI: ${ANDROID_ABI}, output: ${ANDROID_LIB_OUTPUT}")
    endif()
else()
    message(STATUS "Building Engine for Desktop")
    add_definitions(-DPLATFORM_DESKTOP)
    
    # 桌面上构建为共享库
    set(LIB_TYPE SHARED)
endif()

# 获取项目根目录
get_filename_component(ENGINE_ROOT ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)
get_filename_component(WORKSPACE_ROOT ${ENGINE_ROOT}/.. ABSOLUTE)

# 源文件 - 根据平台选择不同的实现文件
set(ENGINE_SOURCES
    src/EngineAPI.cpp
    src/LineRenderer.cpp
    src/Camera.cpp
    src/ModelObject.cpp
    src/Shader.cpp  
    src/mesh.cpp  
    src/model.cpp  
    src/Skybox.cpp
    src/Primitive.cpp
    src/Plane.cpp
    src/Cylinder.cpp
    src/entity.cpp
    src/Transform.cpp
    src/OcclusionCullingManager.cpp
    extend/logger.cpp
)

# 根据平台添加不同的Engine实现文件
if(ANDROID OR ANDROID_BUILD)
    list(APPEND ENGINE_SOURCES
        src/Engine_Android.cpp
    )
else()
    list(APPEND ENGINE_SOURCES
        src/Engine_Desktop.cpp
    )
endif()

# Android不需要桌面OpenGL的glad.c
if(NOT ANDROID AND NOT ANDROID_BUILD)
    list(APPEND ENGINE_SOURCES ${GLAD_DIR}/src/glad.c)
endif()

set(ENGINE_HEADERS
    include/Engine.h
    include/EngineAPI.h
    include/camera.h
    include/LineRenderer.h
    include/ModelObject.h
    include/RenderObject.h
    include/Shader.h
    include/mesh.h
    include/model.h
    include/skybox.h
    include/Primitive.h
    include/Plane.h
    include/Cylinder.h
    include/Transform.h
    include/OcclusionCullingManager.h
    include/stb_image.h  # 添加STB头文件
)

# 创建库（Android上静态库，桌面上动态库）
add_library(engine ${LIB_TYPE} ${ENGINE_SOURCES} ${ENGINE_HEADERS})

# 包含目录
target_include_directories(engine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${GLM_DIR}/include
    ${ASSIMP_DIR}/include
)

# Android不需要这些桌面库的包含目录
if(NOT ANDROID AND NOT ANDROID_BUILD)
    target_include_directories(engine PUBLIC
        ${GLFW_DIR}/include
        ${GLAD_DIR}/include
    )
else()
    target_include_directories(engine PUBLIC
        ${NATIVE_APP_GLUE_DIR}
    )
endif()

# 链接库
if(ANDROID OR ANDROID_BUILD)
    # Android平台链接
    set(ASSIMP_LIB_PATH ${ASSIMP_DIR}/lib/libassimp.so)
    target_link_libraries(engine
        ${ASSIMP_LIB_PATH}
        EGL
        GLESv3
        android
        log
        z  # 有些模型库需要zlib
    )
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
    message(STATUS "Linking Android libraries")
else()
    # 桌面平台链接
    set(ASSIMP_LIB_PATH ${ASSIMP_DIR}/lib/libassimp.dll.a)
    target_link_libraries(engine
        ${GLFW_DIR}/lib/libglfw3.a
        ${ASSIMP_LIB_PATH}
        opengl32
        gdi32
    )
    message(STATUS "Linking Desktop libraries")
endif()

# MinGW调试选项
if(MINGW AND NOT ANDROID)
    target_link_options(engine PRIVATE
        $<$<CONFIG:DEBUG>:-Wl,--export-all-symbols>
        $<$<CONFIG:DEBUG>:-g>
    )
endif()

# 设置输出目录（继承父项目）
if(NOT ANDROID)  # Android上不设置这些输出目录
    set_target_properties(engine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    )
endif()

# Android静态库输出位置
if(ANDROID)
    set_target_properties(engine PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs/${ANDROID_ABI}
    )
endif()

# 定义预处理器宏
target_compile_definitions(engine PRIVATE
    $<$<CONFIG:DEBUG>:DEBUG>
    $<$<NOT:$<CONFIG:DEBUG>>:NDEBUG>
)

if(ANDROID OR ANDROID_BUILD)
    target_compile_definitions(engine PRIVATE
        PLATFORM_ANDROID
        GLM_FORCE_RADIANS
        GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
    )
else()
    target_compile_definitions(engine PRIVATE
        PLATFORM_DESKTOP
    )
endif()

# 安装 - 只在桌面上需要安装
if(NOT ANDROID)
    install(TARGETS engine 
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    install(FILES ${ENGINE_HEADERS} DESTINATION include/Engine)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/core 
        DESTINATION include/Engine/core)
endif()

# 打印编译信息
message(STATUS "Engine build configuration:")
message(STATUS "  - Library type: ${LIB_TYPE}")
message(STATUS "  - Sources: ${ENGINE_SOURCES}")
if(ANDROID)
    message(STATUS "  - Android ABI: ${ANDROID_ABI}")
    message(STATUS "  - Output dir: ${ANDROID_LIB_OUTPUT}")
endif()