#include <jni.h>
#include <string>
#include <android/log.h>
#include <glm/gtc/type_ptr.hpp>
#include "EngineAPI.h"
#include <extend/logger.h>

// 全局变量，用于跟踪引擎状态
static bool engineInitialized = false;
static bool enginePaused = false; // 添加暂停状态
static float lastFrameTime = 0;
static int g_modelId = -1;           // 保存模型ID
static float g_rotationAngle = 0.0f; // 添加旋转角度变量
// JNI 函数声明
extern "C"
{
    JNIEXPORT void JNICALL
    Java_com_example_androidengineapp_MainActivity_initEngine(JNIEnv *env, jobject thiz, jint width, jint height, jstring internalStoragePath);

    JNIEXPORT void JNICALL
    Java_com_example_androidengineapp_MainActivity_drawFrame(JNIEnv *env, jobject thiz, jfloat deltaTime);

    JNIEXPORT void JNICALL
    Java_com_example_androidengineapp_MainActivity_nativeOnPause(JNIEnv *env, jobject thiz); // 新增

    JNIEXPORT void JNICALL
    Java_com_example_androidengineapp_MainActivity_nativeOnResume(JNIEnv *env, jobject thiz); // 新增

    JNIEXPORT void JNICALL
    Java_com_example_androidengineapp_MainActivity_shutdownEngine(JNIEnv *env, jobject thiz);

    JNIEXPORT void JNICALL
    Java_com_example_androidengineapp_MainActivity_handleTouchEvent(JNIEnv *env, jobject thiz, jint action, jfloat x, jfloat y);
    JNIEXPORT void JNICALL
    Java_com_example_androidengineapp_MainActivity_handlePinchEvent(JNIEnv *env, jobject thiz, jfloat scaleFactor);
    JNIEXPORT void JNICALL
    Java_com_example_androidengineapp_MainActivity_handleKeyEvent(JNIEnv *env, jobject thiz, jint keyCode, jint action);
    JNIEXPORT void JNICALL
    Java_com_example_androidengineapp_MainActivity_setTouchSensitivity(JNIEnv *env, jobject thiz, jfloat sensitivity);
    JNIEXPORT void JNICALL
    Java_com_example_androidengineapp_MainActivity_handleRotateEvent(JNIEnv *env, jobject thiz, jfloat angle);

    JNIEXPORT jstring JNICALL Java_com_example_androidengineapp_MainActivity_stringFromJNI(JNIEnv *env, jobject thiz);
}

// 初始化引擎
JNIEXPORT void JNICALL
Java_com_example_androidengineapp_MainActivity_initEngine(JNIEnv
                                                              *env,
                                                          jobject thiz, jint width,
                                                          jint height, jstring internalStoragePath)
{
    LOGI("EngineJNI", "========== initEngine called ==========");

    if (engineInitialized)
    {
        LOGI("EngineJNI", "Engine already initialized");
        return;
    }

    const GLubyte *renderer = glGetString(GL_RENDERER);
    if (renderer == nullptr)
    {
        LOGE("EngineJNI", "OpenGL context not ready yet!");
        return;
    }

    LOGI("EngineJNI", "OpenGL Renderer: %s", (const char *)renderer);
    // 获取内部存储路径
    const char *storagePathStr = env->GetStringUTFChars(internalStoragePath, nullptr);
    std::string storagePath(storagePathStr);
    //    EngineAPI::EnableTestTriangle(true);

    // 创建引擎配置
    Engine::EngineConfig config;
    config.width = width;
    config.height = height;
    config.title = "Android Engine App";
    config.enableLineRenderer = true;
    config.internalStoragePath = storagePath;

    if (!EngineAPI::InitWithConfig(config))
    {
        LOGE("EngineJNI", "Failed to initialize engine");
        env->ReleaseStringUTFChars(internalStoragePath, storagePathStr);
        return;
    }
    // if (!EngineAPI::Init(width, height, "Android Engine App"))
    // {
    //     LOGE("Failed to initialize engine");
    //     return;
    // }

    // 设置摄像机
    EngineAPI::SetCameraPosition(0, 5, 20);
    EngineAPI::SetCameraZoom(45.0f);
    LOGI("EngineJNI", "Camera setup complete");
    EngineAPI::DrawGrid(10.0f, 20, 0.0f, 0.3f, 0.3f, 0.3f);
    EngineAPI::DrawAxis(3.0f);
    EngineAPI::DrawLine(
        {0.0f, 0.0f, 0.0f}, // 起点（原点）
        {2.0f, 0.5f, 2.0f}, // 终点（立方体位置）
        {1.0f, 1.0f, 0.0f}, // 黄色
        -1.0f);
    // 绘制实线
    EngineAPI::DrawLine(glm::vec3(0, 0, 0), glm::vec3(2, 0, 0), glm::vec3(1, 0, 0));

    // 绘制虚线
    EngineAPI::DrawDashedLine(glm::vec3(0, 1, 0), glm::vec3(2, 1, 0),
                              glm::vec3(0, 1, 0), 0.15f, 0.08f);

    // 绘制点划线
    EngineAPI::DrawStyledLine(glm::vec3(0, 2, 0), glm::vec3(2, 2, 0),
                              glm::vec3(0, 0, 1), LineStyle::DASH_DOTTED,
                              0.1f, 0.05f);

    // 绘制虚线圆
    EngineAPI::DrawCircle(glm::vec3(0, 3, 0), 1.0f, glm::vec3(1, 1, 0),
                          LineStyle::DASHED, 32);
    // 从内部存储加载模型
    if (internalStoragePath != nullptr)
    {
        // const char *storagePathStr = env->GetStringUTFChars(internalStoragePath, nullptr);

        // 构建模型文件路径
        std::string modelPath = std::string(storagePathStr) + "/cube2.glb";
        std::string texturePath = std::string(storagePathStr) + "/2.png";

        LOGI("EngineJNI", "Loading model from: %s", modelPath.c_str());
        LOGI("EngineJNI", "Texture path: %s", texturePath.c_str());

        // 检查文件是否存在（可选）
        FILE *file = fopen(modelPath.c_str(), "r");
        if (file)
        {
            fclose(file);
            LOGI("EngineJNI", "Model file exists");

            // 加载模型
            g_modelId = EngineAPI::LoadModel(modelPath.c_str());
            if (g_modelId >= 0)
            {
                LOGI("EngineJNI", "Model loaded successfully with ID: %d", g_modelId);

                // 设置纹理
                if (EngineAPI::SetModelTexture(g_modelId, texturePath.c_str()))
                {
                    LOGI("EngineJNI", "Texture set successfully");
                }
                else
                {
                    LOGI("EngineJNI", "Failed to set texture");
                }

                EngineAPI::SetEntityPosition(g_modelId, 3, 3, 0);
                EngineAPI::SetEntityScale(g_modelId, 0.8, 0.8, 0.8);

                LOGI("EngineJNI", "Model rotation applied (45 degrees)");

                int entityId = EngineAPI::LoadModel(modelPath.c_str());
                if (entityId < 0)
                {
                    std::cerr << "Failed to load model!" << std::endl;
                }
                // 创建根实体
                EngineAPI::SetEntityPosition(entityId, 0, 2, 1);
                EngineAPI::SetEntityScale(entityId, 0.8, 0.8, 0.8);
                EngineAPI::SetEntityParent(entityId,g_modelId);
            }
            else
            {
                LOGI("EngineJNI", "Failed to load model!");
            }
        }
        else
        {
            LOGI("EngineJNI", "Model file does not exist: %s", modelPath.c_str());
        }

        // 准备天空盒纹理文件（6张图片）
        std::vector<std::string> skyboxFaces = {
            std::string(storagePathStr) + "/skybox/right.jpg",  // 右
            std::string(storagePathStr) + "/skybox/left.jpg",   // 左
            std::string(storagePathStr) + "/skybox/top.jpg",    // 上
            std::string(storagePathStr) + "/skybox/bottom.jpg", // 下
            std::string(storagePathStr) + "/skybox/front.jpg",  // 前
            std::string(storagePathStr) + "/skybox/back.jpg"    // 后
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
    }
    else
    {
        LOGI("EngineJNI", "Internal storage path is null, skipping model loading");
    }
//
//    // 样条曲线示例
//    std::vector<glm::vec3> splinePoints = {
//        glm::vec3(-5.0f, 1.0f, 0.0f),
//        glm::vec3(-3.0f, 2.0f, 0.0f),
//        glm::vec3(0.0f, 0.0f, 0.0f),
//        glm::vec3(3.0f, 2.0f, 0.0f),
//        glm::vec3(5.0f, 1.0f, 0.0f)};
//    EngineAPI::DrawSplineCurve(splinePoints, glm::vec3(1.0f, 0.0f, 1.0f), 15, -1.0f);
//
//    // 正弦曲线示例
//    EngineAPI::DrawSineWave(
//        glm::vec3(-5.0f, 2.0f, 0.0f), // 起点
//        glm::vec3(1.0f, 0.0f, 0.0f),  // 方向（X轴方向）
//        1.0f,                         // 振幅
//        1.0f,                         // 频率
//        10.0f,                        // 长度
//        glm::vec3(0.0f, 1.0f, 1.0f),  // 青色
//        100,                          // 分段数
//        -1.0f                         // 永久显示
//    );

    // 在 main.cpp 或初始化代码中

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
        glm::vec3(0.0f, 5.0f, 5.0f),   // 位置
        glm::vec3(0.0f, -1.0f, -1.0f), // 方向（指向原点）
        glm::vec3(1.0f, 1.0f, 1.0f),   // 白色
        2.0f,                          // 强度
        12.5f, 17.5f,                  // 内角和外角
        1.0f, 0.07f, 0.017f            // 衰减参数
    );
    // 添加一个平面
    int planeId = EngineAPI::AddPlane(50.0f, 50.0f, 40.f);
    EngineAPI::SetEntityPosition(planeId, 0, -2, 0);
    EngineAPI::SetPrimitiveColor(planeId, 0.5f, 0.5f, 0.5f);
//
//    // 添加一个圆柱体
//    int cylinderId = EngineAPI::AddCylinder(1.0f, 3.0f, 32);
//    EngineAPI::SetEntityPosition(cylinderId, 2, 0, 2);
//    EngineAPI::SetEntityRotationAxis(cylinderId, 50.0f, 0, 1, 0); // 旋转45度
//    EngineAPI::SetPrimitiveColor(cylinderId, 0.8f, 0.2f, 0.2f);
//    // 多项式曲线示例 3: y = x^3 (三次曲线)
//    EngineAPI::DrawPolynomialCurveXZ(
//        0.0f, 0.0f, 0.0f, 6.0f, // c0, c1, c2, c3 (c3=6，因为公式中是c3/6，所以实际系数为1)
//        -2.0f, 2.0f,
//        glm::vec3(0.0f, 0.0f, 1.0f), // 蓝色
//        50, -1.0f);
//
//    // 多项式曲线示例 4: 复杂曲线 y = 1 + 0.5x + x^2 + 0.5x^3
//    EngineAPI::DrawPolynomialCurveXY(
//        1.0f, 0.5f, 2.0f, 3.0f, // c0, c1, c2, c3 (c2=2→实际系数1, c3=3→实际系数0.5)
//        -3.0f, 3.0f,
//        glm::vec3(1.0f, 1.0f, 0.0f), // 黄色
//        100, -1.0f);

    engineInitialized = true;
    enginePaused = false;
    env->ReleaseStringUTFChars(internalStoragePath, storagePathStr);
    LOGI("EngineJNI", "Engine initialized successfully with %d lines", 20 + 3 + 12);
}

// 绘制帧 - 每帧调用
JNIEXPORT void JNICALL
Java_com_example_androidengineapp_MainActivity_drawFrame(JNIEnv
                                                             *env,
                                                         jobject thiz, jfloat deltaTime)
{
    // 【必须】立即打印日志，无论引擎状态如何
    static int callCount = 0;
    callCount++;

    // // 每帧都打印前10帧，之后每100帧打印一次
    // if (callCount <= 10 || callCount % 100 == 0)
    // {
    //     LOGI("EngineJNI","========== drawFrame CALLED! count=%d, delta=%.4f ==========", callCount, deltaTime);
    // }

    if (!engineInitialized || enginePaused)
    {
        if (callCount <= 10)
        {
            LOGI("EngineJNI", "drawFrame skipped - engineInit=%d, enginePaused=%d",
                 engineInitialized, enginePaused);
        }
        return;
    }

    // 每60帧检查OpenGL错误
    if (callCount % 200 == 0)
    {
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
        {
            LOGE("EngineJNI", "OpenGL error in drawFrame: 0x%x", err);
        }
    }

    if (g_modelId >= 0)
    {
        // 设置旋转速度（度/秒）
        const float ROTATION_SPEED = 90.0f; // 每秒旋转90度

        // 根据时间增量更新角度
        g_rotationAngle += ROTATION_SPEED * deltaTime;

        // 限制角度在0-360度范围内（可选）
        if (g_rotationAngle >= 360.0f)
        {
            g_rotationAngle -= 360.0f;
        }

        // 应用新的旋转
        EngineAPI::SetEntityRotationAxis(g_modelId, g_rotationAngle, 0, 1, 0);
    }
    EngineAPI::DrawFrame(deltaTime);
}

// 新增：暂停处理
JNIEXPORT void JNICALL
Java_com_example_androidengineapp_MainActivity_nativeOnPause(JNIEnv *env, jobject thiz)
{
    LOGI("EngineJNI", "onPause called");
    enginePaused = true;
    // 不要 shutdown，只是暂停渲染
}

// 新增：恢复处理
JNIEXPORT void JNICALL
Java_com_example_androidengineapp_MainActivity_nativeOnResume(JNIEnv *env, jobject thiz)
{
    LOGI("EngineJNI", "onResume called");
    enginePaused = false;
}

// 关闭引擎
JNIEXPORT void JNICALL
Java_com_example_androidengineapp_MainActivity_shutdownEngine(JNIEnv
                                                                  *env,
                                                              jobject thiz)
{
    LOGI("EngineJNI", "shutdownEngine called - IGNORING for now");

    //    if (engineInitialized)
    //    {
    //        EngineAPI::Shutdown();
    //
    //        engineInitialized = false;
    //    }
    //
    //    LOGI("EngineJNI","Engine shutdown complete");
}

// native-lib.cpp 中添加Android输入处理的JNI函数

// 触摸事件处理
JNIEXPORT void JNICALL
Java_com_example_androidengineapp_MainActivity_handleTouchEvent(
    JNIEnv *env, jobject thiz, jint action, jfloat x, jfloat y)
{

    LOGI("EngineJNI", "JNI handleTouchEvent: action=%d, (%.2f, %.2f)", action, x, y);
    EngineAPI::HandleTouchEvent(action, x, y);
}

// 双指缩放事件
JNIEXPORT void JNICALL
Java_com_example_androidengineapp_MainActivity_handlePinchEvent(
    JNIEnv *env, jobject thiz, jfloat scaleFactor)
{

    LOGI("EngineJNI", "JNI handlePinchEvent: scaleFactor=%.2f", scaleFactor);
    EngineAPI::HandlePinchEvent(scaleFactor);
}

// 按键事件
JNIEXPORT void JNICALL
Java_com_example_androidengineapp_MainActivity_handleKeyEvent(
    JNIEnv *env, jobject thiz, jint keyCode, jint action)
{

    LOGI("EngineJNI", "JNI handleKeyEvent: keyCode=%d, action=%d", keyCode, action);
    EngineAPI::HandleKeyEvent(keyCode, action);
}

// 设置触摸灵敏度
JNIEXPORT void JNICALL
Java_com_example_androidengineapp_MainActivity_setTouchSensitivity(
    JNIEnv *env, jobject thiz, jfloat sensitivity)
{

    LOGI("EngineJNI", "JNI setTouchSensitivity: %.2f", sensitivity);
    EngineAPI::SetTouchSensitivity(sensitivity);
}

// 双指旋转事件（可选）
JNIEXPORT void JNICALL
Java_com_example_androidengineapp_MainActivity_handleRotateEvent(
    JNIEnv *env, jobject thiz, jfloat angle)
{

    LOGI("EngineJNI", "JNI handleRotateEvent: angle=%.2f", angle);
    EngineAPI::HandleRotateEvent(angle);
}

// 原有的 stringFromJNI 函数
JNIEXPORT jstring

    JNICALL
    Java_com_example_androidengineapp_MainActivity_stringFromJNI(JNIEnv *env, jobject /* this */)
{
    std::string hello = "Hello from C++ with Engine! Engine initialized: " +
                        std::string(engineInitialized ? "Yes" : "No");
    return env->NewStringUTF(hello.c_str());
}