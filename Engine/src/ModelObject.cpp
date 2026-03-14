#include "ModelObject.h"
// SPDX-License-Identifier: GPL-3.0-or-later
namespace Engine
{

    ModelObject::ModelObject(const std::string& path): Entity() {
        LOGI("ModelObject", "Creating ModelObject with path: %s", path.c_str());
        
        try {
            model = std::make_unique<Model>(path);
            
            if (model) {
                // 设置 Entity 的模型
                setModel(*model);
                LOGI("ModelObject", "Model created successfully");
                LOGI("ModelObject", "Mesh count: %zu", model->meshes.size());
                
                // 移除 modelMatrix 相关代码 - 不再需要
                // transform.computeModelMatrix();  // 如果需要，可以保留
                // modelMatrix = transform.getModelMatrix();  // 删除这行
                
                // 从模型的网格中提取纹理
                textures.clear();
                for (auto& mesh : model->meshes) {
                    // 将网格的纹理添加到模型的纹理列表
                    for (auto& tex : mesh.textures) {
                        textures.push_back(tex);
                        LOGI("ModelObject", "Found embedded texture: ID=%d, type=%s, path=%s", 
                             tex.id, tex.type.c_str(), tex.path.c_str());
                    }
                }
                
                // 输出统计信息
                LOGI("ModelObject", "Total embedded textures: %zu", textures.size());
            }
        } catch (const std::exception& e) {
            LOGE("ModelObject", "Exception in ModelObject constructor: %s", e.what());
        }
    }

    void ModelObject::Update(float deltaTime)
    {
        // 先调用基类的Update（如果需要更新父子关系等）
        Entity::Update(deltaTime);
        
        // 移除 modelMatrix 的更新 - 直接从 transform 获取
        // modelMatrix = transform.getModelMatrix();  // 删除这行
        
        // 更新动画
        if (animator && animationPlaying)
        {
            animator->UpdateAnimation(deltaTime * animationSpeed);
        }
    }

    void ModelObject::Render(Shader &shader, ShaderType shaderType)
    {
        if (!model) return;
        
        // 直接从 Entity 的 transform 获取模型矩阵并设置
        shader.setMat4("model", transform.getModelMatrix());
        
        static int frameCount = 0;
        frameCount++;

        // 如果有动画，上传骨骼矩阵
        if (animator && animationPlaying)
        {
            auto boneMatrices = animator->GetFinalBoneMatrices();
            for (int i = 0; i < boneMatrices.size(); i++)
            {
                shader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", boneMatrices[i]);
            }
        }

        if (shaderType == ShaderType::SHADER_SIMPLE)
        {
            // 简单着色器模式 - 使用 texture_diffuse1
            if (!textures.empty())
            {
                // 查找漫反射纹理
                int diffuseIndex = -1;
                for (size_t i = 0; i < textures.size(); i++)
                {
                    if (textures[i].type == "texture_diffuse")
                    {
                        diffuseIndex = i;
                        break;
                    }
                }

                if (diffuseIndex >= 0)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, textures[diffuseIndex].id);
                    shader.setInt("texture_diffuse1", 0);
                    shader.setBool("hasTexture", true);

                    static bool logged = false;
                    if (!logged)
                    {
                        LOGI("ModelObject", "Simple shader: Using diffuse texture ID: %d",
                             textures[diffuseIndex].id);
                        logged = true;
                    }
                }
                else
                {
                    shader.setBool("hasTexture", false);
                }
            }
            else
            {
                shader.setBool("hasTexture", false);
            }

            // 绘制模型
            model->Draw(shader, shaderType);
        }
        else
        {
            // PBR 着色器模式 - 使用多种纹理
            // 设置纹理标志
            bool hasAlbedo = false;
            bool hasNormal = false;
            bool hasMetallic = false;
            bool hasRoughness = false;
            bool hasAO = false;
            bool hasEmissive = false;

            // 绑定所有纹理到对应的纹理单元
            for (size_t i = 0; i < textures.size(); i++)
            {
                const auto &tex = textures[i];

                if (tex.type == "texture_diffuse")
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, tex.id);
                    shader.setInt("albedoMap", 0);
                    hasAlbedo = true;
                }
                else if (tex.type == "texture_normal")
                {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, tex.id);
                    shader.setInt("normalMap", 1);
                    hasNormal = true;
                }
                else if (tex.type == "texture_metallic" || tex.type == "texture_specular")
                {
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, tex.id);
                    shader.setInt("metallicMap", 2);
                    hasMetallic = true;
                }
                else if (tex.type == "texture_roughness")
                {
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, tex.id);
                    shader.setInt("roughnessMap", 3);
                    hasRoughness = true;
                }
                else if (tex.type == "texture_ao" || tex.type == "texture_ambient")
                {
                    glActiveTexture(GL_TEXTURE4);
                    glBindTexture(GL_TEXTURE_2D, tex.id);
                    shader.setInt("aoMap", 4);
                    hasAO = true;
                }
                else if (tex.type == "texture_emissive")
                {
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_2D, tex.id);
                    shader.setInt("emissiveMap", 5);
                    hasEmissive = true;
                }
            }

            // 设置纹理使用标志
            shader.setBool("useAlbedoMap", hasAlbedo);
            shader.setBool("useNormalMap", hasNormal);
            shader.setBool("useMetallicMap", hasMetallic);
            shader.setBool("useRoughnessMap", hasRoughness);
            shader.setBool("useAOMap", hasAO);
            shader.setBool("useEmissiveMap", hasEmissive);

            static bool logged = false;
            if (!logged)
            {
                LOGI("ModelObject", "PBR shader: Albedo:%d Normal:%d Metallic:%d Roughness:%d AO:%d Emissive:%d",
                     hasAlbedo, hasNormal, hasMetallic, hasRoughness, hasAO, hasEmissive);
                logged = true;
            }

            // 绘制模型
            model->Draw(shader, shaderType);
        }

        // 解绑纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void ModelObject::LoadAnimation(const std::string& animationPath) {
        try {
            animation = std::make_unique<Animation>(animationPath, model.get());
            animator = std::make_unique<Animator>(animation.get());
            
            // 将骨骼信息保存回模型
            model->SetBoneInfoMap(animation->GetBoneIDMap());
            
            LOGI("ModelObject", "Animation loaded successfully from: %s", animationPath.c_str());
            LOGI("ModelObject", "Duration: %.2f, TicksPerSecond: %f", 
                 animation->GetDuration(), animation->GetTicksPerSecond());
        } catch (const std::exception& e) {
            LOGE("ModelObject", "Failed to load animation: %s", e.what());
        }
    }
    
    void ModelObject::PlayAnimation() {
        animationPlaying = true;
        LOGI("ModelObject", "Animation started");
    }
    
    void ModelObject::StopAnimation() {
        animationPlaying = false;
        LOGI("ModelObject", "Animation stopped");
    }
    
    void ModelObject::SetAnimationSpeed(float speed) {
        animationSpeed = speed;
        LOGI("ModelObject", "Animation speed set to: %.2f", speed);
    }

    // 如果需要兼容旧的接口，可以取消注释这些方法
    /*
    void ModelObject::SetPosition(const glm::vec3 &pos)
    {
        setLocalPosition(pos);
    }

    void ModelObject::SetRotation(const glm::vec3 &axis, float angle)
    {
        // 将轴角转换为欧拉角（简化版本）
        if (axis.y > 0.5f)
        {
            setLocalRotation(glm::vec3(0, angle, 0));
        }
        else if (axis.x > 0.5f)
        {
            setLocalRotation(glm::vec3(angle, 0, 0));
        }
        else if (axis.z > 0.5f)
        {
            setLocalRotation(glm::vec3(0, 0, angle));
        }
    }

    void ModelObject::SetScale(const glm::vec3 &s)
    {
        setLocalScale(s);
    }
    */

    void ModelObject::SetTexture(unsigned int textureID, const std::string &texturePath, const std::string &type)
    {
        Texture tex;
        tex.id = textureID;
        tex.type = type;
        tex.path = texturePath;

        textures.push_back(tex);

        // 应用到模型的所有网格
        for (auto &mesh : model->meshes)
        {
            mesh.textures.clear();
            mesh.textures.push_back(tex);
        }
    }

} // namespace Engine