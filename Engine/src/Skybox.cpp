#include "Skybox.h"
#include <extend/logger.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
// #define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Skybox::Skybox() : cubemapTexture(0), VAO(0), VBO(0), shaderProgram(0) {}

Skybox::~Skybox()
{
    Shutdown();
}

bool Skybox::SetupShader()
{
    #ifdef PLATFORM_ANDROID
    // 在使用 Raw String Literal R"(...)" 时，字符串从下一行开始 所以#version 300 es前不能有空格
    const char *vertexShaderSource = R"(#version 300 es
layout (location = 0) in vec3 aPos;
out vec3 TexCoords;
uniform mat4 projection;
uniform mat4 view;
void main() {
TexCoords = aPos;
mat4 viewNoTranslation = mat4(mat3(view));
vec4 pos = projection * viewNoTranslation * vec4(aPos, 1.0);
gl_Position = pos.xyww;
}
)";
        
        const char *fragmentShaderSource = R"(#version 300 es
precision highp float;
in vec3 TexCoords;
out vec4 FragColor;
uniform samplerCube skybox;
void main() {
FragColor = texture(skybox, TexCoords);
}
)";
    #else
    const char *vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        out vec3 TexCoords;
        uniform mat4 projection;
        uniform mat4 view;
        void main() {
            TexCoords = aPos;
            mat4 viewNoTranslation = mat4(mat3(view));
            vec4 pos = projection * viewNoTranslation * vec4(aPos, 1.0);
            gl_Position = pos.xyww;
        }
    )";

    const char *fragmentShaderSource = R"(
        #version 330 core
        in vec3 TexCoords;
        out vec4 FragColor;
        uniform samplerCube skybox;
        uniform float fogDensity = 0.0f;
        uniform vec3 fogColor = vec3(0.5f, 0.6f, 0.7f);
        void main() {
        vec4 skyColor = texture(skybox, TexCoords);
        
        // 简单的雾效
        if (fogDensity > 0.0) {
            float fogFactor = exp(-fogDensity * fogDensity);
            fogFactor = clamp(fogFactor, 0.0, 1.0);
            skyColor = mix(vec4(fogColor, 1.0), skyColor, fogFactor);
        }
        
        FragColor = skyColor;
        }
    )";
    #endif
    // 编译顶点着色器
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        LOGE("Skybox", "Vertex shader compilation failed: %s", infoLog);
        return false;
    }

    // 编译片段着色器
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        LOGE("Skybox", "Fragment shader compilation failed: %s", infoLog);
        return false;
    }

    // 链接着色器程序
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        LOGE("Skybox", "Shader program linking failed: %s", infoLog);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    viewLoc = glGetUniformLocation(shaderProgram, "view");
    projLoc = glGetUniformLocation(shaderProgram, "projection");
    fogEnableLoc = glGetUniformLocation(shaderProgram, "fogDensity");
    LOGI("Skybox", "Shader setup successful");
    return true;
}

bool Skybox::SetupMesh()
{
    if (VAO != 0)
        return true; // 已经初始化
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    LOGI("Skybox", "Mesh setup successful");
    return true;
}

bool Skybox::LoadCubemap(const std::vector<std::string> &faces)
{
    if (faces.size() != 6)
    {
        LOGE("Skybox", "Need exactly 6 faces for cubemap");
        return false;
    }

    glGenTextures(1, &cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
            LOGI("Skybox", "Loaded face %d: %s (%dx%d)", i, faces[i].c_str(), width, height);
        }
        else
        {
            LOGE("Skybox", "Failed to load face %d: %s", i, faces[i].c_str());
            return false;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    LOGI("Skybox", "Cubemap loaded successfully, texture ID: %d", cubemapTexture);
    return true;
}

bool Skybox::Initialize(const std::vector<std::string> &faces)
{
    LOGI("Skybox", "Initializing skybox...");

    if (!SetupShader())
    {
        LOGE("Skybox", "Failed to setup shader");
        return false;
    }

    if (!SetupMesh())
    {
        LOGE("Skybox", "Failed to setup mesh");
        return false;
    }

    if (!LoadCubemap(faces))
    {
        LOGE("Skybox", "Failed to load cubemap");
        return false;
    }

    LOGI("Skybox", "Skybox initialized successfully");
    return true;
}

void Skybox::Render(const glm::mat4 &view, const glm::mat4 &projection)
{
    if (!shaderProgram || !VAO || !cubemapTexture)
        return;

    // 缓存 uniform 位置
    static GLint cachedSkyboxLoc = -1;
    if (cachedSkyboxLoc == -1)
    {
        cachedSkyboxLoc = glGetUniformLocation(shaderProgram, "skybox");
    }
    glDepthFunc(GL_LEQUAL);
    glUseProgram(shaderProgram);

    // 移除视图矩阵的平移部分
    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewNoTranslation));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // 设置雾效参数
    if (fogEnableLoc != -1)
    {
        glUniform1i(fogEnableLoc, fogEnabled ? 1 : 0);
    }

    glBindVertexArray(VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glUniform1i(cachedSkyboxLoc, 0);

    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}

void Skybox::Shutdown()
{
    if (VAO)
        glDeleteVertexArrays(1, &VAO);
    if (VBO)
        glDeleteBuffers(1, &VBO);
    if (cubemapTexture)
        glDeleteTextures(1, &cubemapTexture);
    if (shaderProgram)
        glDeleteProgram(shaderProgram);
}