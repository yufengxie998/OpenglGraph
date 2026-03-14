# AppProject_Windows - 可执行文件项目
project(WindowsEngineApp)

# 创建可执行文件
add_executable(${PROJECT_NAME} src/main.cpp)

# 链接引擎DLL（自动处理Debug/Release）
target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    engine  # 直接链接目标，无需手动指定路径！
)

# 包含引擎头文件
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 编译选项
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# 自动复制DLL到运行目录（由CMake自动处理）
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)