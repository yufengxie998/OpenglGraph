#include "Primitive.h"
#include <extend/logger.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// SPDX-License-Identifier: GPL-3.0-or-later
Primitive::Primitive() 
 :Entity(),VAO(0), VBO(0), EBO(0), 
      color(1.0f, 1.0f, 1.0f), textureID(0), 
      hasTexture(false), initialized(false) {}

Primitive::~Primitive() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
}

bool Primitive::SetupMesh() {
    if (vertices.empty() || indices.empty()) {
        LOGE("Primitive", "No vertex or index data");
        return false;
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(PrimitiveVertex), 
                 &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), 
                 &indices[0], GL_STATIC_DRAW);

    // 顶点位置
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PrimitiveVertex), (void*)0);
    
    // 法线
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PrimitiveVertex), 
                         (void*)offsetof(PrimitiveVertex, Normal));
    
    // 纹理坐标
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(PrimitiveVertex), 
                         (void*)offsetof(PrimitiveVertex, TexCoords));
    
    // 切线
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(PrimitiveVertex), 
                         (void*)offsetof(PrimitiveVertex, Tangent));
    
    // 副切线
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(PrimitiveVertex), 
                         (void*)offsetof(PrimitiveVertex, Bitangent));

    glBindVertexArray(0);
    
    initialized = true;
    LOGI("Primitive", "Mesh setup successful, %zu vertices, %zu indices", 
         vertices.size(), indices.size());
    return true;
}

// void Primitive::SetTransform(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scl) {
//     position = pos;
//     rotation = rot;
//     scale = scl;
// }

void Primitive::SetColor(const glm::vec3& col) {
    color = col;
}

void Primitive::SetTexture(GLuint texID) {
    textureID = texID;
    hasTexture = (texID != 0);
}

void Primitive::SetMaterialUniforms(GLuint shaderProgram, ShaderType shaderType) {
    // 使用 Entity 的 transform 获取模型矩阵
    glm::mat4 modelMat = transform.getModelMatrix();
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 
                       1, GL_FALSE, glm::value_ptr(modelMat));
    
    if (shaderType == ShaderType::SHADER_SIMPLE) {
        // 简单着色器：只设置漫反射纹理
        if (hasTexture && textureID != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "texture_diffuse1"), 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "hasTexture"), 1);
        } else {
            glUniform1i(glGetUniformLocation(shaderProgram, "hasTexture"), 0);
        }
    } 
    else if (shaderType == ShaderType::SHADER_PBR) {
        // PBR 着色器：设置所有 PBR 相关 uniforms
        glUniform3fv(glGetUniformLocation(shaderProgram, "albedoColor"), 
                     1, glm::value_ptr(color));
        
        // 设置纹理使用标志
        glUniform1i(glGetUniformLocation(shaderProgram, "useAlbedoMap"), hasTexture ? 1 : 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "useNormalMap"), 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "useMetallicMap"), 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "useRoughnessMap"), 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "useAOMap"), 0);
        
        // 设置 PBR 材质默认值
        glUniform1f(glGetUniformLocation(shaderProgram, "metallicUniform"), 0.1f);
        glUniform1f(glGetUniformLocation(shaderProgram, "roughnessUniform"), 0.5f);
        glUniform1f(glGetUniformLocation(shaderProgram, "aoUniform"), 1.0f);
        
        // 如果有纹理，绑定到对应的纹理单元
        if (hasTexture && textureID != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "albedoMap"), 0);
        }
    }
}