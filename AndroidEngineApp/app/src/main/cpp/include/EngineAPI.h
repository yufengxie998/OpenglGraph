#ifndef _H_ENGINE_API_H
#define _H_ENGINE_API_H

#include "Engine.h"
#include <memory>
#include "LineRenderer.h"

// 对外暴露的简洁API
namespace EngineAPI
{

    // 初始化引擎
    bool Init(int width = 800, int height = 600, const char *title = "DFL Engine");
    bool InitWithConfig(const Engine::EngineConfig &config);
    // 天空盒API
    bool LoadSkybox(const std::vector<std::string> &faces);
    void EnableSkybox(bool enable);
    bool IsSkyboxEnabled();
    // 关闭引擎
    void Shutdown();

    // 开始主循环
    void Run();

    // 停止主循环
    void Stop();
    void DrawFrame(float deltaTime);
    // 加载模型
    int LoadModel(const char *path);
    void GetCameraPosition(float *pos); // pos 是长度为3的数组
    void SetCameraRotation(float yaw, float pitch);
    // 设置模型纹理
    bool SetModelTexture(int modelId, const char *texturePath);

    // 设置摄像机位置
    void SetCameraPosition(float x, float y, float z);

    // 设置摄像机缩放
    void SetCameraZoom(float zoom);

    // 设置更新回调
    void SetUpdateCallback(void (*callback)(float deltaTime));

    // 设置渲染回调
    void SetRenderCallback(void (*callback)());

    // ============ 测试方法 ============
    void EnableTestTriangle(bool enable); // 启用/禁用测试三角形
    // ============ 新增：线条绘制API ============

    // 基本线条绘制
    void DrawLine(float x1, float y1, float z1,
                  float x2, float y2, float z2,
                  float r = 1.0f, float g = 1.0f, float b = 1.0f,
                  float lifetime = -1.0f);

    void DrawLine(const glm::vec3 &start, const glm::vec3 &end,
                  const glm::vec3 &color = glm::vec3(1.0f),
                  float lifetime = -1.0f);

    // 批量线条绘制
    void DrawLines(const float *vertices, int vertexCount,
                   const float *colors = nullptr,
                   float lifetime = -1.0f);

    // 几何图形绘制

    void DrawCube(const glm::vec3 &center, float size,
                  const glm::vec3 &color = glm::vec3(1.0f),
                  float lifetime = -1.0f);

    void DrawGrid(float size, int divisions, float y = 0.0f,
                  float r = 0.3f, float g = 0.3f, float b = 0.3f,
                  float lifetime = -1.0f);

    void DrawAxis(float length = 1.0f, float lifetime = -1.0f);

    // 包围盒绘制
    // void DrawAABB(const glm::vec3 &min, const glm::vec3 &max,
    //               const glm::vec3 &color = glm::vec3(0.0f, 1.0f, 0.0f),
    //               float lifetime = -1.0f);

    // 绘制虚线
    void DrawDashedLine(const glm::vec3 &start, const glm::vec3 &end,
                        const glm::vec3 &color, float dashLength = 0.1f,
                        float gapLength = 0.05f, float lifetime = -1.0f);

    // 绘制带样式的线
    void DrawStyledLine(const glm::vec3 &start, const glm::vec3 &end,
                        const glm::vec3 &color, LineStyle style,
                        float dashLength = 0.1f, float gapLength = 0.05f,
                        float lifetime = -1.0f);

    // 设置默认虚线参数
    void SetDefaultDashParams(float dashLength, float gapLength);

    // 绘制贝塞尔曲线（带样式）
    void DrawBezierCurve(const glm::vec3 &p0, const glm::vec3 &p1,
                         const glm::vec3 &p2, const glm::vec3 &p3,
                         const glm::vec3 &color, LineStyle style = LineStyle::SOLID,
                         int segments = 20, float lifetime = -1.0f);

    // 绘制圆形（带样式）
    void DrawCircle(const glm::vec3 &center, float radius, const glm::vec3 &color,
                    LineStyle style = LineStyle::SOLID, int segments = 32,
                    float lifetime = -1.0f);

    // 添加样条曲线
    void DrawSplineCurve(const std::vector<glm::vec3> &points,
                         const glm::vec3 &color, int segmentsPerSegment = 10,
                         float lifetime = -1.0f);

    // 添加正弦曲线
    void DrawSineWave(const glm::vec3 &start, const glm::vec3 &direction,
                      float amplitude, float frequency, float length,
                      const glm::vec3 &color, int segments = 50,
                      float lifetime = -1.0f);

    // 添加多项式曲线：y = c0 + c1*x + c2/2*x^2 + c3/6*x^3
    void DrawPolynomialCurve(const glm::vec3 &startPoint,
                             const glm::vec3 &direction,
                             const glm::vec3 &upDirection,
                             float c0, float c1, float c2, float c3,
                             float xMin, float xMax,
                             const glm::vec3 &color,
                             int segments = 50,
                             float lifetime = -1.0f);

    // 添加多项式曲线（局部坐标系版本）
    void DrawPolynomialCurveLocal(const glm::vec3 &center,
                                  const glm::vec3 &xAxis,
                                  const glm::vec3 &yAxis,
                                  const glm::vec3 &zAxis,
                                  float c0, float c1, float c2, float c3,
                                  float xMin, float xMax,
                                  const glm::vec3 &color,
                                  int segments = 50,
                                  float lifetime = -1.0f);

    // 快捷版本：在XZ平面上绘制多项式曲线（Y轴向上）
    void DrawPolynomialCurveXZ(float c0, float c1, float c2, float c3,
                               float xMin, float xMax,
                               const glm::vec3 &color,
                               int segments = 50,
                               float lifetime = -1.0f);

    // 快捷版本：在XY平面上绘制多项式曲线（Z轴向前）
    void DrawPolynomialCurveXY(float c0, float c1, float c2, float c3,
                               float xMin, float xMax,
                               const glm::vec3 &color,
                               int segments = 50,
                               float lifetime = -1.0f);

    // 线条管理
    void ClearLines();
    void RemoveLine(int lineId); // 如果支持ID
    void SetLineWidth(float width);

        // 光源API
    int AddAmbientLight(const glm::vec3& color, float intensity = 1.0f);
    int AddPointLight(const glm::vec3& position, const glm::vec3& color, 
                      float intensity = 1.0f,
                      float constant = 1.0f, float linear = 0.09f, float quadratic = 0.032f);
    int AddSpotLight(const glm::vec3& position, const glm::vec3& direction,
                     const glm::vec3& color, float intensity = 1.0f,
                     float cutOff = 12.5f, float outerCutOff = 17.5f,
                     float constant = 1.0f, float linear = 0.09f, float quadratic = 0.032f);
    void RemoveLight(int index);
    void ClearLights();
    int GetLightCount();

        // 基本体绘制API
    int AddPlane(float width = 10.0f, float depth = 10.0f, int segments = 1);
    int AddCylinder(float radius = 1.0f, float height = 2.0f, int sectors = 32);
    void SetPrimitiveColor(int id, float r, float g, float b);
    void SetPrimitiveTexture(int id, const char* texturePath);

        // 动画控制
    void LoadModelAnimation(int modelId, const char* animationPath);
    void PlayModelAnimation(int modelId);
    void StopModelAnimation(int modelId);
    void SetModelAnimationSpeed(int modelId, float speed);
    bool IsModelAnimationPlaying(int modelId);

    // int CreateEntity(int modelId);  // 从已有的 modelId 创建 Entity
    void SetEntityParent(int entityId, int parentId);
    void SetEntityPosition(int entityId, float x, float y, float z);
    void SetEntityRotation(int entityId, float x, float y, float z);
    void SetEntityRotationAxis(int entityId, float angle, float axisX, float axisY, float axisZ);
    void SetEntityScale(int entityId, float x, float y, float z);
    void RemoveEntity(int entityId);

    glm::vec3 GetEntityPosition(int entityId);
    glm::vec3 GetEntityRotation(int entityId);
    glm::vec3 GetEntityScale(int entityId);

#ifdef PLATFORM_ANDROID
    void HandleTouchEvent(int action, float x, float y);
    void HandlePinchEvent(float scaleFactor);
    void HandleKeyEvent(int keyCode, int action);
    void SetTouchSensitivity(float sensitivity);
    void HandleRotateEvent(float angle); // 可选
#endif

} // namespace EngineAPI

#endif // ENGINE_API_H