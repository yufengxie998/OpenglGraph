#include "entity.h"
#include "Shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <limits>
#include <algorithm>
// SPDX-License-Identifier: GPL-3.0-or-later
// 初始化静态成员
int Entity::s_nextId = 0;

Entity::Entity() : m_id(s_nextId++), pModel(nullptr) {
}

Entity::Entity(Model& model) : m_id(s_nextId++), pModel(&model)
{
    updateBoundingVolume();
}

void Entity::Update(float deltaTime) {
    updateSelfAndChild();
}

void Entity::setLocalPosition(const glm::vec3& newPosition) {
    transform.setLocalPosition(newPosition);
}

void Entity::setLocalRotationAxis(float angle, const glm::vec3& axis) {
    transform.setLocalRotationAxis(angle, axis);
}

void Entity::setLocalRotation(const glm::vec3& newRotation) {
    transform.setLocalRotation(newRotation);
}

void Entity::setLocalScale(const glm::vec3& newScale) {
    transform.setLocalScale(newScale);
}

glm::vec3 Entity::getLocalPosition() const {
    return transform.getLocalPosition();
}

glm::vec3 Entity::getLocalRotation() const {
    return transform.getLocalRotation();
}

glm::vec3 Entity::getLocalScale() const {
    return transform.getLocalScale();
}

void Entity::setModel(Model& model) {
    pModel = &model;
    updateBoundingVolume();
}

AABB Entity::getGlobalAABB() const
{
    if (!boundingVolume) {
        return AABB(glm::vec3(0.0f), 0.0f, 0.0f, 0.0f);
    }
    
    const glm::vec3 globalCenter = glm::vec3(
        transform.getModelMatrix() * glm::vec4(boundingVolume->center, 1.0f)
    );

    const glm::vec3 right = transform.getRight() * boundingVolume->extents.x;
    const glm::vec3 up = transform.getUp() * boundingVolume->extents.y;
    const glm::vec3 forward = transform.getForward() * boundingVolume->extents.z;

    const float newIi = std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, right)) +
                        std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, up)) +
                        std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, forward));

    const float newIj = std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, right)) +
                        std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, up)) +
                        std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, forward));

    const float newIk = std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, right)) +
                        std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, up)) +
                        std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, forward));

    return AABB(globalCenter, newIi, newIj, newIk);
}

void Entity::updateSelfAndChild()
{
    if (transform.isDirty()) {
        forceUpdateSelfAndChild();
        return;
    }
    
    for (auto&& child : m_children)
    {
        child->updateSelfAndChild();
    }
}

void Entity::forceUpdateSelfAndChild()
{
    if (auto parent = m_parent.lock()) {
        transform.computeModelMatrix(parent->transform.getModelMatrix());
    } else {
        transform.computeModelMatrix();
    }

    for (auto&& child : m_children)
    {
        child->forceUpdateSelfAndChild();
    }
}

void Entity::drawSelfAndChild(const Frustum& frustum, Shader& ourShader, 
                              unsigned int& display, unsigned int& total)
{
    total++;
    
    if (boundingVolume && boundingVolume->isOnFrustum(frustum, transform))
    {
        ourShader.setMat4("model", transform.getModelMatrix());
        if (pModel) {
            pModel->Draw(ourShader, ShaderType::SHADER_SIMPLE);
            display++;
        }
    }

    for (auto&& child : m_children)
    {
        child->drawSelfAndChild(frustum, ourShader, display, total);
    }
}

// Child management implementations
void Entity::removeChild(std::shared_ptr<Entity> child)
{
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        (*it)->m_parent.reset();
        m_children.erase(it);
    }
}

void Entity::removeChild(int childId)
{
    auto it = std::find_if(m_children.begin(), m_children.end(),
        [childId](const auto& child) { return child->GetId() == childId; });
    
    if (it != m_children.end()) {
        (*it)->m_parent.reset();
        m_children.erase(it);
    }
}

void Entity::clearChildren()
{
    for (auto& child : m_children) {
        child->m_parent.reset();
    }
    m_children.clear();
}

std::shared_ptr<Entity> Entity::getChild(int childId) const
{
    auto it = std::find_if(m_children.begin(), m_children.end(),
        [childId](const auto& child) { return child->GetId() == childId; });
    
    return (it != m_children.end()) ? *it : nullptr;
}

std::vector<std::shared_ptr<Entity>> Entity::getAllChildren() const
{
    return m_children;
}

std::shared_ptr<Entity> Entity::getParent() const
{
    return m_parent.lock();
}

void Entity::detachFromParent()
{
    if (auto parent = m_parent.lock()) {
        parent->removeChild(GetId());
    }
}

void Entity::updateBoundingVolume()
{
    if (pModel) {
        boundingVolume = std::make_unique<AABB>(generateAABB(*pModel));
    }
}