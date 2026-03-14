/**
 * @file Skybox.h
 * @brief 天空盒类的头文件
 * 
 * 该文件定义了Skybox类，用于实现天空盒的加载、渲染和管理。
 * 支持立方体贴图的加载、着色器编译、雾效参数设置等功能。
 * 
 * @author 谢兴国   1452492081@qq.com
 * @version 1.0
 * @date 2025-10-12
 * 
 * @note 支持雾效混合，自动移除视图矩阵的平移部分以模拟无限远天空
 */
#ifndef _H_SKYBOX_H
#define _H_SKYBOX_H

#include <glm/glm.hpp>
#include <string>
#include <vector>

#ifdef PLATFORM_ANDROID
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

class Skybox
{
public:
    Skybox();
    ~Skybox();

    bool Initialize(const std::vector<std::string> &faces);
    void Render(const glm::mat4 &view, const glm::mat4 &projection);
    void Shutdown();

private:
    bool LoadCubemap(const std::vector<std::string> &faces);
    bool SetupMesh();
    bool SetupShader();

    unsigned int cubemapTexture;
    unsigned int VAO, VBO;
    unsigned int shaderProgram;
    int viewLoc, projLoc;
    bool fogEnabled = false;
    GLint fogEnableLoc = -1;
    // 直接在类内初始化
    static constexpr float vertices[108] = {
        // positions
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f};
};

#endif // SKYBOX_H