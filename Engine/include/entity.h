/**
 * @file entity.h
 * @brief 实体系统类的头文件
 * 
 * 该文件定义了Transform、Entity以及视锥体剔除相关的类，
 * 实现了场景对象的变换层级、包围盒计算和视锥体裁剪功能。
 * 
 * @author 谢兴国   1452492081@qq.com
 * @version 2.0
 * @date 2025-10-12
 */
#ifndef _H_ENTITY_H
#define _H_ENTITY_H

#include "Transform.h"
#include <memory>
#include <list>
#include <vector>
#include <glm/glm.hpp>
#include "dataType.h"

using namespace FrustumMath;

// 前向声明
class Shader;

class Entity : public std::enable_shared_from_this<Entity> 
{
public:
    // Constructors and destructor
    Entity();
    explicit Entity(Model& model);
    virtual ~Entity() = default;
    
    // 删除拷贝构造和赋值操作符
    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;
    
    // ID相关
    int GetId() const { return m_id; }
    
    // 虚函数
    virtual void Update(float deltaTime);
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
    AABB getGlobalAABB() const;

    // ============ Child management ============
    // 模板版本：创建并添加新子物体
    template<typename T, typename... Args>
    std::shared_ptr<T> addChild(Args&&... args);
    
    // 重载版本：添加已存在的子物体
    void addChild(std::shared_ptr<Entity> child) {
        if (child) {
            child->m_parent = weak_from_this();
            m_children.push_back(child);
        }
    }
    
    void removeChild(std::shared_ptr<Entity> child);
    void removeChild(int childId);
    void clearChildren();
    
    std::shared_ptr<Entity> getChild(int childId) const;
    std::vector<std::shared_ptr<Entity>> getAllChildren() const;
    
    template<typename T>
    std::vector<std::shared_ptr<T>> getChildrenByType() const;
    
    // Parent management
    std::shared_ptr<Entity> getParent() const;
    void detachFromParent();

    // Update methods
    void updateSelfAndChild();
    void forceUpdateSelfAndChild();
    void drawSelfAndChild(const Frustum& frustum, Shader& ourShader, 
                          unsigned int& display, unsigned int& total);
    
    // Space information
    const Transform& GetTransform() const { return transform; }
    Transform& GetTransform() { return transform; }
    
    const AABB* GetBoundingVolume() const { return boundingVolume.get(); }
    
    // 友元类，允许OcclusionCullingManager访问私有成员
    friend class OcclusionCullingManager;
    Transform transform;
    std::unique_ptr<AABB> boundingVolume;
protected:

    Model* pModel = nullptr;

private:
    static int s_nextId;
    int m_id;
    
    // 使用 weak_ptr 避免循环引用
    std::vector<std::shared_ptr<Entity>> m_children;
    std::weak_ptr<Entity> m_parent;
    
    void updateBoundingVolume();
};

// Template method implementations
template<typename T, typename... Args>
std::shared_ptr<T> Entity::addChild(Args&&... args)
{
    static_assert(std::is_base_of<Entity, T>::value, 
                  "T must be derived from Entity");
    
    // 禁止直接创建 Entity
    static_assert(!std::is_same<Entity, T>::value,
                  "Cannot create Entity directly, use ModelObject or Primitive");
    
    auto child = std::make_shared<T>(std::forward<Args>(args)...);
    child->m_parent = weak_from_this();
    m_children.push_back(child);
    
    return child;
}

template<typename T>
std::vector<std::shared_ptr<T>> Entity::getChildrenByType() const
{
    std::vector<std::shared_ptr<T>> result;
    for (const auto& child : m_children) {
        if (auto casted = std::dynamic_pointer_cast<T>(child)) {
            result.push_back(casted);
        }
    }
    return result;
}

#endif // _H_ENTITY_H