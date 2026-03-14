/**
 * @file model.h
 * @brief 模型类的头文件
 * 
 * 该文件定义了Model类，用于加载和处理3D模型文件。
 * 支持多种格式的模型加载（通过Assimp库），自动处理网格、材质和纹理。
 * 
 * @author 谢兴国   1452492081@qq.com
 * @version 1.0
 * @date 2025-10-12
 * 
 * @note 支持嵌入式纹理和外部纹理文件，自动处理模型层级结构
 */
#ifndef _H_MODEL_H
#define _H_MODEL_H


#ifdef PLATFORM_ANDROID
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif
#include <extend/logger.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "mesh.h"
#include "shader.h"
#include "dataType.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <extend/animdata.h>
using namespace std;

inline unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);

class Model
{
public:
    // model data
    vector<Texture> textures_loaded; // stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh> meshes;
    string directory;
    bool gammaCorrection;

    // constructor, expects a filepath to a 3D model.
    Model(string const &path, bool gamma = false);// : gammaCorrection(gamma)

    void Draw(Shader &shader,  ShaderType shaderType);
    // 添加骨骼动画相关方法
    std::map<std::string, BoneInfo>& GetBoneInfoMap() { return m_BoneInfoMap; }
    int& GetBoneCount() { return m_BoneCounter; }
    
    void SetBoneInfoMap(const std::map<std::string, BoneInfo>& boneInfoMap) {
        m_BoneInfoMap = boneInfoMap;
    }
private:
    std::map<std::string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;
    unsigned int TextureFromEmbedded(aiTexture* embeddedTex) {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        
        if (embeddedTex->mHeight == 0) {
            // 压缩格式（如 PNG）
            int width, height, channels;
            unsigned char* data = stbi_load_from_memory(
                reinterpret_cast<unsigned char*>(embeddedTex->pcData),
                embeddedTex->mWidth,
                &width, &height, &channels, 4
            );
            
            if (data) {
                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, 
                             GL_RGBA, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);
                
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                
                stbi_image_free(data);
            } else {
                LOGE("Model", "Failed to load embedded texture data");
            }
        } else {
            // 未压缩格式（如 RAW）
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, embeddedTex->mWidth, embeddedTex->mHeight,
                         0, GL_RGBA, GL_UNSIGNED_BYTE, embeddedTex->pcData);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        
        return textureID;
    }
    
    void loadModel(string const &path);
    void processNode(aiNode *node, const aiScene *scene);

    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName, const aiScene* scene);
};


inline unsigned int TextureFromFile(const char *path, const string &directory, bool gamma)
{
    if (gamma)
    {
    }
    // 🔴 立即修复：空指针检查
    if (!path)
    {
        std::cerr << "❌ ERROR: TextureFromFile received NULL path!"
                  << " [File: " << __FILE__ << ", Line: " << __LINE__ << "]" << std::endl;
        return 0; // 返回无效纹理ID
    }

    // 空字符串检查
    if (path[0] == '\0')
    {
        std::cerr << "❌ ERROR: TextureFromFile received empty string path!" << std::endl;
        return 0;
    }
    string filename = string(path);
    filename = directory + '/' + filename;
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "inline  TextureFromFile  Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

#endif
