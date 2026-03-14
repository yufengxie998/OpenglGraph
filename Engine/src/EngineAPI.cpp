#include "EngineAPI.h"
#include <unordered_map>
#include "ModelObject.h"

#ifdef PLATFORM_ANDROID
#include <android_native_app_glue.h>
#endif

namespace EngineAPI
{
    using EngineCore = Engine::Engine;
    static EngineCore &engine = EngineCore::GetInstance();
    
    // 模型ID映射（用于快速查找ModelObject）
    static std::unordered_map<int, std::shared_ptr<Engine::ModelObject>> modelCache;
    
    // 辅助函数：获取ModelObject
    static std::shared_ptr<Engine::ModelObject> GetModelObject(int id)
    {
        auto entity = engine.GetEntity(id);
        if (!entity) return nullptr;
        return std::dynamic_pointer_cast<Engine::ModelObject>(entity);
    }

    bool Init(int width, int height, const char *title)
    {
        Engine::EngineConfig config;
        config.width = width;
        config.height = height;
        config.title = title;
        config.enableLineRenderer = true;
        return engine.Initialize(config);
    }
    
    bool InitWithConfig(const Engine::EngineConfig &config)
    {
        Engine::EngineConfig &engineConfig = const_cast<Engine::EngineConfig &>(config);
        return engine.Initialize(engineConfig);
    }

#ifdef PLATFORM_ANDROID
    void EnableTestTriangle(bool enable)
    {
        engine.EnableTestTriangle(enable);
    }
#endif

    bool LoadSkybox(const std::vector<std::string> &faces)
    {
        return engine.LoadSkybox(faces);
    }

    void EnableSkybox(bool enable)
    {
        engine.SetSkyboxEnabled(enable);
    }

    bool IsSkyboxEnabled()
    {
        return engine.IsSkyboxEnabled();
    }
    
    void Shutdown()
    {
        modelCache.clear();
        engine.Shutdown();
    }

    void Run()
    {
        engine.Run();
    }

    void Stop()
    {
        engine.Stop();
    }

    void DrawFrame(float deltaTime)
    {
        engine.RenderFrame(deltaTime);
    }

    int LoadModel(const char *path)
    {
        auto model = std::make_shared<Engine::ModelObject>(path);
        int id = engine.AddEntity(model);
        if (id >= 0) {
            modelCache[id] = model;
        }
        return id;
    }
    
    void GetCameraPosition(float *pos)
    {
        auto &camera = engine.GetCamera();
        auto camPos = camera.Position;
        pos[0] = camPos.x;
        pos[1] = camPos.y;
        pos[2] = camPos.z;
    }
    
    bool SetModelTexture(int modelId, const char *texturePath)
    {
        auto model = GetModelObject(modelId);
        if (!model) return false;
        
        unsigned int textureID = engine.LoadTexture(texturePath);
        if (textureID == 0) return false;
        
        model->SetTexture(textureID, texturePath);
        return true;
    }

    void SetCameraPosition(float x, float y, float z)
    {
        engine.SetCameraPosition(glm::vec3(x, y, z));
    }

    void SetCameraRotation(float yaw, float pitch)
    {
        auto &camera = engine.GetCamera();
        
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        camera.Front = glm::normalize(front);
        
        camera.Right = glm::normalize(glm::cross(camera.Front, camera.WorldUp));
        camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));
    }

    void SetCameraZoom(float zoom)
    {
        engine.SetCameraZoom(zoom);
    }

    void SetUpdateCallback(void (*callback)(float))
    {
        if (callback)
        {
            engine.SetUpdateCallback([callback](float dt)
                                     { callback(dt); });
        }
    }

    void SetRenderCallback(void (*callback)())
    {
        if (callback)
        {
            engine.SetRenderCallback([callback]()
                                     { callback(); });
        }
    }

    // ============ 线条绘制API ============

    void DrawLine(float x1, float y1, float z1,
                  float x2, float y2, float z2,
                  float r, float g, float b,
                  float lifetime)
    {
        auto *renderer = engine.GetLineRenderer();
        if (renderer && engine.IsLineRendererAvailable())
        {
            renderer->AddLine(
                glm::vec3(x1, y1, z1),
                glm::vec3(x2, y2, z2),
                glm::vec3(r, g, b),
                lifetime);
        }
    }

    void DrawLine(const glm::vec3 &start, const glm::vec3 &end,
                  const glm::vec3 &color, float lifetime)
    {
        DrawLine(start.x, start.y, start.z,
                 end.x, end.y, end.z,
                 color.x, color.y, color.z,
                 lifetime);
    }

    void DrawCube(const glm::vec3 &center, float size,
                  const glm::vec3 &color, float lifetime)
    {
        float half = size * 0.5f;
        glm::vec3 corners[8] = {
            center + glm::vec3(-half, -half, -half),
            center + glm::vec3( half, -half, -half),
            center + glm::vec3(-half,  half, -half),
            center + glm::vec3( half,  half, -half),
            center + glm::vec3(-half, -half,  half),
            center + glm::vec3( half, -half,  half),
            center + glm::vec3(-half,  half,  half),
            center + glm::vec3( half,  half,  half)
        };

        // 底面
        DrawLine(corners[0], corners[1], color, lifetime);
        DrawLine(corners[1], corners[3], color, lifetime);
        DrawLine(corners[3], corners[2], color, lifetime);
        DrawLine(corners[2], corners[0], color, lifetime);

        // 顶面
        DrawLine(corners[4], corners[5], color, lifetime);
        DrawLine(corners[5], corners[7], color, lifetime);
        DrawLine(corners[7], corners[6], color, lifetime);
        DrawLine(corners[6], corners[4], color, lifetime);

        // 垂直线
        DrawLine(corners[0], corners[4], color, lifetime);
        DrawLine(corners[1], corners[5], color, lifetime);
        DrawLine(corners[2], corners[6], color, lifetime);
        DrawLine(corners[3], corners[7], color, lifetime);
    }

    void DrawGrid(float size, int divisions, float y,
                  float r, float g, float b, float lifetime)
    {
        float half = size * 0.5f;
        float step = size / divisions;
        glm::vec3 color(r, g, b);

        for (int i = 0; i <= divisions; i++)
        {
            float pos = -half + i * step;
            DrawLine(glm::vec3(pos, y, -half),
                     glm::vec3(pos, y, half),
                     color, lifetime);
            DrawLine(glm::vec3(-half, y, pos),
                     glm::vec3(half, y, pos),
                     color, lifetime);
        }
    }

    void DrawAxis(float length, float lifetime)
    {
        DrawLine(glm::vec3(0.0f), glm::vec3(length, 0.0f, 0.0f),
                 glm::vec3(1.0f, 0.0f, 0.0f), lifetime);
        DrawLine(glm::vec3(0.0f), glm::vec3(0.0f, length, 0.0f),
                 glm::vec3(0.0f, 1.0f, 0.0f), lifetime);
        DrawLine(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, length),
                 glm::vec3(0.0f, 0.0f, 1.0f), lifetime);
    }

    // ============ Primitive 颜色设置 ============

void SetPrimitiveColor(int id, float r, float g, float b)
{
    auto entity = engine.GetEntity(id);
    if (!entity) {
        LOGE("EngineAPI", "Entity %d not found for SetPrimitiveColor", id);
        return;
    }
    
    // Primitive 需要有一个 SetColor 方法
    auto primitive = std::dynamic_pointer_cast<Primitive>(entity);
    if (primitive) {
        primitive->SetColor(glm::vec3(r, g, b));
    } else {
        LOGE("EngineAPI", "Entity %d is not a Primitive", id);
    }
}

// ============ 线条绘制函数 ============

void DrawDashedLine(const glm::vec3 &start, const glm::vec3 &end,
                    const glm::vec3 &color, float dashLength,
                    float gapLength, float lifetime)
{
    auto *renderer = engine.GetLineRenderer();
    if (renderer && engine.IsLineRendererAvailable())
    {
        renderer->AddDashedLine(start, end, color, dashLength, gapLength, lifetime);
    }
}
// ============ 高级线条绘制函数 ============

void DrawStyledLine(const glm::vec3 &start, const glm::vec3 &end,
    const glm::vec3 &color, LineStyle style,
    float dashLength, float gapLength,
    float lifetime)
{
auto *renderer = engine.GetLineRenderer();
if (renderer && engine.IsLineRendererAvailable())
{
renderer->AddStyledLine(start, end, color, style, dashLength, gapLength, lifetime);
}
}

void SetDefaultDashParams(float dashLength, float gapLength)
{
auto *renderer = engine.GetLineRenderer();
if (renderer)
{
renderer->SetDefaultDashParams(dashLength, gapLength);
}
}

void DrawBezierCurve(const glm::vec3 &p0, const glm::vec3 &p1,
     const glm::vec3 &p2, const glm::vec3 &p3,
     const glm::vec3 &color, LineStyle style,
     int segments, float lifetime)
{
auto *renderer = engine.GetLineRenderer();
if (renderer && engine.IsLineRendererAvailable())
{
renderer->AddBezierCurve(p0, p1, p2, p3, color, style, segments, lifetime);
}
}

void DrawCircle(const glm::vec3 &center, float radius, const glm::vec3 &color,
LineStyle style, int segments, float lifetime)
{
auto *renderer = engine.GetLineRenderer();
if (renderer && engine.IsLineRendererAvailable())
{
renderer->AddCircle(center, radius, color, style, segments, lifetime);
}
}

void DrawSplineCurve(const std::vector<glm::vec3> &points,
     const glm::vec3 &color, int segmentsPerSegment,
     float lifetime)
{
auto *renderer = engine.GetLineRenderer();
if (renderer && engine.IsLineRendererAvailable())
{
renderer->AddSplineCurve(points, color, segmentsPerSegment, lifetime);
}
}

void DrawSineWave(const glm::vec3 &start, const glm::vec3 &direction,
  float amplitude, float frequency, float length,
  const glm::vec3 &color, int segments, float lifetime)
{
auto *renderer = engine.GetLineRenderer();
if (renderer && engine.IsLineRendererAvailable())
{
renderer->AddSineWave(start, direction, amplitude, frequency,
              length, color, segments, lifetime);
}
}

// ============ 多项式曲线绘制 ============

void DrawPolynomialCurveXY(float c0, float c1, float c2, float c3,
                           float xMin, float xMax,
                           const glm::vec3 &color,
                           int segments, float lifetime)
{
    auto *renderer = engine.GetLineRenderer();
    if (!renderer || !engine.IsLineRendererAvailable()) return;
    
    std::vector<glm::vec3> points;
    float step = (xMax - xMin) / segments;
    
    for (int i = 0; i <= segments; i++) {
        float x = xMin + i * step;
        float y = c0 + c1*x + c2*x*x + c3*x*x*x; // 多项式计算
        points.push_back(glm::vec3(x, y, 0.0f));
    }
    
    // 将点连接成线
    for (size_t i = 0; i < points.size() - 1; i++) {
        renderer->AddLine(points[i], points[i + 1], color, lifetime);
    }
}

// ============ 如果需要，也可以添加其他曲线函数 ============

void DrawPolynomialCurveXZ(float c0, float c1, float c2, float c3,
                           float xMin, float xMax,
                           const glm::vec3 &color,
                           int segments, float lifetime)
{
    auto *renderer = engine.GetLineRenderer();
    if (!renderer || !engine.IsLineRendererAvailable()) return;
    
    std::vector<glm::vec3> points;
    float step = (xMax - xMin) / segments;
    
    for (int i = 0; i <= segments; i++) {
        float x = xMin + i * step;
        float z = c0 + c1*x + c2*x*x + c3*x*x*x; // 在XZ平面上，Y=0
        points.push_back(glm::vec3(x, 0.0f, z));
    }
    
    for (size_t i = 0; i < points.size() - 1; i++) {
        renderer->AddLine(points[i], points[i + 1], color, lifetime);
    }
}

void DrawPolynomialCurve(const glm::vec3 &startPoint,
                         const glm::vec3 &direction,
                         const glm::vec3 &upDirection,
                         float c0, float c1, float c2, float c3,
                         float xMin, float xMax,
                         const glm::vec3 &color,
                         int segments, float lifetime)
{
    auto *renderer = engine.GetLineRenderer();
    if (!renderer || !engine.IsLineRendererAvailable()) return;
    
    glm::vec3 normDir = glm::normalize(direction);
    glm::vec3 normUp = glm::normalize(upDirection);
    glm::vec3 normRight = glm::normalize(glm::cross(normDir, normUp));
    
    std::vector<glm::vec3> points;
    float step = (xMax - xMin) / segments;
    
    for (int i = 0; i <= segments; i++) {
        float x = xMin + i * step;
        float y = c0 + c1*x + c2*x*x + c3*x*x*x;
        
        glm::vec3 point = startPoint + normDir * x + normUp * y;
        points.push_back(point);
    }
    
    for (size_t i = 0; i < points.size() - 1; i++) {
        renderer->AddLine(points[i], points[i + 1], color, lifetime);
    }
}

void DrawPolynomialCurveLocal(const glm::vec3 &center,
                              const glm::vec3 &xAxis,
                              const glm::vec3 &yAxis,
                              const glm::vec3 &zAxis,
                              float c0, float c1, float c2, float c3,
                              float xMin, float xMax,
                              const glm::vec3 &color,
                              int segments, float lifetime)
{
    auto *renderer = engine.GetLineRenderer();
    if (!renderer || !engine.IsLineRendererAvailable()) return;
    
    glm::vec3 normX = glm::normalize(xAxis);
    glm::vec3 normY = glm::normalize(yAxis);
    
    std::vector<glm::vec3> points;
    float step = (xMax - xMin) / segments;
    
    for (int i = 0; i <= segments; i++) {
        float x = xMin + i * step;
        float y = c0 + c1*x + c2*x*x + c3*x*x*x;
        
        glm::vec3 point = center + normX * x + normY * y;
        points.push_back(point);
    }
    
    for (size_t i = 0; i < points.size() - 1; i++) {
        renderer->AddLine(points[i], points[i + 1], color, lifetime);
    }
}

    void ClearLines()
    {
        auto *renderer = engine.GetLineRenderer();
        if (renderer)
        {
            renderer->Clear();
        }
    }

    void SetLineWidth(float width)
    {
        auto *renderer = engine.GetLineRenderer();
        if (renderer)
        {
            renderer->SetLineWidth(width);
        }
    }

    // ============ 光源API ============

    int AddAmbientLight(const glm::vec3 &color, float intensity)
    {
        Light light;
        light.type = LightType::AMBIENT;
        light.color = color;
        light.intensity = intensity;
        return engine.AddLight(light);
    }

    int AddPointLight(const glm::vec3 &position, const glm::vec3 &color,
                      float intensity, float constant, float linear, float quadratic)
    {
        Light light;
        light.type = LightType::POINT;
        light.position = position;
        light.color = color;
        light.intensity = intensity;
        light.constant = constant;
        light.linear = linear;
        light.quadratic = quadratic;
        return engine.AddLight(light);
    }

    int AddSpotLight(const glm::vec3 &position, const glm::vec3 &direction,
                     const glm::vec3 &color, float intensity,
                     float cutOff, float outerCutOff,
                     float constant, float linear, float quadratic)
    {
        Light light;
        light.type = LightType::SPOT;
        light.position = position;
        light.direction = glm::normalize(direction);
        light.color = color;
        light.intensity = intensity;
        light.cutOff = glm::radians(cutOff);
        light.outerCutOff = glm::radians(outerCutOff);
        light.constant = constant;
        light.linear = linear;
        light.quadratic = quadratic;
        return engine.AddLight(light);
    }

    void RemoveLight(int index)
    {
        engine.RemoveLight(index);
    }

    void ClearLights()
    {
        engine.ClearLights();
    }

    int GetLightCount()
    {
        return engine.GetLights().size();
    }

    // ============ 基本体绘制API ============

    int AddPlane(float width, float depth, int segments)
    {
        auto plane = std::make_shared<Geometry::Plane>(width, depth, segments);
        return engine.AddEntity(plane);
    }

    int AddCylinder(float radius, float height, int sectors)
    {
        auto cylinder = std::make_shared<Geometry::Cylinder>(radius, height, sectors);
        return engine.AddEntity(cylinder);
    }

    // ============ 动画控制API ============

    void LoadModelAnimation(int modelId, const char *animationPath)
    {
        auto model = GetModelObject(modelId);
        if (model)
        {
            LOGI("EngineAPI", "Loading animation for model %d from: %s", modelId, animationPath);
            model->LoadAnimation(animationPath);
        }
        else
        {
            LOGE("EngineAPI", "Model %d not found, cannot load animation", modelId);
        }
    }

    void PlayModelAnimation(int modelId)
    {
        auto model = GetModelObject(modelId);
        if (model)
        {
            if (model->HasAnimation())
            {
                model->PlayAnimation();
                LOGI("EngineAPI", "Animation started for model %d", modelId);
            }
            else
            {
                LOGW("EngineAPI", "Model %d has no animation loaded", modelId);
            }
        }
    }

    void StopModelAnimation(int modelId)
    {
        auto model = GetModelObject(modelId);
        if (model)
        {
            model->StopAnimation();
            LOGI("EngineAPI", "Animation stopped for model %d", modelId);
        }
    }

    void SetModelAnimationSpeed(int modelId, float speed)
    {
        auto model = GetModelObject(modelId);
        if (model)
        {
            model->SetAnimationSpeed(speed);
            LOGI("EngineAPI", "Animation speed set to %.2f for model %d", speed, modelId);
        }
    }

    bool IsModelAnimationPlaying(int modelId)
    {
        auto model = GetModelObject(modelId);
        return model ? model->IsAnimationPlaying() : false;
    }

    // ============ Entity管理API ============

    void SetEntityParent(int entityId, int parentId)
{
    auto entity = engine.GetEntity(entityId);
    auto parent = engine.GetEntity(parentId);

    if (!entity || !parent) {
        LOGE("EngineAPI", "Entity %d or parent %d not found", entityId, parentId);
        return;
    }

    // 从原父节点移除
    if (auto currentParent = entity->getParent()) {
        currentParent->removeChild(entityId);
    }
    
    // 直接添加，使用基类指针版本
    // 需要在 Entity 类中添加一个接受 shared_ptr<Entity> 的 addChild 方法
    parent->addChild(entity);
    
    LOGI("EngineAPI", "Set entity %d parent to %d", entityId, parentId);
}

    void SetEntityPosition(int entityId, float x, float y, float z)
    {
        auto entity = engine.GetEntity(entityId);
        if (entity)
        {
            entity->setLocalPosition(glm::vec3(x, y, z));
        }
    }

    void SetEntityRotation(int entityId, float x, float y, float z)
    {
        auto entity = engine.GetEntity(entityId);
        if (entity)
        {
            entity->setLocalRotation(glm::vec3(x, y, z));
        }
    }

    void SetEntityRotationAxis(int entityId, float angle, float axisX, float axisY, float axisZ)
    {
        auto entity = engine.GetEntity(entityId);
        if (!entity)
        {
            LOGE("EngineAPI", "Entity %d not found", entityId);
            return;
        }
        entity->setLocalRotationAxis(angle, glm::vec3(axisX, axisY, axisZ));
    }
    
    void SetEntityScale(int entityId, float x, float y, float z)
    {
        auto entity = engine.GetEntity(entityId);
        if (entity)
        {
            entity->setLocalScale(glm::vec3(x, y, z));
        }
    }

    void RemoveEntity(int entityId)
    {
        // 从缓存中移除如果是ModelObject
        modelCache.erase(entityId);
        engine.RemoveEntity(entityId);
    }

    glm::vec3 GetEntityPosition(int entityId)
    {
        auto entity = engine.GetEntity(entityId);
        if (!entity)
        {
            LOGE("EngineAPI", "Entity %d not found", entityId);
            return glm::vec3(0.0f);
        }
        return entity->getLocalPosition();
    }

    glm::vec3 GetEntityRotation(int entityId)
    {
        auto entity = engine.GetEntity(entityId);
        if (!entity)
        {
            LOGE("EngineAPI", "Entity %d not found", entityId);
            return glm::vec3(0.0f);
        }
        return entity->getLocalRotation();
    }

    glm::vec3 GetEntityScale(int entityId)
    {
        auto entity = engine.GetEntity(entityId);
        if (!entity)
        {
            LOGE("EngineAPI", "Entity %d not found", entityId);
            return glm::vec3(1.0f);
        }
        return entity->getLocalScale();
    }

#ifdef PLATFORM_ANDROID
    void HandleTouchEvent(int action, float x, float y)
    {
        engine.HandleTouchEvent(action, x, y);
    }

    void HandlePinchEvent(float scaleFactor)
    {
        engine.HandlePinchEvent(scaleFactor);
    }

    void HandleKeyEvent(int keyCode, int action)
    {
        engine.HandleKeyEvent(keyCode, action);
    }

    void SetTouchSensitivity(float sensitivity)
    {
        engine.SetTouchSensitivity(sensitivity);
    }

    void HandleRotateEvent(float angle)
    {
        engine.HandleRotateEvent(angle);
    }
#endif

} // namespace EngineAPI