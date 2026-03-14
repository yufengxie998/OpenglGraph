/**
 * @file entity.h
 * @brief 实体系统类的头文件
 * 
 * 该文件定义了Transform、Entity以及视锥体剔除相关的类，
 * 实现了场景对象的变换层级、包围盒计算和视锥体裁剪功能。
 * 
 * @author 谢兴国   1452492081@qq.com
 * @version 1.0
 * @date 2025-10-12
 * 
 * @note 支持AABB、包围球等多种包围体，提供完整的视锥体剔除功能
 */
#ifndef _H_ENTITY_H
#define _H_ENTITY_H
#include "Transform.h"
using namespace FrustumMath;

class Entity
{
public:
    // Constructors and destructor
    Entity();
    explicit Entity(Model& model);
    virtual ~Entity() = default;
    // 提供默认实现，这样派生类可以选择性重写
    virtual void Update(float deltaTime);
    int GetId() const { return m_id; }
    virtual void Render(Shader& shader, ShaderType shaderType) = 0;
    // Transform methods
    void setLocalPosition(const glm::vec3& newPosition);
    void setLocalRotationAxis(float angle, const glm::vec3& axis);
    void setLocalRotation(const glm::vec3& newRotation);
    void setLocalScale(const glm::vec3& newScale);
    
    glm::vec3 getLocalPosition() const;
    glm::vec3 getLocalRotation() const;
    glm::vec3 getLocalScale() const;
    
    void setModel(Model& model);
    AABB getGlobalAABB();

    // Child management
    template<typename... TArgs>
    void addChild(TArgs&... args);

    // Update methods
    void updateSelfAndChild();
    void forceUpdateSelfAndChild();
    void drawSelfAndChild(const Frustum& frustum, Shader& ourShader, unsigned int& display, unsigned int& total);
    //Space information
    Transform transform;
    std::unique_ptr<AABB> boundingVolume;
    std::list<std::unique_ptr<Entity>> children;
    Entity* parent = nullptr;
private:
    static int s_nextId;
    int m_id;




    Model* pModel = nullptr;

};

// Template method implementation (must be in header)
template<typename... TArgs>
void Entity::addChild(TArgs&... args)
{
    children.emplace_back(std::make_unique<Entity>(args...));
    children.back()->parent = this;
}

#endif