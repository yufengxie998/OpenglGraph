// SPDX-License-Identifier: GPL-3.0-or-later
#include "Plane.h"
#include <extend/logger.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
namespace Geometry{
Plane::Plane(float w, float d, int seg) 
    : Primitive(),width(w), depth(d), segments(seg) {
    GenerateGeometry();
    SetupMesh();

        // 为平面创建包围盒
    glm::vec3 min(-width/2, -0.1f, -depth/2);  // 平面在 y=0，稍微有点厚度
    glm::vec3 max(width/2, 0.1f, depth/2);
    boundingVolume = std::make_unique<AABB>(min, max);
}

Plane::~Plane() {}

void Plane::GenerateGeometry() {
    vertices.clear();
    indices.clear();
    
    float halfW = width / 2.0f;
    float halfD = depth / 2.0f;
    float stepW = width / segments;
    float stepD = depth / segments;
    
    // 生成顶点
    for (int i = 0; i <= segments; i++) {
        for (int j = 0; j <= segments; j++) {
            PrimitiveVertex vertex;
            
            // 位置
            float x = -halfW + i * stepW;
            float z = -halfD + j * stepD;
            vertex.Position = glm::vec3(x, 0.0f, z);
            
            // 法线（向上）
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            
            // 纹理坐标
            vertex.TexCoords = glm::vec2(
                (float)i / segments,
                (float)j / segments
            );
            
            // 切线（沿X方向）
            vertex.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            
            // 副切线（沿Z方向）
            vertex.Bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
            
            vertices.push_back(vertex);
        }
    }
    
    // 生成索引
    for (int i = 0; i < segments; i++) {
        for (int j = 0; j < segments; j++) {
            int start = i * (segments + 1) + j;
            
            // 两个三角形组成一个网格
            // 三角形1
            indices.push_back(start);
            indices.push_back(start + segments + 1);
            indices.push_back(start + 1);
            
            // 三角形2
            indices.push_back(start + 1);
            indices.push_back(start + segments + 1);
            indices.push_back(start + segments + 2);
        }
    }
    
    LOGI("Plane", "Generated plane: %zu vertices, %zu indices", 
         vertices.size(), indices.size());
}

void Plane::Draw(GLuint shaderProgram, ShaderType shaderType) {
    if (!initialized) return;
    
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    
    // 设置材质 uniforms（包含模型矩阵）
    SetMaterialUniforms(shaderProgram, shaderType);

    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // 解绑纹理
    if (hasTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Plane::SetSize(float w, float d) {
    width = w;
    depth = d;
    GenerateGeometry();
    SetupMesh();
}

void Plane::SetSegments(int seg) {
    segments = seg;
    GenerateGeometry();
    SetupMesh();
}
}