#include "model.h"
// SPDX-License-Identifier: GPL-3.0-or-later
    // constructor, expects a filepath to a 3D model.
    Model::Model(string const &path, bool gamma )
    {
        loadModel(path);
    }

    void Model::Draw(Shader &shader,  ShaderType shaderType)
    {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader,shaderType);
    }

    void Model::loadModel(string const &path)
    {
        LOGI("Model", "Loading model from: %s", path.c_str());

        // 检查文件是否存在
        FILE *file = fopen(path.c_str(), "r");
        if (!file)
        {
            LOGE("Model", "Model file does not exist: %s", path.c_str());
            return;
        }
        fclose(file);

        // 获取文件大小
        file = fopen(path.c_str(), "rb");
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fclose(file);
        LOGI("Model", "Model file size: %ld bytes", fileSize);

        // 使用最基本的处理标志，逐步添加
        // 首先尝试最基本的加载
        unsigned int flags = aiProcess_Triangulate |
                             aiProcess_FlipUVs |
                             aiProcess_JoinIdenticalVertices;

        LOGI("Model", "Attempting with basic Assimp flags: 0x%X", flags);

        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(path, flags);

        if (!scene)
        {
            LOGE("Model", "Basic loading failed: %s", importer.GetErrorString());

            // 如果基本加载失败，尝试添加更多处理标志
            flags |= aiProcess_GenNormals |
                     aiProcess_OptimizeMeshes |
                     aiProcess_RemoveRedundantMaterials |
                     aiProcess_FindDegenerates |
                     aiProcess_FindInvalidData |
                     aiProcess_ValidateDataStructure;

            LOGI("Model", "Attempting with extended flags: 0x%X", flags);

            scene = importer.ReadFile(path, flags);

            if (!scene)
            {
                LOGE("Model", "Extended loading also failed: %s", importer.GetErrorString());
                return;
            }
        }

        if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
        {
            LOGE("Model", "Scene is incomplete");
            return;
        }

        if (!scene->mRootNode)
        {
            LOGE("Model", "No root node");
            return;
        }

        // 输出场景信息
        LOGI("Model", "Scene info:");
        LOGI("Model", "  - Num meshes: %d", scene->mNumMeshes);
        LOGI("Model", "  - Num materials: %d", scene->mNumMaterials);
        LOGI("Model", "  - Num textures: %d", scene->mNumTextures);

        if (scene->mNumMeshes == 0)
        {
            LOGE("Model", "No meshes found in the model file!");
            return;
        }

        directory = path.substr(0, path.find_last_of('/'));
        LOGI("Model", "Directory: %s", directory.c_str());

        processNode(scene->mRootNode, scene);
        LOGI("Model", "Processing complete. Total meshes: %zu", meshes.size());
    }
    void Model::processNode(aiNode *node, const aiScene *scene)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene)
    {
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;

            // 位置
            vertex.Position = glm::vec3(mesh->mVertices[i].x,
                                        mesh->mVertices[i].y,
                                        mesh->mVertices[i].z);

            // 法线
            if (mesh->HasNormals())
            {
                vertex.Normal = glm::vec3(mesh->mNormals[i].x,
                                          mesh->mNormals[i].y,
                                          mesh->mNormals[i].z);
            }

            // 【关键】纹理坐标 - 添加调试信息
            if (mesh->HasTextureCoords(0))
            {
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x,
                                             mesh->mTextureCoords[0][i].y);

                // 打印第一个顶点的纹理坐标作为调试
                if (i == 0)
                {
                    LOGI("Model", "First vertex tex coords: (%.2f, %.2f)",
                         vertex.TexCoords.x, vertex.TexCoords.y);
                }
            }
            else
            {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
                LOGI("Model", "Warning: Vertex %d has no texture coordinates", i);
            }
            // 切线（如果有）
            if (mesh->HasTangentsAndBitangents())
            {
                vertex.Tangent = glm::vec3(mesh->mTangents[i].x,
                                           mesh->mTangents[i].y,
                                           mesh->mTangents[i].z);
                vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x,
                                             mesh->mBitangents[i].y,
                                             mesh->mBitangents[i].z);
            }

            vertices.push_back(vertex);
        }

        // 索引
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // 材质处理...
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        vector<Texture> diffuseMaps = loadMaterialTextures(material,
                                                           aiTextureType_DIFFUSE, "texture_diffuse",scene);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        // 输出最终统计
        LOGI("Model", "Mesh processed - vertices: %zu, each with tex coords: %s",
             vertices.size(), mesh->HasTextureCoords(0) ? "yes" : "no");

        return Mesh(vertices, indices, textures);
    }
    
    vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName, const aiScene* scene)
    {
        vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            
            // 检查是否是嵌入式纹理
            if (str.data[0] == '*') {
                // 嵌入式纹理
                unsigned int textureIndex = atoi(str.data + 1);
                if (scene->HasTextures() && textureIndex < scene->mNumTextures) {
                    aiTexture* embeddedTex = scene->mTextures[textureIndex];
                    LOGI("Model", "Loading embedded texture: index=%d, format=%s", 
                         textureIndex, embeddedTex->achFormatHint);
                    
                    // 从嵌入式数据创建纹理
                    Texture texture;
                    texture.id = TextureFromEmbedded(embeddedTex);
                    texture.type = typeName;
                    texture.path = str.C_Str();
                    textures.push_back(texture);
                    textures_loaded.push_back(texture);
                    continue;
                }
            }
            
            // 普通文件纹理
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            
            if(!skip)
            {
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }

