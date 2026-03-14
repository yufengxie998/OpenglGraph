#include <Engine/EngineAPI.h>
#include <iostream>
#include <Engine/windows_compat.h>

// 全局变量存储modelId
int g_modelId = -1;

void OnUpdate(float deltaTime)
{
    static float time = 0;
    time += deltaTime;

    if (g_modelId >= 0)
    {
        EngineAPI::SetEntityRotationAxis(g_modelId, time * 50.0f, 0, 1, 0);
    }

    // 绘制动态线条
    float radius = 2.0f;
    float x1 = sin(time) * radius;
    float z1 = cos(time) * radius;
    float x2 = sin(time + 3.14159f) * radius;
    float z2 = cos(time + 3.14159f) * radius;

    EngineAPI::DrawLine(x1, 0.5f, z1, x2, 0.5f, z2, 1.0f, 0.0f, 0.0f, 0.1f);
}

int main()
{
    std::cout << "Starting Engine Application..." << std::endl;
    Logger::init("my_app.log");
    // 初始化引擎
    if (!EngineAPI::Init(800, 600, "OpenGL Graph Engine Windows App"))
    {
        std::cerr << "Failed to initialize engine!" << std::endl;
        return -1;
    }

    // 准备天空盒纹理文件（6张图片）
    std::vector<std::string> skyboxFaces = {
        "skybox/right.jpg",  // 右
        "skybox/left.jpg",   // 左
        "skybox/top.jpg",    // 上
        "skybox/bottom.jpg", // 下
        "skybox/front.jpg",  // 前
        "skybox/back.jpg"    // 后
    };

    // 加载天空盒
    if (EngineAPI::LoadSkybox(skyboxFaces))
    {
        EngineAPI::EnableSkybox(true);
        LOGI("App", "Skybox loaded and enabled");
    }
    else
    {
        LOGE("App", "Failed to load skybox");
    }

    // 加载模型 - 直接赋值给全局变量
    g_modelId = EngineAPI::LoadModel("cube2.glb");
    if (g_modelId < 0)
    {
        std::cerr << "Failed to load model!" << std::endl;
        return -1;
    }

    // 创建根实体
    EngineAPI::SetEntityPosition(g_modelId, 3, 3, 0);
    EngineAPI::SetEntityScale(g_modelId, 2, 2, 2);

    int entityId = EngineAPI::LoadModel("cube2.glb");
    if (entityId < 0)
    {
        std::cerr << "Failed to load model!" << std::endl;
        return -1;
    }
    // 创建根实体
    EngineAPI::SetEntityPosition(entityId, 0, 2, 1);
    EngineAPI::SetEntityScale(entityId, 0.8, 0.8, 0.8);
    EngineAPI::SetEntityParent(entityId,g_modelId);

    int entityId1 = EngineAPI::LoadModel("cube2.glb");
    if (entityId1 < 0)
    {
        std::cerr << "Failed to load model!" << std::endl;
        return -1;
    }
    // 创建根实体
    EngineAPI::SetEntityPosition(entityId1, 0, 2, 1);
    EngineAPI::SetEntityScale(entityId1, 0.5, 0.5, 0.5);
    EngineAPI::SetEntityParent(entityId1,g_modelId);
    // 设置纹理
    const char *texturePath = "2.png";
    std::cout << "Setting texture: " << texturePath << std::endl;
    EngineAPI::SetModelTexture(entityId, texturePath);

    // // 设置模型变换
    // EngineAPI::SetModelPosition(g_modelId, 0, 0, 0);
    // EngineAPI::SetModelScale(g_modelId, 1, 1, 1);
    // 动画加载需要调试
    // EngineAPI::LoadModelAnimation(g_modelId, "cube2.glb");
    // // 播放动画（如果模型自带动画）
    // EngineAPI::PlayModelAnimation(g_modelId);

    // // 设置动画速度
    // EngineAPI::SetModelAnimationSpeed(g_modelId, 1.5f); // 1.5倍速

    // // 检查动画状态
    // if (EngineAPI::IsModelAnimationPlaying(g_modelId))
    // {
    //     LOGI("Main", "Animation is playing");
    // }
    // 测试旋转 - 立即应用一个旋转，检查模型是否会旋转
    // EngineAPI::SetModelRotation(g_modelId, 0, 1, 0, 45.0f);
    // std::cout << "Test rotation applied (45 degrees)" << std::endl;

    // 静态线条
    EngineAPI::DrawGrid(10.0f, 20, 0.0f, 0.3f, 0.3f, 0.3f);
    EngineAPI::DrawAxis(3.0f);
    EngineAPI::DrawLine(
        {0.0f, 0.0f, 0.0f}, // 起点（原点）
        {2.0f, 0.5f, 2.0f}, // 终点（立方体位置）
        {1.0f, 1.0f, 0.0f}, // 黄色
        -1.0f);
    // EngineAPI::DrawCube({0.0f, 0.0f, 0.0f}, 5.0, {1.0f, 1.0f, 0.0f}, -1.0f);
    // float cubeVertices[] = {
    //     // 底面四条边
    //     -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
    //     1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f,
    //     1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
    //     -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f,

    //     // 顶面四条边
    //     -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f,
    //     1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f,
    //     1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
    //     -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f,

    //     // 四条垂直边
    //     -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
    //     1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f,
    //     1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    //     -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f};
    // EngineAPI::DrawLines(cubeVertices, 72, nullptr, -1.0f); // 永久显示
    // 设置更新回调 - 使用普通函数
    EngineAPI::SetUpdateCallback(OnUpdate);
    std::cout << "Update callback registered" << std::endl;

    // 设置摄像机
    EngineAPI::SetCameraPosition(0, 5, 30);
    EngineAPI::SetCameraZoom(45.0f);

    std::cout << "Engine running..." << std::endl;
    std::cout << "Model ID: " << g_modelId << std::endl;

    // 添加一个环境光（全局光照）
    EngineAPI::AddAmbientLight(glm::vec3(0.2f, 0.2f, 0.2f), 1.0f);

    // 添加几个点光源
    EngineAPI::AddPointLight(
        glm::vec3(3.0f, 3.0f, 3.0f), // 位置
        glm::vec3(1.0f, 0.0f, 0.0f), // 红色
        1.0f,                        // 强度
        1.0f, 0.09f, 0.032f          // 衰减参数
    );

    EngineAPI::AddPointLight(
        glm::vec3(-3.0f, 2.0f, 2.0f), // 位置
        glm::vec3(0.0f, 1.0f, 0.0f),  // 绿色
        1.0f);

    // 添加一个聚光灯
    EngineAPI::AddSpotLight(
        glm::vec3(0.0f, 7.0f, 2.0f),   // 位置
        glm::vec3(0.0f, -1.0f, -1.0f), // 方向（指向原点）
        glm::vec3(1.0f, 1.0f, 1.0f),   // 白色
        2.0f,                          // 强度
        12.5f, 17.5f,                  // 内角和外角
        1.0f, 0.07f, 0.017f            // 衰减参数
    );

    // 添加一个平面
    int planeId = EngineAPI::AddPlane(50.0f, 50.0f, 40.f);
    EngineAPI::SetEntityPosition(planeId, 0, -1, 0);
    EngineAPI::SetPrimitiveColor(planeId, 0.5f, 0.5f, 0.5f);

    // 添加一个圆柱体
    int cylinderId = EngineAPI::AddCylinder(1.0f, 3.0f, 32);
    EngineAPI::SetEntityPosition(cylinderId, 2, 0, 2);
    EngineAPI::SetEntityRotationAxis(cylinderId, 50.0f, 0, 1, 0); // 旋转45度
    EngineAPI::SetPrimitiveColor(cylinderId, 0.8f, 0.2f, 0.2f);

    // 添加另一个圆柱体作为柱子
    int pillarId = EngineAPI::AddCylinder(0.5f, 5.0f, 16);
    EngineAPI::SetEntityPosition(pillarId, -2, 0, -2);
    EngineAPI::SetPrimitiveColor(pillarId, 0.7f, 0.7f, 0.7f);

    EngineAPI::DrawDashedLine(glm::vec3(0, 1, 0), glm::vec3(2, 1, 0),
                              glm::vec3(0, 1, 0), 0.15f, 0.08f);

    // 多项式曲线示例 4: 复杂曲线 y = 1 + 0.5x + x^2 + 0.5x^3
    EngineAPI::DrawPolynomialCurveXY(
        1.0f, 0.5f, 2.0f, 3.0f, // c0, c1, c2, c3 (c2=2→实际系数1, c3=3→实际系数0.5)
        -3.0f, 3.0f,
        glm::vec3(1.0f, 1.0f, 0.0f), // 黄色
        100, -1.0f);

    // 运行主循环
    EngineAPI::Run();

    // 清理
    EngineAPI::Shutdown();
    std::cout << "Engine shutdown." << std::endl;
    Logger::close();
    return 0;
}