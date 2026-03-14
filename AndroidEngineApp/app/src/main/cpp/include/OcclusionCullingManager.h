#ifndef OCCLUSION_CULLING_MANAGER_H
#define OCCLUSION_CULLING_MANAGER_H

#include "entity.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <unordered_map>

// 前向声明，避免循环依赖
namespace Engine {
    class Engine;  // 前向声明
}

class Camera;  // 前向声明

namespace Engine {

class OcclusionCullingManager {
public:
    OcclusionCullingManager(class Engine* engine);  // 使用class关键字
    ~OcclusionCullingManager();

    bool Initialize();
    
    void Update(const FrustumMath::Frustum& frustum, const glm::mat4& viewProj);

    void RegisterEntity(std::shared_ptr<Entity> entity);
    void UnregisterEntity(int entityId);
    void ClearEntities();

    void SetCullingDistance(float distance) { cullingDistance = distance; }
    void SetOcclusionThreshold(int threshold) { occlusionThreshold = threshold; }
    void EnableDebugDraw(bool enable) { debugDraw = enable; }

    const std::vector<int>& GetVisibleEntities() const { return visibleEntities; }
    int GetVisibleCount() const { return visibleCount; }
    int GetTotalCount() const { return totalCount; }
    float GetCullingTime() const { return cullingTime; }

private:
    class Engine* engine;  
    
    std::unordered_map<int, std::shared_ptr<Entity>> entities;
    std::vector<int> visibleEntities;
    
    float cullingDistance = 1000.0f;
    int occlusionThreshold = 100;
    bool debugDraw = false;
    
    int visibleCount = 0;
    int totalCount = 0;
    float cullingTime = 0.0f;

    bool FrustumCull(const FrustumMath::Frustum& frustum, 
                     const FrustumMath::AABB& bounds, 
                     const Transform& transform);
    
    bool DistanceCull(const glm::vec3& entityPos, const glm::vec3& cameraPos);
    
    bool SoftwareOcclusionCull(const FrustumMath::AABB& bounds, 
                               const Transform& transform,
                               const glm::mat4& viewProj);
    
    void UpdateStats();
};

} // namespace Engine

#endif