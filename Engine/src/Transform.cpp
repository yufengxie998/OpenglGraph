/**
 * @file Transform.cpp
 * @brief 变换和视锥体剔除相关类的实现文件
 * 
 * 该文件实现了Transform类和FrustumMath命名空间下的
 * 各种包围体结构的成员函数。
 * 
 * @author 谢兴国   1452492081@qq.com
 * @version 1.0
 * @date 2025-10-12
 */

 #include "Transform.h"
 #include <glm/gtc/matrix_transform.hpp>
 #include <glm/gtc/quaternion.hpp>
 #include <limits>
 
 //=============================================================================
 // Transform 类实现
 //=============================================================================
 
 glm::mat4 Transform::getLocalModelMatrix()
 {
     const glm::mat4 transformX = glm::rotate(glm::mat4(1.0f), glm::radians(m_eulerRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
     const glm::mat4 transformY = glm::rotate(glm::mat4(1.0f), glm::radians(m_eulerRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
     const glm::mat4 transformZ = glm::rotate(glm::mat4(1.0f), glm::radians(m_eulerRot.z), glm::vec3(0.0f, 0.0f, 1.0f));
 
     // Y * X * Z
     const glm::mat4 rotationMatrix = transformY * transformX * transformZ;
 
     // translation * rotation * scale (also know as TRS matrix)
     return glm::translate(glm::mat4(1.0f), m_pos) * rotationMatrix * glm::scale(glm::mat4(1.0f), m_scale);
 }
 
 void Transform::computeModelMatrix()
 {
     m_modelMatrix = getLocalModelMatrix();
     m_isDirty = false;
 }
 
 void Transform::computeModelMatrix(const glm::mat4& parentGlobalModelMatrix)
 {
     m_modelMatrix = parentGlobalModelMatrix * getLocalModelMatrix();
     m_isDirty = false;
 }
 
 void Transform::setLocalPosition(const glm::vec3& newPosition)
 {
     m_pos = newPosition;
     m_isDirty = true;
 }
 
 void Transform::setLocalRotation(const glm::vec3& newRotation)
 {
     m_eulerRot = newRotation;
     m_isDirty = true;
 }
 
 void Transform::setLocalRotationAxis(float angle, const glm::vec3& axis)
 {
     // 根据主旋转轴设置欧拉角（简化版）
     if (axis.y > 0.5f) {
         // 主要绕Y轴旋转
         m_eulerRot.y = angle;
     } else if (axis.x > 0.5f) {
         // 主要绕X轴旋转
         m_eulerRot.x = angle;
     } else if (axis.z > 0.5f) {
         // 主要绕Z轴旋转
         m_eulerRot.z = angle;
     }
     
     m_isDirty = true;
 }
 
 void Transform::setLocalScale(const glm::vec3& newScale)
 {
     m_scale = newScale;
     m_isDirty = true;
 }
 
 glm::vec3 Transform::getGlobalPosition() const
 {
     return glm::vec3(m_modelMatrix[3]);
 }
 
 const glm::vec3& Transform::getLocalPosition() const
 {
     return m_pos;
 }
 
 const glm::vec3& Transform::getLocalRotation() const
 {
     return m_eulerRot;
 }
 
 const glm::vec3& Transform::getLocalScale() const
 {
     return m_scale;
 }
 
 const glm::mat4& Transform::getModelMatrix() const
 {
     return m_modelMatrix;
 }
 
 glm::vec3 Transform::getRight() const
 {
     return m_modelMatrix[0];
 }
 
 glm::vec3 Transform::getUp() const
 {
     return m_modelMatrix[1];
 }
 
 glm::vec3 Transform::getBackward() const
 {
     return m_modelMatrix[2];
 }
 
 glm::vec3 Transform::getForward() const
 {
     return -m_modelMatrix[2];
 }
 
 glm::vec3 Transform::getGlobalScale() const
 {
     return { glm::length(getRight()), glm::length(getUp()), glm::length(getBackward()) };
 }
 
 bool Transform::isDirty() const
 {
     return m_isDirty;
 }
 
 //=============================================================================
 // FrustumMath 命名空间实现
 //=============================================================================
 
 namespace FrustumMath {
 
 //-----------------------------------------------------------------------------
 // Plane 结构实现
 //-----------------------------------------------------------------------------
 
 Plane::Plane(const glm::vec3& p1, const glm::vec3& norm)
     : normal(glm::normalize(norm)),
     distance(glm::dot(normal, p1))
 {}
 
 float Plane::getSignedDistanceToPlane(const glm::vec3& point) const
 {
     return glm::dot(normal, point) - distance;
 }
 
 //-----------------------------------------------------------------------------
 // BoundingVolume 结构实现
 //-----------------------------------------------------------------------------
 
 bool BoundingVolume::isOnFrustum(const Frustum& camFrustum) const
 {
     return (isOnOrForwardPlane(camFrustum.leftFace) &&
             isOnOrForwardPlane(camFrustum.rightFace) &&
             isOnOrForwardPlane(camFrustum.topFace) &&
             isOnOrForwardPlane(camFrustum.bottomFace) &&
             isOnOrForwardPlane(camFrustum.nearFace) &&
             isOnOrForwardPlane(camFrustum.farFace));
 }
 
 //-----------------------------------------------------------------------------
 // Sphere 结构实现
 //-----------------------------------------------------------------------------
 
 Sphere::Sphere(const glm::vec3& inCenter, float inRadius)
     : BoundingVolume{}, center{ inCenter }, radius{ inRadius }
 {}
 
 bool Sphere::isOnOrForwardPlane(const Plane& plane) const
 {
     return plane.getSignedDistanceToPlane(center) > -radius;
 }
 
 bool Sphere::isOnFrustum(const Frustum& camFrustum, const Transform& transform) const
 {
     // Get global scale thanks to our transform
     const glm::vec3 globalScale = transform.getGlobalScale();
 
     // Get our global center with process it with the global model matrix of our transform
     const glm::vec3 globalCenter{ transform.getModelMatrix() * glm::vec4(center, 1.f) };
 
     // To wrap correctly our shape, we need the maximum scale scalar.
     const float maxScale = std::max(std::max(globalScale.x, globalScale.y), globalScale.z);
 
     // Max scale is assuming for the diameter. So, we need the half to apply it to our radius
     Sphere globalSphere(globalCenter, radius * (maxScale * 0.5f));
 
     // Check Firstly the result that have the most chance to failure to avoid to call all functions.
     return (globalSphere.isOnOrForwardPlane(camFrustum.leftFace) &&
             globalSphere.isOnOrForwardPlane(camFrustum.rightFace) &&
             globalSphere.isOnOrForwardPlane(camFrustum.farFace) &&
             globalSphere.isOnOrForwardPlane(camFrustum.nearFace) &&
             globalSphere.isOnOrForwardPlane(camFrustum.topFace) &&
             globalSphere.isOnOrForwardPlane(camFrustum.bottomFace));
 }
 
 //-----------------------------------------------------------------------------
 // SquareAABB 结构实现
 //-----------------------------------------------------------------------------
 
 SquareAABB::SquareAABB(const glm::vec3& inCenter, float inExtent)
     : BoundingVolume{}, center{ inCenter }, extent{ inExtent }
 {}
 
 bool SquareAABB::isOnOrForwardPlane(const Plane& plane) const
 {
     // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
     const float r = extent * (std::abs(plane.normal.x) + std::abs(plane.normal.y) + std::abs(plane.normal.z));
     return -r <= plane.getSignedDistanceToPlane(center);
 }
 
 bool SquareAABB::isOnFrustum(const Frustum& camFrustum, const Transform& transform) const
 {
     // Get global scale thanks to our transform
     const glm::vec3 globalCenter{ transform.getModelMatrix() * glm::vec4(center, 1.f) };
 
     // Scaled orientation
     const glm::vec3 right = transform.getRight() * extent;
     const glm::vec3 up = transform.getUp() * extent;
     const glm::vec3 forward = transform.getForward() * extent;
 
     const float newIi = std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, right)) +
         std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, up)) +
         std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, forward));
 
     const float newIj = std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, right)) +
         std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, up)) +
         std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, forward));
 
     const float newIk = std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, right)) +
         std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, up)) +
         std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, forward));
 
     const SquareAABB globalAABB(globalCenter, std::max(std::max(newIi, newIj), newIk));
 
     return (globalAABB.isOnOrForwardPlane(camFrustum.leftFace) &&
             globalAABB.isOnOrForwardPlane(camFrustum.rightFace) &&
             globalAABB.isOnOrForwardPlane(camFrustum.topFace) &&
             globalAABB.isOnOrForwardPlane(camFrustum.bottomFace) &&
             globalAABB.isOnOrForwardPlane(camFrustum.nearFace) &&
             globalAABB.isOnOrForwardPlane(camFrustum.farFace));
 }
 
 //-----------------------------------------------------------------------------
 // AABB 结构实现
 //-----------------------------------------------------------------------------
 
 AABB::AABB(const glm::vec3& min, const glm::vec3& max)
     : BoundingVolume{}, center{ (max + min) * 0.5f }, extents{ max.x - center.x, max.y - center.y, max.z - center.z }
 {}
 
 AABB::AABB(const glm::vec3& inCenter, float iI, float iJ, float iK)
     : BoundingVolume{}, center{ inCenter }, extents{ iI, iJ, iK }
 {}
 
 std::array<glm::vec3, 8> AABB::getVertice() const
 {
     std::array<glm::vec3, 8> vertice;
     vertice[0] = { center.x - extents.x, center.y - extents.y, center.z - extents.z };
     vertice[1] = { center.x + extents.x, center.y - extents.y, center.z - extents.z };
     vertice[2] = { center.x - extents.x, center.y + extents.y, center.z - extents.z };
     vertice[3] = { center.x + extents.x, center.y + extents.y, center.z - extents.z };
     vertice[4] = { center.x - extents.x, center.y - extents.y, center.z + extents.z };
     vertice[5] = { center.x + extents.x, center.y - extents.y, center.z + extents.z };
     vertice[6] = { center.x - extents.x, center.y + extents.y, center.z + extents.z };
     vertice[7] = { center.x + extents.x, center.y + extents.y, center.z + extents.z };
     return vertice;
 }
 
 bool AABB::isOnOrForwardPlane(const Plane& plane) const
 {
     // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
     const float r = extents.x * std::abs(plane.normal.x) + extents.y * std::abs(plane.normal.y) +
         extents.z * std::abs(plane.normal.z);
 
     return -r <= plane.getSignedDistanceToPlane(center);
 }
 
 bool AABB::isOnFrustum(const Frustum& camFrustum, const Transform& transform) const
 {
     // Get global scale thanks to our transform
     const glm::vec3 globalCenter{ transform.getModelMatrix() * glm::vec4(center, 1.f) };
 
     // Scaled orientation
     const glm::vec3 right = transform.getRight() * extents.x;
     const glm::vec3 up = transform.getUp() * extents.y;
     const glm::vec3 forward = transform.getForward() * extents.z;
 
     const float newIi = std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, right)) +
         std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, up)) +
         std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, forward));
 
     const float newIj = std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, right)) +
         std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, up)) +
         std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, forward));
 
     const float newIk = std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, right)) +
         std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, up)) +
         std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, forward));
 
     const AABB globalAABB(globalCenter, newIi, newIj, newIk);
 
     return (globalAABB.isOnOrForwardPlane(camFrustum.leftFace) &&
             globalAABB.isOnOrForwardPlane(camFrustum.rightFace) &&
             globalAABB.isOnOrForwardPlane(camFrustum.topFace) &&
             globalAABB.isOnOrForwardPlane(camFrustum.bottomFace) &&
             globalAABB.isOnOrForwardPlane(camFrustum.nearFace) &&
             globalAABB.isOnOrForwardPlane(camFrustum.farFace));
 }
 
 //-----------------------------------------------------------------------------
 // 辅助函数实现
 //-----------------------------------------------------------------------------
 
 Frustum createFrustumFromCamera(const Camera& cam, float aspect, float fovY, float zNear, float zFar)
 {
     Frustum     frustum;
     const float halfVSide = zFar * tanf(fovY * 0.5f);
     const float halfHSide = halfVSide * aspect;
     const glm::vec3 frontMultFar = zFar * cam.Front;
 
     frustum.nearFace = { cam.Position + zNear * cam.Front, cam.Front };
     frustum.farFace = { cam.Position + frontMultFar, -cam.Front };
     frustum.rightFace = { cam.Position, glm::cross(frontMultFar - cam.Right * halfHSide, cam.Up) };
     frustum.leftFace = { cam.Position, glm::cross(cam.Up, frontMultFar + cam.Right * halfHSide) };
     frustum.topFace = { cam.Position, glm::cross(cam.Right, frontMultFar - cam.Up * halfVSide) };
     frustum.bottomFace = { cam.Position, glm::cross(frontMultFar + cam.Up * halfVSide, cam.Right) };
     return frustum;
 }
 
 AABB generateAABB(const Model& model)
 {
     glm::vec3 minAABB = glm::vec3(std::numeric_limits<float>::max());
     glm::vec3 maxAABB = glm::vec3(std::numeric_limits<float>::min());
     for (auto&& mesh : model.meshes)
     {
         for (auto&& vertex : mesh.vertices)
         {
             minAABB.x = std::min(minAABB.x, vertex.Position.x);
             minAABB.y = std::min(minAABB.y, vertex.Position.y);
             minAABB.z = std::min(minAABB.z, vertex.Position.z);
 
             maxAABB.x = std::max(maxAABB.x, vertex.Position.x);
             maxAABB.y = std::max(maxAABB.y, vertex.Position.y);
             maxAABB.z = std::max(maxAABB.z, vertex.Position.z);
         }
     }
     return AABB(minAABB, maxAABB);
 }
 
 Sphere generateSphereBV(const Model& model)
 {
     glm::vec3 minAABB = glm::vec3(std::numeric_limits<float>::max());
     glm::vec3 maxAABB = glm::vec3(std::numeric_limits<float>::min());
     for (auto&& mesh : model.meshes)
     {
         for (auto&& vertex : mesh.vertices)
         {
             minAABB.x = std::min(minAABB.x, vertex.Position.x);
             minAABB.y = std::min(minAABB.y, vertex.Position.y);
             minAABB.z = std::min(minAABB.z, vertex.Position.z);
 
             maxAABB.x = std::max(maxAABB.x, vertex.Position.x);
             maxAABB.y = std::max(maxAABB.y, vertex.Position.y);
             maxAABB.z = std::max(maxAABB.z, vertex.Position.z);
         }
     }
 
     return Sphere((maxAABB + minAABB) * 0.5f, glm::length(minAABB - maxAABB));
 }
 
 } // namespace FrustumMath