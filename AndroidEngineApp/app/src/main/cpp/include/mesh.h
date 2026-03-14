/**
 * @file mesh.h
 * @brief 网格类的头文件
 * 
 * 该文件定义了Mesh类，用于管理3D模型的网格数据。
 * 包含顶点数据、索引数据、纹理数据的管理，以及VAO/VBO/EBO的设置。
 * 
 * @author 谢兴国   1452492081@qq.com
 * @version 1.0
 * @date 2025-10-12
 * 
 * @note 支持多种顶点属性（位置、纹理坐标、法线、切线、副切线、骨骼信息）
 */
#ifndef _H_MESH_H
#define _H_MESH_H

// 根据平台选择不同的 OpenGL 头文件
#ifdef PLATFORM_ANDROID
    #include <GLES3/gl3.h>
#else
    #include <glad/glad.h>
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.h"
#include "dataType.h"
#include <string>
#include <vector>
using namespace std;

#define MAX_BONE_INFLUENCE 4

struct Vertex {
    // position
    glm::vec3 Position;
    // normal
    glm::vec3 Normal;
    // texCoords
    glm::vec2 TexCoords;
    // tangent
    glm::vec3 Tangent;
    // bitangent
    glm::vec3 Bitangent;
    // Android平台：将bone IDs存储为float，因为GLES不支持整数属性
    float m_BoneIDs[MAX_BONE_INFLUENCE];
    // weights from each bone
    float m_Weights[MAX_BONE_INFLUENCE];
};

struct Texture {
    unsigned int id;
    string type;
    string path;
};

class Mesh {
public:
    // mesh Data
    vector<Vertex>       vertices;
    vector<unsigned int> indices;
    vector<Texture>      textures;
    unsigned int VAO;

    // constructor
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures);

    void Draw(Shader &shader,  ShaderType shaderType);

private:
    // render data 
    unsigned int VBO, EBO;
    void setupMesh();

};
#endif