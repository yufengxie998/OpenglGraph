// SPDX-License-Identifier: GPL-3.0-or-later
#include "Engine.h"
#include "LineRenderer.h"
#include <iostream>
#include <extend/logger.h>
// 1. 首先定义STB实现宏，然后包含头文件
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"  // 确保这个路径正确

#ifdef PLATFORM_ANDROID
    #include <GLES3/gl3.h>
    #include <glm/gtc/matrix_transform.hpp>
    #include <glm/gtc/type_ptr.hpp>
#endif


namespace Engine {

// 静态成员初始化
Engine* Engine::s_instance = nullptr;
float Engine::lastX = 400;
float Engine::lastY = 300;
bool Engine::firstMouse = true;
float Engine::deltaTime = 0.0f;
float Engine::lastFrame = 0.0f;

// Engine实现
Engine::Engine() {
    s_instance = this;
}

Engine::~Engine() {
    Shutdown();
}

Engine& Engine::GetInstance() {
    if (!s_instance) {
        s_instance = new Engine();
    }
    return *s_instance;
}

#ifdef PLATFORM_ANDROID
// Android 平台初始化
bool Engine::Initialize(const EngineConfig& config) {
    LOGI("Engine","Initializing Engine for Android");
    
    this->config = config;
    this->width = config.width;
    this->height = config.height;
    this->internalStoragePath = config.internalStoragePath; 
        // 获取 OpenGL 信息（用于调试）
    const GLubyte* renderer = glGetString(GL_RENDERER);
    if (renderer == nullptr) {
        LOGI("Engine","OpenGL context not ready in Engine::Initialize!");
        return false;
    }
    const GLubyte* version = glGetString(GL_VERSION);
    LOGI("Engine","OpenGL Renderer: %s", (renderer ? (const char*)renderer : "unknown"));
    LOGI("Engine","OpenGL Version: %s", (version ? (const char*)version : "unknown"));

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1, 0.1, 0.1, 0.1);
    // 初始化摄像机
    camera = std::make_unique<Camera>(config.cameraPosition);
 
    // 初始化着色器
    if (!InitShaders()) {
        LOGI("Engine","Failed to initialize shaders");
        return false;
    }
   // 初始化测试三角形
    LOGI("Engine","Engine::Initialize test triangle...");
    if (testTriangleEnabled && !initTestTriangle()) {
        LOGI("Engine","Failed to initialize test triangle, but continuing...");
    }
    // 初始化线条渲染器
    if (config.enableLineRenderer) {
        LOGI("Engine","Initializing LineRenderer...");
        bool lineInitResult = InitLineRenderer();
        LOGI("Engine","LineRenderer initialization result: %s", lineInitResult ? "SUCCESS" : "FAILED");
    }
    LOGI("Engine","Engine initialized successfully");
    return true;
}


// 初始化测试三角形
bool Engine::initTestTriangle() {
    LOGI("Engine","Engine::initTestTriangle Initializing test triangle...");
    
    // 简单的顶点着色器
    const char* vertexShaderSource = 
        "#version 300 es\n"
        "layout (location = 0) in vec3 aPos;\n"
        "uniform mat4 uMVP;\n"
        "void main() {\n"
        "    gl_Position = uMVP * vec4(aPos, 1.0);\n"
        "}\n";
    
    // 简单的片段着色器 - 直接输出黄色
    const char* fragmentShaderSource = 
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = vec4(1.0, 1.0, 0.0, 1.0);\n"  // 黄色
        "}\n";
    
    // 编译顶点着色器
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        LOGI("Engine","Test vertex shader compilation failed: %s", infoLog);
        return false;
    }
    
    // 编译片段着色器
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        LOGI("Engine","Test fragment shader compilation failed: %s", infoLog);
        return false;
    }
    
    // 链接着色器程序
    testShaderProgram = glCreateProgram();
    glAttachShader(testShaderProgram, vertexShader);
    glAttachShader(testShaderProgram, fragmentShader);
    glLinkProgram(testShaderProgram);
    
    glGetProgramiv(testShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(testShaderProgram, 512, NULL, infoLog);
        LOGI("Engine","Test shader program linking failed: %s", infoLog);
        return false;
    }
    
    testMVPLoc = glGetUniformLocation(testShaderProgram, "uMVP");
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // 设置三角形顶点数据
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };
    
    glGenVertexArrays(1, &testVAO);
    glGenBuffers(1, &testVBO);
    
    glBindVertexArray(testVAO);
    glBindBuffer(GL_ARRAY_BUFFER, testVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    LOGI("Engine","Test triangle initialized successfully");
    return true;
}

void Engine::RenderTestTriangle() {
    // 添加详细日志
    static int frameCount = 0;
    frameCount++;
    
    // // 每60帧打印一次，确认渲染循环在运行
    // if (frameCount % 1000 == 0) {
    //     LOGI("Engine","========== RenderTestTriangle ==========");
    //     LOGI("Engine","Frame: %d", frameCount);
    //     LOGI("Engine","testTriangleEnabled: %s", testTriangleEnabled ? "true" : "false");
    //     LOGI("Engine","testShaderProgram: %u", testShaderProgram);
    //     LOGI("Engine","testVAO: %u", testVAO);
    //     LOGI("Engine","testMVPLoc: %d", testMVPLoc);
    // }
    
    if (!testTriangleEnabled) {
        if (frameCount % 1000 == 0) LOGI("Engine","→ Skipping: testTriangleEnabled is false");
        return;
    }
    
    if (testShaderProgram == 0) {
        if (frameCount % 1000 == 0) LOGI("Engine","→ Skipping: testShaderProgram is 0");
        return;
    }
    
    if (testVAO == 0) {
        if (frameCount % 1000 == 0) LOGI("Engine","→ Skipping: testVAO is 0");
        return;
    }
    // 简单的旋转
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, testTriangleTime * 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    
    // 视图矩阵
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    // 投影矩阵
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        (float)width / (float)height,
        0.1f, 100.0f
    );
    
    glm::mat4 mvp = projection * view * model;
    
    glUseProgram(testShaderProgram);
    glUniformMatrix4fv(testMVPLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    
    glBindVertexArray(testVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    glBindVertexArray(0);
    glUseProgram(0);
}



void Engine::Shutdown() {
    LOGI("Engine","Shutting down Engine");
    
    if (lineRenderer) {
        lineRenderer->Shutdown();
        lineRenderer.reset();
    }
    // 清理测试三角形资源
    if (testVAO) glDeleteVertexArrays(1, &testVAO);
    if (testVBO) glDeleteBuffers(1, &testVBO);
    if (testShaderProgram) glDeleteProgram(testShaderProgram);
    renderObjects.clear();
    camera.reset();
    shader.reset();
    LOGI("Engine","Engine shutdown complete");

}

void Engine::RenderFrame(float deltaTime)
{
    // 清除缓冲区
    glClearColor(config.clearColor[0], config.clearColor[1], 
                 config.clearColor[2], config.clearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 更新所有对象
    for (auto& obj : renderObjects) {
        obj->Update(deltaTime);
    }
    
    // 更新线条
    if (lineRenderer && lineRendererInitialized) {
        lineRenderer->Update(deltaTime);
    }
    
    // 调用更新回调
    if (updateCallback) {
        updateCallback(deltaTime);
    }
    
    // 设置相机矩阵
    if (camera) {
        glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom),
                                                (float)width / (float)height, 0.1f, 100.0f);
        glm::mat4 view = camera->GetViewMatrix();

                // 检查是否有渲染对象
        static int frameCount = 0;
        frameCount++;
        if (frameCount % 200 == 0) {
            LOGI("Engine","RenderFrame: renderObjects count = %zu", renderObjects.size());
        }

        // 使用 shader（如果存在）
        if (shader) {
            shader->use();
            shader->setMat4("projection", projection);
            shader->setMat4("view", view);
            
            // 【重要】设置光照相关 uniform
            // shader->setVec3("viewPos", camera->Position);
            
            // // 设置一个简单的光源位置
            // shader->setVec3("lightPos", glm::vec3(2.0f, 2.0f, 2.0f));
            // shader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
            
            // 设置纹理 uniform
            // shader->setInt("texture_diffuse1", 0);
            
            // // 检查是否有纹理
            // bool hasTexture = false;
            // for (auto& obj : renderObjects) {
            //     auto modelObj = std::dynamic_pointer_cast<ModelObject>(obj);
            //     if (modelObj && !modelObj->GetTextures().empty()) {
            //         hasTexture = true;
            //         break;
            //     }
            // }
            // shader->setBool("hasTexture", hasTexture);
            static int debugMode = 3; // 可以手动改这个值来测试
            shader->setInt("debugMode", debugMode);
            // 渲染所有对象
            for (auto& obj : renderObjects) {
                shader->setMat4("model", obj->GetModelMatrix());
                obj->Render(*shader);
            }
        }
        else{
            if (frameCount % 200 == 0) {
                LOGI("Engine","Shader is null!");
            }
        }
        
        // 渲染线条（使用专门的线条渲染器，可能不需要 shader）
        if (lineRenderer && lineRendererInitialized) {
            lineRenderer->Render(view, projection);
        }
        
        // 调用渲染回调
        if (renderCallback) {
            renderCallback();
        }
    }
    
    // 渲染测试三角形（如果有）
    testTriangleTime += deltaTime;
    if(testTriangleEnabled) RenderTestTriangle();
}


void Engine::Run() {
    LOGI("Engine","Android Engine::Run started");

    isRunning = true;
    int frameCount = 0;
    
    while (isRunning) {
        frameCount++;
        
        // 计算时间
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        float currentFrame = now.tv_sec + now.tv_nsec / 1000000000.0f;
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // 更新测试三角形时间
        testTriangleTime += deltaTime;
        
        // 每300帧打印一次状态
        if (frameCount % 300 == 0) {
            LOGI("Engine","Engine running - frame=%d, time=%.2f, delta=%.4f", 
                 frameCount, testTriangleTime, deltaTime);
        }
        
        // 清除缓冲区
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 更新回调
        if (updateCallback) {
            updateCallback(deltaTime);
        }
        
        // 更新线条
        if (lineRenderer && lineRendererInitialized) {
            lineRenderer->Update(deltaTime);
        }
        
        // 渲染
        if (camera) {
            glm::mat4 projection = glm::perspective(
                glm::radians(camera->Zoom), 
                (float)width / (float)height, 
                0.1f, 100.0f
            );
            glm::mat4 view = camera->GetViewMatrix();
            
            // 渲染测试三角形
            if(testTriangleEnabled) RenderTestTriangle();
            
            // 渲染其他对象
            if (shader) {
                shader->use();
                shader->setMat4("projection", projection);
                shader->setMat4("view", view);
                
                for (auto& obj : renderObjects) {
                    shader->setMat4("model", obj->GetModelMatrix());
                    obj->Render(*shader);
                }
            }
            
            // 渲染线条
            if (lineRenderer && lineRendererInitialized) {
                lineRenderer->Render(view, projection);
            }
        }
    }
}
// Android平台的输入处理函数
void Engine::HandleTouchEvent(int action, float x, float y) {
    if (!camera) return;
    
    static float lastTouchX = 0;
    static float lastTouchY = 0;
    static bool firstTouch = true;
    static bool isDragging = false;
    
    switch (action) {
        case 0: // ACTION_DOWN
            firstTouch = true;
            isDragging = true;
            lastTouchX = x;
            lastTouchY = y;
            LOGI("Engine","Touch DOWN at (%.2f, %.2f)", x, y);
            break;
            
        case 1: // ACTION_UP
        case 3: // ACTION_CANCEL
            isDragging = false;
            firstTouch = true;
            LOGI("Engine","Touch UP");
            break;
            
        case 2: // ACTION_MOVE
            if (!isDragging) return;
            
            if (firstTouch) {
                lastTouchX = x;
                lastTouchY = y;
                firstTouch = false;
                return;
            }
            
            // 计算移动距离
            float xoffset = x - lastTouchX;
            float yoffset = y - lastTouchY;
            
            // 如果移动距离太小，忽略
            if (fabs(xoffset) < 0.5f && fabs(yoffset) < 0.5f) {
                return;
            }
            
            // 调整灵敏度
            xoffset *= config.touchSensitivity;
            yoffset *= config.touchSensitivity;
            
            // 处理摄像机旋转
            camera->ProcessMouseMovement(xoffset, yoffset);
            
            lastTouchX = x;
            lastTouchY = y;
            break;
    }
}

void Engine::HandlePinchEvent(float scaleFactor) {
    if (!camera) return;
    
    // 限制缩放速度
    const float MAX_SCALE_FACTOR = 1.5f;
    const float MIN_SCALE_FACTOR = 0.5f;
    
    if (scaleFactor > MAX_SCALE_FACTOR) scaleFactor = MAX_SCALE_FACTOR;
    if (scaleFactor < MIN_SCALE_FACTOR) scaleFactor = MIN_SCALE_FACTOR;
    
    // 缩放因子转换为摄像机缩放
    // scaleFactor > 1 表示放大，< 1 表示缩小
    float zoomDelta = (scaleFactor - 1.0f) * 20.0f;
    camera->ProcessMouseScroll(zoomDelta);
    
    LOGI("Engine","Pinch scale factor: %.2f, zoom delta: %.2f", scaleFactor, zoomDelta);
}

void Engine::HandleRotateEvent(float angle) {
    if (!camera) return;
    
    // 处理双指旋转（可选功能）
    // 这里可以旋转摄像机或场景
    static float totalRotation = 0.0f;
    totalRotation += angle * config.touchSensitivity;
    
    // 例如：围绕Y轴旋转摄像机位置
    // 或者可以旋转模型
    LOGI("Engine","Rotate angle: %.2f, total: %.2f", angle, totalRotation);
}

void Engine::HandleKeyEvent(int keyCode, int action) {
    if (!camera) return;
    
    float dt = deltaTime > 0 ? deltaTime : 0.016f; // 默认60fps
    
    // Android键码映射
    // 参考：https://developer.android.com/reference/android/view/KeyEvent
    bool isDown = (action == 0); // ACTION_DOWN
    
    if (!isDown) return; // 我们只处理按下事件
    
    switch (keyCode) {
        case 19: // KEYCODE_DPAD_UP
        case 51: // KEYCODE_W (如果连接了键盘)
            camera->ProcessKeyboard(FORWARD, dt);
            LOGI("Engine","Key: Forward");
            break;
        case 20: // KEYCODE_DPAD_DOWN
        case 47: // KEYCODE_S
            camera->ProcessKeyboard(BACKWARD, dt);
            LOGI("Engine","Key: Backward");
            break;
        case 21: // KEYCODE_DPAD_LEFT
        case 29: // KEYCODE_A
            camera->ProcessKeyboard(LEFT, dt);
            LOGI("Engine","Key: Left");
            break;
        case 22: // KEYCODE_DPAD_RIGHT
        case 32: // KEYCODE_D
            camera->ProcessKeyboard(RIGHT, dt);
            LOGI("Engine","Key: Right");
            break;
        case 4:  // KEYCODE_BACK
            // 处理返回键
            LOGI("Engine","Key: Back");
            break;
        case 24: // KEYCODE_VOLUME_UP
            camera->ProcessMouseScroll(1.0f); // 放大
            LOGI("Engine","Key: Volume Up (Zoom In)");
            break;
        case 25: // KEYCODE_VOLUME_DOWN
            camera->ProcessMouseScroll(-1.0f); // 缩小
            LOGI("Engine","Key: Volume Down (Zoom Out)");
            break;
    }
}

void Engine::SetTouchSensitivity(float sensitivity) {
    config.touchSensitivity = sensitivity;
    LOGI("Engine","Touch sensitivity set to: %.2f", sensitivity);
}

#else
// 桌面平台初始化
bool Engine::Initialize(const EngineConfig& config) {
    LOGI("Engine","Initializing Engine for Desktop");
    
    this->config = config;
    
    // 初始化 GLFW
    if (!glfwInit()) {
        LOGI("Engine","Failed to initialize GLFW");
        return false;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    window = glfwCreateWindow(config.width, config.height, config.title.c_str(), NULL, NULL);
    if (!window) {
        LOGI("Engine","Failed to create GLFW window");
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetKeyCallback(window, KeyCallback);
    
    glfwSetInputMode(window, GLFW_CURSOR, config.enableMouseCapture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    
    // 初始化 GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        LOGI("Engine","Failed to initialize GLAD");
        return false;
    }
    
    // 初始化 OpenGL 状态
    if (config.enableDepthTest) {
        glEnable(GL_DEPTH_TEST);
    }
    
    if (config.enableWireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    
    // 设置纹理翻转
    stbi_set_flip_vertically_on_load(true);
    
    // 初始化摄像机
    camera = std::make_unique<Camera>(config.cameraPosition);
    // 初始化着色器
    if (!InitShaders()) {
        LOGI("Engine","Failed to initialize shaders");
        return false;
    }
    // 初始化线条渲染器
    if (config.enableLineRenderer) {
        InitLineRenderer();
    }
    
    LOGI("Engine","Engine initialized successfully");
    return true;
}
void Engine::Shutdown() {
    LOGI("Engine","Shutting down Engine");
    
    if (lineRenderer) {
        lineRenderer->Shutdown();
        lineRenderer.reset();
    }
    
    renderObjects.clear();
    camera.reset();
    shader.reset();
    
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    
    glfwTerminate();
}

void Engine::Run() {
    isRunning = true;
    
    while (isRunning && !glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        glfwPollEvents();
              
        // 更新回调
        if (updateCallback) {
            updateCallback(deltaTime);
        }
        
        // 更新所有渲染对象
        for (auto& obj : renderObjects) {
            obj->Update(deltaTime);
        }
        glClearColor(config.clearColor[0], config.clearColor[1], 
            config.clearColor[2], config.clearColor[3]);
        // glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        if (lineRenderer && lineRendererInitialized) {
            lineRenderer->Update(deltaTime);
        }
        shader->use();
        
        // 设置视图和投影矩阵
        glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), 
                                                (float)config.width / (float)config.height, 
                                                0.1f, 100.0f);
        glm::mat4 view = camera->GetViewMatrix();
        shader->setMat4("projection", projection);
        shader->setMat4("view", view);
        
        // 渲染回调
        if (renderCallback) {
            renderCallback();
        }
        
        // 渲染所有对象
        for (auto& obj : renderObjects) {
            shader->setMat4("model", obj->GetModelMatrix());
            obj->Render(*shader);
        }
        RenderLines();
        glfwSwapBuffers(window);
    }
}


void Engine::RenderFrame(float deltaTime) {
    LOGI("Engine","PC Engine::RenderFrame started"); //pc不用Engine::RenderFrame，Android用的方法
    return;
}
// GLFW 回调函数
void Engine::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    if (s_instance) {
        s_instance->config.width = width;
        s_instance->config.height = height;
    }
}

void Engine::MouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
    if (!s_instance || !s_instance->camera) return;
    
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    
    lastX = xpos;
    lastY = ypos;
    
    s_instance->camera->ProcessMouseMovement(xoffset, yoffset);
}

void Engine::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (s_instance && s_instance->camera) {
        s_instance->camera->ProcessMouseScroll(static_cast<float>(yoffset));
    }
}

void Engine::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
        // Tab键：切换鼠标模式
        static bool f1Pressed = false;
        if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !f1Pressed)
        {
            f1Pressed = true;
    
            int mode = glfwGetInputMode(window, GLFW_CURSOR);
            if (mode == GLFW_CURSOR_DISABLED)
            {
                // 切换到UI模式
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                std::cout << "UI Mode: Mouse is free for clicking" << std::endl;
            }
            else
            {
                // 切换到3D模式
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                firstMouse = true;
                std::cout << "3D Mode: Mouse controls camera" << std::endl;
            }
        }
    
        if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE)
        {
            f1Pressed = false;
        }

    // 摄像机键盘控制
    if (s_instance && s_instance->camera) {
        float dt = deltaTime;
        if (key == GLFW_KEY_W) s_instance->camera->ProcessKeyboard(FORWARD, dt);
        if (key == GLFW_KEY_S) s_instance->camera->ProcessKeyboard(BACKWARD, dt);
        if (key == GLFW_KEY_A) s_instance->camera->ProcessKeyboard(LEFT, dt);
        if (key == GLFW_KEY_D) s_instance->camera->ProcessKeyboard(RIGHT, dt);
    }

}

#endif

bool Engine::InitShaders() {
    try {
        #ifdef PLATFORM_ANDROID
            LOGI("Engine", "Android Engine::InitShaders started - using file-based shaders");
            
            std::string vsPath = config.internalStoragePath + "/model_loading_es.vs";
            std::string fsPath = config.internalStoragePath + "/model_loading_es.fs";
            
            LOGI("Engine", "Loading vertex shader from: %s", vsPath.c_str());
            LOGI("Engine", "Loading fragment shader from: %s", fsPath.c_str());
            
            // 检查文件是否存在
            FILE* vsFile = fopen(vsPath.c_str(), "r");
            FILE* fsFile = fopen(fsPath.c_str(), "r");
            
            if (!vsFile || !fsFile) {
                if (vsFile) fclose(vsFile);
                if (fsFile) fclose(fsFile);
                LOGE("Engine", "Shader files not found, using fallback shaders");
                
                // 使用硬编码的备用着色器
                const char* fallbackVS = 
                    "#version 300 es\n"
                    "layout (location = 0) in vec3 aPos;\n"
                    "layout (location = 1) in vec2 aTexCoord;\n"
                    "out vec2 TexCoord;\n"
                    "uniform mat4 model;\n"
                    "uniform mat4 view;\n"
                    "uniform mat4 projection;\n"
                    "void main() {\n"
                    "    TexCoord = aTexCoord;\n"
                    "    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
                    "}\n";
                
                const char* fallbackFS = 
                    "#version 300 es\n"
                    "precision mediump float;\n"
                    "in vec2 TexCoord;\n"
                    "out vec4 FragColor;\n"
                    "uniform sampler2D texture_diffuse1;\n"
                    "uniform bool hasTexture;\n"
                    "void main() {\n"
                    "    if (hasTexture) {\n"
                    "        FragColor = texture(texture_diffuse1, TexCoord);\n"
                    "    } else {\n"
                    "        FragColor = vec4(0.8, 0.8, 0.8, 1.0);\n"
                    "    }\n"
                    "}\n";
                
                // 调用字符串源码构造函数（第三个参数 fromSource 默认为 true）
                shader = std::make_unique<Shader>(fallbackVS, fallbackFS);
                return true;
            }
            
            fclose(vsFile);
            fclose(fsFile);
            
            // 调用文件路径构造函数（第三个参数 geometryPath 默认为 nullptr）
            shader = std::make_unique<Shader>(vsPath.c_str(), fsPath.c_str(),true);
            
        #else
            // Windows 平台使用文件
            LOGI("Engine", "PC Engine::InitShaders started");
            shader = std::make_unique<Shader>("1.model_loading.vs", "1.model_loading.fs",true);
        #endif
        
        // 设置着色器 uniform 默认值
        if (shader && shader->ID) {
            shader->use();
            shader->setInt("texture_diffuse1", 0);
            shader->setBool("hasTexture", false);
            LOGI("Engine", "Shader program created successfully, ID: %d", shader->ID);
        }
        
        return true;
    } catch (const std::exception& e) {
        LOGE("Engine", "Failed to load shaders: %s", e.what());
        return false;
    }
}

void Engine::Stop() {
    isRunning = false;
}


bool Engine::InitLineRenderer() {
    if (!lineRenderer) {
        lineRenderer = std::make_unique<LineRenderer>();
        lineRendererInitialized = lineRenderer->Initialize();
        if (lineRendererInitialized) {
            std::cout << "LineRenderer initialized successfully!" << std::endl;
        } else {
            std::cerr << "Failed to initialize LineRenderer!" << std::endl;
        }
    }
    return lineRendererInitialized;
}

void Engine::RenderLines() {
    if (lineRenderer && lineRendererInitialized) {
        glm::mat4 view = camera->GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom),
                                               (float)config.width / (float)config.height,
                                               0.1f, 100.0f);
        lineRenderer->Render(view, projection);
    }
}

void Engine::AddRenderObject(std::shared_ptr<RenderObject> obj) {
    renderObjects.push_back(obj);
}

void Engine::RemoveRenderObject(std::shared_ptr<RenderObject> obj) {
    renderObjects.erase(
        std::remove(renderObjects.begin(), renderObjects.end(), obj),
        renderObjects.end()
    );
    auto it = std::find(renderObjects.begin(), renderObjects.end(), obj);
    if (it != renderObjects.end()) {
        renderObjects.erase(it);
    }
}

void Engine::ClearRenderObjects() {
    renderObjects.clear();
}

void Engine::SetUpdateCallback(std::function<void(float)> callback) {
    updateCallback = callback;
}

void Engine::SetRenderCallback(std::function<void()> callback) {
    renderCallback = callback;
}

void Engine::SetCameraPosition(const glm::vec3& pos) {
    if (camera) {
        camera->Position = pos;
    }
}

void Engine::SetCameraZoom(float zoom) {
    if (camera) {
        camera->Zoom = zoom;
    }
}


unsigned int Engine::LoadTexture(const std::string& path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    LOGI("Engine", "Loading texture from: %s", path.c_str());
    
    // 检查文件是否存在
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) {
        LOGE("Engine", "Texture file does not exist: %s", path.c_str());
        return 0;
    }
    fclose(file);
    
    // 强制加载为RGBA
    int width, height, nrComponents;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrComponents, STBI_rgb_alpha);
    
    if (data) {
        LOGI("Engine", "Texture data loaded: %dx%d, components: %d (converted to RGBA)", 
             width, height, nrComponents);
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        // 使用GL_RGBA格式上传纹理数据
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        // 设置纹理参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // 检查OpenGL错误
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            LOGE("Engine", "OpenGL error after texture upload: 0x%x", err);
        } else {
            LOGI("Engine", "Texture loaded successfully with ID: %d", textureID);
        }
        
        stbi_image_free(data);
        return textureID;
    } else {
        LOGE("Engine", "Texture failed to load at path: %s", path.c_str());
        LOGE("Engine", "STB Error: %s", stbi_failure_reason());
        return 0;
    }
}


std::shared_ptr<ModelObject> Engine::LoadModel(const std::string& path) {
    return std::make_shared<ModelObject>(path);
}

} // namespace Engine