# OpenGL Graph Library (OpenglGraph)

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![OpenGL](https://img.shields.io/badge/OpenGL-3.3+-red.svg)](https://www.opengl.org/)
[![CMake](https://img.shields.io/badge/CMake-3.15+-brightgreen.svg)](https://cmake.org/)
[![行为准则](https://img.shields.io/badge/Contributor%20Covenant-2.1-4baaaa.svg)](CODE_OF_CONDUCT.md)

## 项目简介

OpenGL Graph (简称OLG) 是一个基于 OpenGL 3.3+ 的现代图形渲染库，致力于提供高效、易用的渲染解决方案。本库封装了复杂的 OpenGL 调用，提供简洁的 API 接口，让开发者能够快速构建高质量的图形应用程序。

### ?? 项目状态
- **当前版本**: 0.1.0 (开发中)
- **许可证**: GPL-3.0
- **OpenGL版本要求**: 3.3 或更高

## ? 主要特性

### 线条绘制系统
- 支持贝塞尔曲线、多项式曲线、圆等几何图形的绘制
- 实线和虚线样式支持
- 自定义线条样式（颜色、宽度、纹理等）

### 3D 模型加载
- 支持多种主流模型格式：OBJ、GLB、GLTF
- 自动材质和纹理加载
- 支持多网格模型

### 基础几何体
- 平面、圆柱体、球体、立方体等基础几何体生成
- 支持自定义细分程度
- 法线、UV坐标自动生成

### 双渲染管线
- **普通材质管线**：传统Phong/Blinn-Phong光照模型
- **PBR材质管线**：基于物理的渲染，支持金属/粗糙度工作流
- 运行时动态切换渲染管线

### 环境渲染
- 天空盒支持（立方体贴图）
- HDR环境贴图支持
- 支持动态环境切换

### 场景优化
- 视锥体剔除
- 遮挡剔除
- 支持自定义剔除回调
- 渲染批次优化

## ?? 快速开始

### 环境要求
- C++17 兼容的编译器 (GCC 7+, Clang 5+, MSVC 2017+)
- OpenGL 3.3+ 驱动的显卡
- CMake 3.15+
- 支持的操作系统：Windows 10/11, Linux, macOS 10.15+

### 安装依赖

#### Windows (使用vcpkg)
```bash
vcpkg install glm assimp stb
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
 常见问题解决
Assimp DLL 加载错误
问题: 遇到"无法定位程序输入点 _xxxx 于动态链接库 libassimp-6.dll"

解决方案:

重新编译Assimp库:

bash
git clone https://github.com/assimp/assimp.git
cd assimp
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
cmake --build . --config Release
将编译生成的DLL文件复制到可执行文件目录

确保使用与项目相同的编译器版本编译Assimp


 联系方式
项目维护者: yufengxie998
邮箱: 1452492081@qq.com
问题反馈: GitHub Issues

致谢
感谢所有为本项目提供支持和帮助的开发者和组织