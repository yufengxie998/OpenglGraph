#ifndef _H_ENGINE_H
#define _H_ENGINE_H

// 平台检测
#ifdef PLATFORM_ANDROID
#include <GLES3/gl3.h>
#include <android_native_app_glue.h>
#else
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif

#include <glm/glm.hpp>
#include <memory>
#include <functional>
#include <vector>
#include <string>

#include "camera.h"
#include "ModelObject.h"
#include "Skybox.h"
#include "Primitive.h"
#include "Plane.h"
#include "Cylinder.h"
#include "OcclusionCullingManager.h"

// 前向声明
class LineRenderer;

// 基础光源结构
struct Light
{
    LightType type;
    glm::vec3 position;  // 位置（点光源/聚光灯）
    glm::vec3 direction; // 方向（聚光灯）
    glm::vec3 color;     // 颜色
    float intensity;     // 强度

    // 点光源衰减参数
    float constant;  // 常数衰减
    float linear;    // 线性衰减
    float quadratic; // 二次衰减

    // 聚光灯参数
    float cutOff;      // 内切光角（弧度）
    float outerCutOff; // 外切光角（弧度）

    Light() : type(LightType::POINT), position(0.0f), direction(0.0f, -1.0f, 0.0f),
              color(1.0f), intensity(1.0f), constant(1.0f), linear(0.09f),
              quadratic(0.032f), cutOff(glm::radians(12.5f)),
              outerCutOff(glm::radians(17.5f)) {}
};

namespace Engine
{

    // 引擎配置结构
    struct EngineConfig
    {
        int width = 800;
        int height = 600;
        std::string title = "Engine";
        bool enableVSync = true;
        bool enableDepthTest = true;
        bool enableWireframe = false;
        bool enableMouseCapture = true;
        bool enableLineRenderer = true;
        float clearColor[4] = {0.05f, 0.05f, 0.05f, 1.0f};
        glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f);

#ifdef PLATFORM_ANDROID
        float touchSensitivity = 0.1f;
        std::string internalStoragePath;
#endif
    };

    // 引擎主类
    class Engine
    {
    public:
        Engine();
        ~Engine();

        // 初始化与关闭
        bool Initialize(const EngineConfig &config = EngineConfig());
        void Shutdown();

        // 主循环控制
        void Run();
        void Stop();
        void RenderFrame(float deltaTime);

        // 事件回调设置
        void SetUpdateCallback(std::function<void(float)> callback);
        void SetRenderCallback(std::function<void()> callback);

        // 摄像机控制
        Camera &GetCamera() { return *camera; }
        void SetCameraPosition(const glm::vec3 &pos);
        void SetCameraZoom(float zoom);

        // 天空盒相关
        bool LoadSkybox(const std::vector<std::string> &faces);
        void SetSkyboxEnabled(bool enable) { skyboxEnabled = enable; }
        bool IsSkyboxEnabled() const { return skyboxEnabled; }

        // 资源加载
        unsigned int LoadTexture(const std::string &path);
        std::shared_ptr<ModelObject> LoadModel(const std::string &path);

        void SetShaderType(ShaderType type) { currentShaderType = type; }
        ShaderType GetShaderType() const { return currentShaderType; }

        // 线条渲染器
        bool InitLineRenderer();
        LineRenderer *GetLineRenderer() const { return lineRenderer.get(); }
        bool IsLineRendererAvailable() const { return lineRenderer != nullptr && lineRendererInitialized; }
    // 遮挡剔除相关
    OcclusionCullingManager* GetOcclusionCullingManager() { return occlusionCulling.get(); }
    void EnableOcclusionCulling(bool enable) { occlusionCullingEnabled = enable; }
    bool IsOcclusionCullingEnabled() const { return occlusionCullingEnabled; }
        // 获取窗口/配置
#ifdef PLATFORM_ANDROID
        // Android特定接口
        void HandleTouchEvent(int action, float x, float y);
        void HandlePinchEvent(float scaleFactor);
        void HandleRotateEvent(float angle);
        void HandleKeyEvent(int keyCode, int action);
        void SetTouchSensitivity(float sensitivity);

        // 测试三角形渲染（临时）
        void RenderTestTriangle();
        bool initTestTriangle();
        void EnableTestTriangle(bool enable) { testTriangleEnabled = enable; }
#else
        GLFWwindow *GetWindow() const { return window; }
#endif

        const EngineConfig &GetConfig() const { return config; }

        // 静态单例访问
        static Engine &GetInstance();

        // 光源管理API
        int AddLight(const Light &light);
        void RemoveLight(int index);
        void ClearLights();
        Light &GetLight(int index);
        void UpdateLight(int index, const Light &light);
        void SetMaxLights(int max);

        // 获取光源数据供着色器使用
        const std::vector<Light> &GetLights() const { return lights; }

        // Entity 管理
        int AddEntity(std::shared_ptr<Entity> entity);
        void RemoveEntity(int id);
        void ClearEntities();
        std::shared_ptr<Entity> GetEntity(int id);

    private:
        // 初始化相关
        bool InitShaders();
        void RenderLines();

#ifdef PLATFORM_ANDROID
        // Android特定成员
        bool testTriangleEnabled = false;
        float testTriangleTime = 0.0f;
        GLuint testVAO = 0;
        GLuint testVBO = 0;
        GLuint testShaderProgram = 0;
        GLint testMVPLoc = -1;
#else
        // 桌面平台回调函数
        static void FramebufferSizeCallback(GLFWwindow *window, int width, int height);
        static void MouseCallback(GLFWwindow *window, double xposIn, double yposIn);
        static void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset);
        static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

        GLFWwindow *window = nullptr;
#endif

        // 核心成员变量
        std::unique_ptr<Camera> camera;
        std::unique_ptr<Shader> shader;
        std::unique_ptr<Shader> shaderSimple;
        std::unique_ptr<Shader> shaderPBR;
        std::unique_ptr<Skybox> skybox;
        std::unique_ptr<LineRenderer> lineRenderer;

        std::function<void(float)> updateCallback;
        std::function<void()> renderCallback;

        std::unique_ptr<OcclusionCullingManager> occlusionCulling;
        bool occlusionCullingEnabled = false;

        EngineConfig config;
        ShaderType currentShaderType = ShaderType::SHADER_PBR;

        bool isRunning = false;
        bool skyboxEnabled = false;
        bool lineRendererInitialized = false;

        // 静态成员（用于回调）
        static Engine *s_instance;
        static float lastX, lastY;
        static bool firstMouse;
        static float deltaTime, lastFrame;
        std::vector<Light> lights;
        int maxLights = 16; // 最大光源数

        std::unordered_map<int, std::shared_ptr<Entity>> entities;
        int nextEntityId = 0;

        // 模板方法，方便特定类型的添加
        template <typename T, typename... Args>
        std::shared_ptr<T> CreateEntity(Args &&...args)
        {
            auto entity = std::make_shared<T>(std::forward<Args>(args)...);
            int id = AddEntity(entity);
            return entity;
        }

        // 查询特定类型的实体
        template <typename T>
        std::vector<std::shared_ptr<T>> GetEntitiesByType()
        {
            std::vector<std::shared_ptr<T>> result;
            for (auto &pair : entities)
            {
                auto casted = std::dynamic_pointer_cast<T>(pair.second);
                if (casted)
                {
                    result.push_back(casted);
                }
            }
            return result;
        }
    };

} // namespace Engine

#endif // ENGINE_H