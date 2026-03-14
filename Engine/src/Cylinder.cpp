#include "Cylinder.h"
#include <extend/logger.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <vector>
namespace Geometry{
Cylinder::Cylinder(float r, float h, int s) 
    : Primitive(), radius(r), height(h), sectors(s) {
    GenerateGeometry();
    SetupMesh();

    // 为圆柱体创建包围盒
    glm::vec3 min(-radius, -height/2, -radius);
    glm::vec3 max(radius, height/2, radius);
    boundingVolume = std::make_unique<AABB>(min, max);
}

Cylinder::~Cylinder() {}

void Cylinder::GenerateGeometry() {
    vertices.clear();
    indices.clear();
    
    float halfHeight = height / 2.0f;
    
    // 生成侧面顶点
    for (int i = 0; i <= sectors; i++) {
        float angle = 2.0f * M_PI * i / sectors;
        float cosA = cos(angle);
        float sinA = sin(angle);
        
        // 底部顶点
        PrimitiveVertex vBottom;
        vBottom.Position = glm::vec3(radius * cosA, -halfHeight, radius * sinA);
        vBottom.Normal = glm::vec3(cosA, 0.0f, sinA);
        vBottom.TexCoords = glm::vec2((float)i / sectors, 0.0f);
        vBottom.Tangent = glm::vec3(-sinA, 0.0f, cosA);
        vBottom.Bitangent = glm::vec3(0.0f, 1.0f, 0.0f);
        vertices.push_back(vBottom);
        
        // 顶部顶点
        PrimitiveVertex vTop;
        vTop.Position = glm::vec3(radius * cosA, halfHeight, radius * sinA);
        vTop.Normal = glm::vec3(cosA, 0.0f, sinA);
        vTop.TexCoords = glm::vec2((float)i / sectors, 1.0f);
        vTop.Tangent = glm::vec3(-sinA, 0.0f, cosA);
        vTop.Bitangent = glm::vec3(0.0f, 1.0f, 0.0f);
        vertices.push_back(vTop);
    }
    
    // 生成侧面索引
    for (int i = 0; i < sectors; i++) {
        int bottom1 = i * 2;
        int top1 = bottom1 + 1;
        int bottom2 = (i + 1) * 2;
        int top2 = bottom2 + 1;
        
        // 两个三角形组成一个四边形
        indices.push_back(bottom1);
        indices.push_back(top1);
        indices.push_back(bottom2);
        
        indices.push_back(bottom2);
        indices.push_back(top1);
        indices.push_back(top2);
    }
    
    // 记录侧面顶点数量，用于底部和顶部的索引偏移
    int sideVertexCount = vertices.size();
    
    // 生成底部盖子
    PrimitiveVertex vBottomCenter;
    vBottomCenter.Position = glm::vec3(0.0f, -halfHeight, 0.0f);
    vBottomCenter.Normal = glm::vec3(0.0f, -1.0f, 0.0f);
    vBottomCenter.TexCoords = glm::vec2(0.5f, 0.5f);
    vBottomCenter.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
    vBottomCenter.Bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
    vertices.push_back(vBottomCenter);
    int bottomCenterIndex = sideVertexCount;
    
    for (int i = 0; i < sectors; i++) {
        float angle1 = 2.0f * M_PI * i / sectors;
        float angle2 = 2.0f * M_PI * (i + 1) / sectors;
        
        PrimitiveVertex v1, v2;
        
        v1.Position = glm::vec3(radius * cos(angle1), -halfHeight, radius * sin(angle1));
        v1.Normal = glm::vec3(0.0f, -1.0f, 0.0f);
        v1.TexCoords = glm::vec2(0.5f + 0.5f * cos(angle1), 0.5f + 0.5f * sin(angle1));
        v1.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        v1.Bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
        vertices.push_back(v1);
        
        v2.Position = glm::vec3(radius * cos(angle2), -halfHeight, radius * sin(angle2));
        v2.Normal = glm::vec3(0.0f, -1.0f, 0.0f);
        v2.TexCoords = glm::vec2(0.5f + 0.5f * cos(angle2), 0.5f + 0.5f * sin(angle2));
        v2.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        v2.Bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
        vertices.push_back(v2);
        
        int idx1 = sideVertexCount + 1 + i * 2;
        int idx2 = sideVertexCount + 1 + i * 2 + 1;
        
        indices.push_back(bottomCenterIndex);
        indices.push_back(idx2);
        indices.push_back(idx1);
    }
    
    // 生成顶部盖子
    int topCenterIndex = vertices.size();
    PrimitiveVertex vTopCenter;
    vTopCenter.Position = glm::vec3(0.0f, halfHeight, 0.0f);
    vTopCenter.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
    vTopCenter.TexCoords = glm::vec2(0.5f, 0.5f);
    vTopCenter.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
    vTopCenter.Bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
    vertices.push_back(vTopCenter);
    
    for (int i = 0; i < sectors; i++) {
        float angle1 = 2.0f * M_PI * i / sectors;
        float angle2 = 2.0f * M_PI * (i + 1) / sectors;
        
        PrimitiveVertex v1, v2;
        
        v1.Position = glm::vec3(radius * cos(angle1), halfHeight, radius * sin(angle1));
        v1.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        v1.TexCoords = glm::vec2(0.5f + 0.5f * cos(angle1), 0.5f + 0.5f * sin(angle1));
        v1.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        v1.Bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
        vertices.push_back(v1);
        
        v2.Position = glm::vec3(radius * cos(angle2), halfHeight, radius * sin(angle2));
        v2.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        v2.TexCoords = glm::vec2(0.5f + 0.5f * cos(angle2), 0.5f + 0.5f * sin(angle2));
        v2.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        v2.Bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
        vertices.push_back(v2);
        
        int idx1 = topCenterIndex + 1 + i * 2;
        int idx2 = topCenterIndex + 1 + i * 2 + 1;
        
        indices.push_back(topCenterIndex);
        indices.push_back(idx1);
        indices.push_back(idx2);
    }
    
    LOGI("Cylinder", "Generated cylinder: %zu vertices, %zu indices", 
         vertices.size(), indices.size());
}

void Cylinder::Draw(GLuint shaderProgram, ShaderType shaderType) {
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

void Cylinder::SetRadius(float r) {
    radius = r;
    GenerateGeometry();
    SetupMesh();
}

void Cylinder::SetHeight(float h) {
    height = h;
    GenerateGeometry();
    SetupMesh();
}

void Cylinder::SetSectors(int s) {
    sectors = s;
    GenerateGeometry();
    SetupMesh();
}
}