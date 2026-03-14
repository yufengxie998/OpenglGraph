# OpenGL Graph Library (OpenglGraph)

[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![OpenGL](https://img.shields.io/badge/OpenGL-4.5-red.svg)](https://www.opengl.org/)
[![CMake](https://img.shields.io/badge/CMake-3.15+-brightgreen.svg)](https://cmake.org/)
[![行为准则](https://img.shields.io/badge/Contributor%20Covenant-2.1-4baaaa.svg)](CODE_OF_CONDUCT.md)

###项目简介

OpenGL Graph 简称OLG 是一个基于 OpenGL 接口的现代图形渲染库，旨在提供高效、易用的渲染解决方案。该库封装了复杂的 OpenGL 调用，提供简洁的 API 接口，让开发者能够快速构建高质量的图形应用程序

####主要特性

- *** 线条绘制系统**
  - 支持贝塞尔曲线、多项式曲线、圆等几何图形的绘制
  - 实线和虚线样式支持
  - 自定义线条样式（颜色、宽度、纹理等）

- *** 3D 模型加载**
  - 支持多种主流模型格式：OBJ、GLB、GLTF
  - 自动材质和纹理加载
  - 支持多网格模型

- *** 基础几何体**
  - 平面、圆柱体等基础几何体生成
  - 支持自定义细分程度
  - 法线、UV坐标自动生成

- *** 双渲染管线**
  - **普通材质管线**：传统Phong/Blinn-Phong光照模型
  - **PBR材质管线**：基于物理的渲染，支持金属/粗糙度工作流
  - 运行时动态切换渲染管线

- *** 环境渲染**
  - 天空盒支持（立方体贴图）
  - HDR环境贴图支持
  - 支持动态环境切换

- **# 场景优化**
  - 视锥体剔除
  - 遮挡剔除
  - 支持自定义剔除回调
  - 渲染批次优化

###快速开始   部署或者开发过程中都可以联系我:1452492081@qq.com

#include <OpenglGraph/Renderer.h>
#include <OpenglGraph/Model.h>
#include <OpenglGraph/Curve.h>

using namespace OpenglGraph;

int main() {
    // 创建渲染器
    Renderer renderer(1280, 720, "OpenGL Graph Demo");
    
    // 切换到PBR渲染管线
    renderer.setPipeline(PipelineType::PBR);
    
    // 加载模型
    Model model("assets/sphere.glb");
    model.setPosition(glm::vec3(0.0f, 0.0f, -5.0f));
    
    // 创建贝塞尔曲线
    BezierCurve curve({
        glm::vec3(-2.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 3.0f, 0.0f),
        glm::vec3(2.0f, 1.0f, 0.0f)
    });
    curve.setColor(glm::vec3(1.0f, 0.0f, 0.0f));
    curve.setLineStyle(LineStyle::Dashed);
    
    // 设置天空盒
    renderer.setSkybox("assets/skybox/");
    
    // 主循环
    while (renderer.isRunning()) {
        renderer.clear();
        
        // 渲染模型和曲线
        model.draw();
        curve.draw();
        
        renderer.swapBuffers();
    }
    
    return 0;
}

### 系统要求

- C++17 或更高版本
- OpenGL 4.5+
- CMake 3.15+
- 支持的操作系统：Windows 10/11, Linux, macOS

### 依赖库

本项目使用以下优秀的开源库：

| 库名称 | 版本 | 许可证 | 用途 |
|--------|------|--------|------|
| [GLM](https://github.com/g-truc/glm) | 0.9.9+ | MIT | 数学运算 |
| [Assimp](https://github.com/assimp/assimp) | 5.2+ | BSD-3 | 模型加载 |
| [stb_image](https://github.com/nothings/stb) | 2.28+ | Public Domain/MIT | 纹理加载 |

### 编译安装

```bash
# 克隆仓库
git clone https://github.com/yufengxie998/OpenglGraph.git
cd OpenglGraph

# 创建构建目录
mkdir build && cd build

# 配置CMake（Windows）
cmake -G "MinGW Makefiles" -DCMAKE_MAKE_PROGRAM=mingw32-make ..

# 编译
cmake --build . --config Release

# 安装（可选）
cmake --install .


项目结构

OpenglGraph/
├── Engine/ # 核心引擎代码
│ ├── include/ # 公共头文件
│ └── src/ # 源代码文件
├── AndroidEngineApp/# Android平台应用
├── WindowsEngineApp/# Windows平台应用
├── thirdparty/ # 第三方依赖库
├── examples/ # 使用示例
├── docs/ # 项目文档
└── tests/ # 测试代码
问题:
如果遇到:无法定位程序输入点 _xxxx 于动态链接库 libassimp-6.dll  ,请重新编译 assimp第三方库 库源码地址:https://github.com/assimp/assimp