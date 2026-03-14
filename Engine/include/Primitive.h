/**
 * @file Primitive.h
 * @brief 基础几何体类的头文件
 * 
 * 该文件定义了Primitive类，作为所有基础几何体（平面、圆柱体等）的基类。
 * 提供顶点数据管理、VAO/VBO/EBO设置、材质uniforms设置等通用功能。
 * 
 * @author 谢兴国   1452492081@qq.com
 * @version 1.0
 * @date 2025-10-12
 * 
 * @note 继承自Entity类，支持普通材质和PBR材质两种渲染管线
 */
#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include <glm/glm.hpp>
#include <vector>
#include "dataType.h"
#include "entity.h"
#ifdef PLATFORM_ANDROID
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

// 顶点结构（与 Mesh 类保持一致）
struct PrimitiveVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

// 几何体基类
class Primitive: public Entity {
public:
    Primitive();
    virtual ~Primitive();
    // Entity 的纯虚函数已经在基类中有默认实现，这里可以重写
    virtual void Update(float deltaTime) override
    {
        Entity::Update(deltaTime);
    }

    virtual void Render(Shader &shader, ShaderType shaderType) override
    {
        shader.setMat4("model", transform.getModelMatrix());
        Draw(shader.ID, shaderType);
    }
    virtual void Draw(GLuint shaderProgram, ShaderType shaderType) = 0; // 纯虚函数
    // void SetPosition(const glm::vec3& pos) { position = pos; }
    // void SetRotation(const glm::vec3& rot) { rotation = rot; }
    // void SetScale(const glm::vec3& scl) { scale = scl; }
    // void SetTransform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale);
    void SetColor(const glm::vec3 &color);
    void SetTexture(GLuint textureID);

    // 添加 getter 方法
    // glm::vec3 GetPosition() const { return position; }
    // glm::vec3 GetRotation() const { return rotation; }
    // glm::vec3 GetScale() const { return scale; }
    glm::vec3 GetColor() const { return color; }
    GLuint GetTextureID() const { return textureID; }
    bool HasTexture() const { return hasTexture; }

protected:
    bool SetupMesh();
    void SetMaterialUniforms(GLuint shaderProgram, ShaderType shaderType);
    
    GLuint VAO, VBO, EBO;
    std::vector<PrimitiveVertex> vertices;
    std::vector<unsigned int> indices;
    
    // glm::vec3 position;
    // glm::vec3 rotation;
    // glm::vec3 scale;
    glm::vec3 color;
    GLuint textureID;
    
    bool hasTexture;
    bool initialized;
};

#endif // PRIMITIVE_H