/**
 * @brief 着色器类，封装OpenGL着色器程序
 * 
 * @details 负责从文件或源码编译顶点和片段着色器，
 *          并提供uniform变量设置的便捷方法
 * 
 * @author 谢兴国   1452492081@qq.com
 * @version 1.0
 * @date 2025-10-12
 * 
 * @note 不支持几何着色器（为Android平台优化）
 */
#ifndef _H_SHADER_H
#define _H_SHADER_H

#ifdef PLATFORM_ANDROID
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    unsigned int ID;

#ifdef PLATFORM_ANDROID
    // 构造函数1：从字符串源码创建 - 使用 std::string 参数以区分
    Shader(const std::string& vertexSource, const std::string& fragmentSource);
#endif 
    // 注意：移除了 geometryPath 参数，因为 OpenGL ES 不支持几何着色器
    Shader(const char *vertexPath, const char *fragmentPath,bool fromFile);

    // activate the shader
    void use();
    
    // utility uniform functions
    void setBool(const std::string &name, bool value) const;
    
    void setInt(const std::string &name, int value) const;
    
    void setFloat(const std::string &name, float value) const;
    
    void setVec2(const std::string &name, const glm::vec2 &value) const;
    
    void setVec2(const std::string &name, float x, float y) const;
    
    void setVec3(const std::string &name, const glm::vec3 &value) const;
    
    void setVec3(const std::string &name, float x, float y, float z) const;
    
    void setVec4(const std::string &name, const glm::vec4 &value) const;
    
    void setVec4(const std::string &name, float x, float y, float z, float w);
    
    void setMat2(const std::string &name, const glm::mat2 &mat) const;
    
    void setMat3(const std::string &name, const glm::mat3 &mat) const;
    
    void setMat4(const std::string &name, const glm::mat4 &mat) const;

private:
    void compileFromSource(const char *vertexSource, const char *fragmentSource);
    
    void loadFromFile(const char *vertexPath, const char *fragmentPath);
};

#endif