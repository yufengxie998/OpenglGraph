/**
 * @file ModelObject.h
 * @brief 模型对象类的头文件
 * 
 * 该文件定义了ModelObject类，继承自Entity，用于管理场景中的模型实例。
 * 支持模型加载、动画播放、纹理管理、变换设置等功能。
 * 
 * @author 谢兴国   1452492081@qq.com
 * @version 1.0
 * @date 2025-10-12
 * 
 * @note 支持普通材质和PBR材质两种渲染管线，支持骨骼动画
 */
// ModelObject.h
#ifndef MODEL_OBJECT_H
#define MODEL_OBJECT_H

#include "entity.h"
#include "shader.h"
#include "model.h"
#include <extend/animation.h>
#include <extend/animator.h>
#include <string>
#include <vector>
#include <memory>

namespace Engine
{
    class ModelObject : public Entity
    {
    public:
        ModelObject(const std::string& path);
        virtual ~ModelObject() = default;

        // 重写 Entity 的虚函数
        virtual void Update(float deltaTime) override;
        virtual void Render(Shader& shader, ShaderType shaderType) override;

        // 动画相关方法
        void LoadAnimation(const std::string& animationPath);
        void PlayAnimation();
        void StopAnimation();
        void SetAnimationSpeed(float speed);
        // 添加缺失的查询方法
        bool HasAnimation() const { return animation != nullptr; }
        bool IsAnimationPlaying() const { return animationPlaying; }

        // 纹理相关 - 修改为两个参数的版本（兼容旧代码）
        void SetTexture(unsigned int textureID, const std::string& texturePath) {
            SetTexture(textureID, texturePath, "texture_diffuse"); // 默认类型为漫反射
        }
        // 纹理相关
        void SetTexture(unsigned int textureID, const std::string& texturePath, const std::string& type);
        const std::vector<Texture>& GetTextures() const { return textures; }

        // 兼容旧接口的方法（可选）
        // void SetPosition(const glm::vec3& pos);
        // void SetRotation(const glm::vec3& axis, float angle);
        // void SetScale(const glm::vec3& s);

        // 模型相关
        Model* GetModel() const { return model.get(); }
        bool IsLoaded() const { return model != nullptr; }

    private:
        std::unique_ptr<Model> model;
        std::unique_ptr<Animation> animation;
        std::unique_ptr<Animator> animator;
        std::vector<Texture> textures;

        bool animationPlaying = false;
        float animationSpeed = 1.0f;
    };
}

#endif // MODEL_OBJECT_H

 