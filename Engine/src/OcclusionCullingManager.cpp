#include "OcclusionCullingManager.h"
#include "Engine.h" 
#include <extend/logger.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

#ifdef PLATFORM_ANDROID
#include <chrono>
#else
#include <GLFW/glfw3.h>
#endif

namespace Engine {

    static double GetCurrentTime()
    {
#ifdef PLATFORM_ANDROID
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration<double>(duration).count();
#else
        return glfwGetTime();
#endif
    }

OcclusionCullingManager::OcclusionCullingManager(class Engine* engine) 
    : engine(engine) {
    LOGI("OcclusionCulling", "Manager created");
}

OcclusionCullingManager::~OcclusionCullingManager() {
    ClearEntities();
    LOGI("OcclusionCulling", "Manager destroyed");
}

bool OcclusionCullingManager::Initialize() {
    LOGI("OcclusionCulling", "Initialized with distance: %.1f, threshold: %d", 
         cullingDistance, occlusionThreshold);
    return true;
}

void OcclusionCullingManager::RegisterEntity(std::shared_ptr<Entity> entity) {
    if (entity && entity->boundingVolume) {
        int id = entity->GetId();
        entities[id] = entity;
        LOGI("OcclusionCulling", "Registered entity with ID: %d", id);
    }
}

void OcclusionCullingManager::UnregisterEntity(int entityId) {
    auto it = entities.find(entityId);
    if (it != entities.end()) {
        entities.erase(it);
        LOGI("OcclusionCulling", "Unregistered entity %d", entityId);
    }
}

void OcclusionCullingManager::ClearEntities() {
    entities.clear();
    visibleEntities.clear();
    LOGI("OcclusionCulling", "All entities cleared");
}

void OcclusionCullingManager::Update(const FrustumMath::Frustum& frustum, 
                                     const glm::mat4& viewProj) {
    auto startTime = GetCurrentTime();
    
    visibleEntities.clear();
    
    totalCount = entities.size();
    
    Camera& camera = engine->GetCamera();  // 使用引用，而不是指针
    glm::vec3 cameraPos = camera.Position;
    
    for (auto& pair : entities) {
        auto& entity = pair.second;
        if (!entity || !entity->boundingVolume) continue;
        
        bool visible = true;
        
        // 步骤1: 视锥体剔除 - 使用AABB的isOnFrustum方法
        if (!entity->boundingVolume->isOnFrustum(frustum, entity->transform)) {
            visible = false;
        }
        
        // 步骤2: 距离剔除
        if (visible) {
            glm::vec3 entityPos = entity->transform.getGlobalPosition();
            if (!DistanceCull(entityPos, cameraPos)) {
                visible = false;
            }
        }
        
        // 步骤3: 软件遮挡剔除（可选）
        if (visible && occlusionThreshold > 0) {
            // 尝试将BoundingVolume转换为AABB
            const FrustumMath::AABB* aabb = 
                dynamic_cast<const FrustumMath::AABB*>(entity->boundingVolume.get());
            if (aabb && SoftwareOcclusionCull(*aabb, entity->transform, viewProj)) {
                visible = false;
            }
        }
        
        if (visible) {
            visibleEntities.push_back(pair.first);
        }
    }
    
    visibleCount = visibleEntities.size();
    cullingTime = (GetCurrentTime() - startTime) * 1000.0f;
    
    UpdateStats();
}

bool OcclusionCullingManager::FrustumCull(const FrustumMath::Frustum& frustum, 
                                         const FrustumMath::AABB& bounds, 
                                         const Transform& transform) {
    return bounds.isOnFrustum(frustum, transform);
}

bool OcclusionCullingManager::DistanceCull(const glm::vec3& entityPos, 
                                          const glm::vec3& cameraPos) {
    float distance = glm::distance(entityPos, cameraPos);
    return distance <= cullingDistance;
}

bool OcclusionCullingManager::SoftwareOcclusionCull(const FrustumMath::AABB& bounds, 
                                                   const Transform& transform,
                                                   const glm::mat4& viewProj) {
    auto vertices = bounds.getVertice();
    
    glm::vec2 minScreen(1.0f, 1.0f);
    glm::vec2 maxScreen(0.0f, 0.0f);
    
    glm::mat4 modelMatrix = transform.getModelMatrix();
    
    for (const auto& vertex : vertices) {
        glm::vec4 worldPos = modelMatrix * glm::vec4(vertex, 1.0f);
        glm::vec4 clipPos = viewProj * worldPos;
        
        if (clipPos.w <= 0.0f) continue;
        
        glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
        glm::vec2 screenPos = (glm::vec2(ndc.x, ndc.y) + 1.0f) * 0.5f;
        
        minScreen = glm::min(minScreen, screenPos);
        maxScreen = glm::max(maxScreen, screenPos);
    }
    
    float screenWidth = (maxScreen.x - minScreen.x) * engine->GetConfig().width;
    float screenHeight = (maxScreen.y - minScreen.y) * engine->GetConfig().height;
    float screenArea = screenWidth * screenHeight;
    
    return screenArea < occlusionThreshold;
}

void OcclusionCullingManager::UpdateStats() {
    static int frameCount = 0;
    frameCount++;
    
    if (frameCount % 100 == 0) {
        float visiblePercent = totalCount > 0 ? 
            (float)visibleCount / totalCount * 100.0f : 0.0f;
        LOGI("OcclusionCulling", 
             "Stats - Total: %d, Visible: %d (%.1f%%), Time: %.2fms", 
             totalCount, visibleCount, visiblePercent, cullingTime);
        frameCount = 0;
    }
}

} // namespace Engine