#ifdef PLATFORM_ANDROID
// SPDX-License-Identifier: GPL-3.0-or-later
#include "Engine.h"
#include "LineRenderer.h"
#include <extend/logger.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

// STB Image 实现
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Engine
{

    // 静态成员初始化
    Engine *Engine::s_instance = nullptr;
    float Engine::deltaTime = 0.0f;
    float Engine::lastFrame = 0.0f;

    Engine::Engine()
    {
        s_instance = this;
    }

    Engine::~Engine()
    {
        Shutdown();
    }

    Engine &Engine::GetInstance()
    {
        if (!s_instance)
        {
            s_instance = new Engine();
        }
        return *s_instance;
    }

    bool Engine::Initialize(const EngineConfig &config)
    {
        LOGI("Engine", "Initializing Engine for Android");

        this->config = config;
        // this->width = config.width;
        // this->height = config.height;
        // this->internalStoragePath = config.internalStoragePath;

        // 获取 OpenGL 信息（用于调试）
        const GLubyte *renderer = glGetString(GL_RENDERER);
        if (renderer == nullptr)
        {
            LOGI("Engine", "OpenGL context not ready in Engine::Initialize!");
            return false;
        }
        const GLubyte *version = glGetString(GL_VERSION);
        LOGI("Engine", "OpenGL Renderer: %s", (renderer ? (const char *)renderer : "unknown"));
        LOGI("Engine", "OpenGL Version: %s", (version ? (const char *)version : "unknown"));

        glViewport(0, 0, config.width, config.height);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

        // 初始化摄像机
        camera = std::make_unique<Camera>(config.cameraPosition);

        // 初始化着色器
        if (!InitShaders())
        {
            LOGI("Engine", "Failed to initialize shaders");
            return false;
        }

        // 初始化测试三角形
        LOGI("Engine", "Engine::Initialize test triangle...");
        if (testTriangleEnabled && !initTestTriangle())
        {
            LOGI("Engine", "Failed to initialize test triangle, but continuing...");
        }

        // 初始化线条渲染器
        if (config.enableLineRenderer)
        {
            LOGI("Engine", "Initializing LineRenderer...");
            bool lineInitResult = InitLineRenderer();
            LOGI("Engine", "LineRenderer initialization result: %s", lineInitResult ? "SUCCESS" : "FAILED");
        }
        // 初始化遮挡剔除  放到最后
        occlusionCulling = std::make_unique<OcclusionCullingManager>(this);
        if (!occlusionCulling->Initialize())
        {
            std::cerr << "Failed to initialize occlusion culling" << std::endl;
        }

        // 注册所有现有实体到遮挡剔除系统
        for (auto &pair : entities)
        {
            occlusionCulling->RegisterEntity(pair.second);
        }
        LOGI("Engine", "Engine initialized successfully");
        return true;
    }

    void Engine::Shutdown()
    {
        LOGI("Engine", "Shutting down Engine");

        if (lineRenderer)
        {
            lineRenderer->Shutdown();
            lineRenderer.reset();
        }

        // 清理测试三角形资源
        if (testVAO)
            glDeleteVertexArrays(1, &testVAO);
        if (testVBO)
            glDeleteBuffers(1, &testVBO);
        if (testShaderProgram)
            glDeleteProgram(testShaderProgram);

        entities.clear();
        camera.reset();
        shader.reset();
        LOGI("Engine", "Engine shutdown complete");
    }

    void Engine::Run()
    {
        LOGI("Engine", "Android Engine::Run started");
        isRunning = true;

        while (isRunning)
        {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            float currentFrame = now.tv_sec + now.tv_nsec / 1000000000.0f;
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            RenderFrame(deltaTime);
        }
    }

    void Engine::Stop()
    {
        isRunning = false;
    }

    void Engine::RenderFrame(float deltaTime)
    {
        // 清除缓冲区
        glClearColor(config.clearColor[0], config.clearColor[1],
                     config.clearColor[2], config.clearColor[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // 先渲染天空盒（如果启用）
        if (skybox && skyboxEnabled && camera)
        {
            glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)config.width / (float)config.height, 0.1f, 100.0f);
            glm::mat4 view = camera->GetViewMatrix();
            skybox->Render(view, projection);
        }
        // 递归更新所有Entity（包括它们的变换和特定逻辑）
        for (auto &pair : entities)
        {
            if (pair.second)
            {
                pair.second->Update(deltaTime); // 调用每个Entity的Update方法
            }
        }
            // 创建视锥体 - 添加这行
        float aspect = (float)config.width / (float)config.height;
        Frustum frustum = createFrustumFromCamera(
        *camera,
        aspect,
        glm::radians(camera->Zoom),
        0.1f,
        100.0f);
        // ===== 新增：执行遮挡剔除 =====
        if (occlusionCullingEnabled && occlusionCulling)
        {
            glm::mat4 view = camera->GetViewMatrix();
            glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom),
                                                    (float)config.width / (float)config.height,
                                                    0.1f, 100.0f);
            occlusionCulling->Update(frustum, projection * view);
        }
        // 更新线条
        if (lineRenderer && lineRendererInitialized)
        {
            lineRenderer->Update(deltaTime);
        }

        // 调用更新回调
        if (updateCallback)
        {
            updateCallback(deltaTime);
        }

        // 渲染
        if (camera)
        {
            // 更新所有 Entity 的变换
            for (auto &pair : entities)
            {
                pair.second->updateSelfAndChild();
            }
            // 根据当前着色器类型选择对应的着色器
            Shader *activeShader = nullptr;

            if (currentShaderType == SHADER_SIMPLE)
            {
                activeShader = shaderSimple.get();
            }
            else
            {
                activeShader = shaderPBR.get();
            }
            glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)config.width / (float)config.height, 0.1f, 100.0f);
            glm::mat4 view = camera->GetViewMatrix();
            // 使用选中的着色器
            if (activeShader)
            {
                activeShader->use();

                // 所有着色器都需要的通用 uniforms
                activeShader->setMat4("projection", projection);
                activeShader->setMat4("view", view);
                static ShaderType lastLoggedType = SHADER_SIMPLE;
                // 根据着色器类型设置特定的 uniforms
                if (currentShaderType == SHADER_SIMPLE)
                {
                    // 简单纹理着色器 - 只需要基础矩阵，hasTexture 在 ModelObject::Render 中设置
                    if (lastLoggedType != SHADER_SIMPLE)
                    {
                        LOGI("Engine", "Using SIMPLE shader");
                        lastLoggedType = SHADER_SIMPLE;
                    }
                }
                else if (currentShaderType == SHADER_PBR)
                {
                    // PBR着色器 - 设置所有PBR相关 uniforms
                    if (lastLoggedType != SHADER_PBR)
                    {
                        LOGI("Engine", "Using PBR shader");
                        lastLoggedType = SHADER_PBR;
                    }

                    activeShader->setVec3("viewPos", camera->Position);

                    activeShader->setInt("lightCount", lights.size());
                    for (size_t i = 0; i < lights.size(); i++)
                    {
                        const Light &light = lights[i];
                        std::string prefix = "lights[" + std::to_string(i) + "].";

                        activeShader->setInt(prefix + "type", static_cast<int>(light.type));
                        activeShader->setVec3(prefix + "position", light.position);
                        activeShader->setVec3(prefix + "direction", light.direction);
                        activeShader->setVec3(prefix + "color", light.color);
                        activeShader->setFloat(prefix + "intensity", light.intensity);

                        // 点光源衰减参数
                        activeShader->setFloat(prefix + "constant", light.constant);
                        activeShader->setFloat(prefix + "linear", light.linear);
                        activeShader->setFloat(prefix + "quadratic", light.quadratic);

                        // 聚光灯参数
                        activeShader->setFloat(prefix + "cutOff", light.cutOff);
                        activeShader->setFloat(prefix + "outerCutOff", light.outerCutOff);
                    }

                    // 设置材质默认值
                    activeShader->setVec3("albedoColor", glm::vec3(0.8f, 0.8f, 0.8f));
                    activeShader->setFloat("metallicUniform", 0.1f);
                    activeShader->setFloat("roughnessUniform", 0.3f);
                    activeShader->setFloat("aoUniform", 1.0f);
                    activeShader->setFloat("emissiveIntensity", 1.0f);

                    // 设置纹理使用标志（可以从模型对象获取）
                    bool hasTextures = false;
                    auto modelObjects = GetEntitiesByType<ModelObject>();
                    if (!modelObjects.empty())
                    {
                        hasTextures = !modelObjects[0]->GetTextures().empty();
                    }
                    activeShader->setBool("useAlbedoMap", hasTextures);
                    activeShader->setBool("useNormalMap", false);
                    activeShader->setBool("useMetallicMap", false);
                    activeShader->setBool("useRoughnessMap", false);
                    activeShader->setBool("useAOMap", false);
                    activeShader->setBool("useEmissiveMap", false);

                    // 绑定基础纹理到纹理单元0
                    if (!modelObjects.empty() && !modelObjects[0]->GetTextures().empty())
                    {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, modelObjects[0]->GetTextures()[0].id);
                        activeShader->setInt("albedoMap", 0);
                    }
                }

                // ===== 修改：使用遮挡剔除结果渲染 =====
                unsigned int renderedEntities = 0;
                unsigned int totalEntities = entities.size();

                if (occlusionCullingEnabled && occlusionCulling)
                {
                    // 使用遮挡剔除结果
                    auto visibleIds = occlusionCulling->GetVisibleEntities();
                    for (int id : visibleIds)
                    {
                        auto entity = GetEntity(id);
                        if (entity)
                        {
                            entity->Render(*activeShader, currentShaderType);
                            renderedEntities++;
                        }
                    }
                }
                else
                {
                    // 回退到视锥体剔除
                    for (auto &pair : entities)
                    {
                        auto entity = pair.second;
                        if (entity->boundingVolume &&
                            entity->boundingVolume->isOnFrustum(frustum, entity->transform))
                        {
                            entity->Render(*activeShader, currentShaderType);
                            renderedEntities++;
                        }
                    }
                }

                // 每1000帧打印一次剔除统计
                static int frameCount = 0;
                frameCount++;
                if (frameCount % 10000 == 0)
                {
                    float visibilityRate = totalEntities > 0 ? (float)renderedEntities / totalEntities * 100.0f : 0.0f;
                    std::cout << "Frustum culling: " << renderedEntities << "/" << totalEntities
                              << " entities visible (" << visibilityRate << "%)" << std::endl;
                }
            }

            // 渲染线条
            if (lineRenderer && lineRendererInitialized)
            {
                lineRenderer->Render(view, projection);
            }

            // 调用渲染回调
            if (renderCallback)
            {
                renderCallback();
            }
        }

        // 渲染测试三角形
        testTriangleTime += deltaTime;
        if (testTriangleEnabled)
            RenderTestTriangle();
    }

    bool Engine::InitShaders()
    {
        try
        {
            LOGI("Engine", "Android Engine::InitShaders started");

            // 根据当前着色器类型只初始化对应的着色器
            if (currentShaderType == SHADER_SIMPLE)
            {
                std::string vsPath = config.internalStoragePath + "/model_loading_es.vs";
                std::string fsPath = config.internalStoragePath + "/model_loading_es.fs";

                LOGI("Engine", "Loading SIMPLE shader from: %s and %s", vsPath.c_str(), fsPath.c_str());

                // 检查文件是否存在
                FILE *vsFile = fopen(vsPath.c_str(), "r");
                FILE *fsFile = fopen(fsPath.c_str(), "r");

                if (!vsFile || !fsFile)
                {
                    if (vsFile)
                        fclose(vsFile);
                    if (fsFile)
                        fclose(fsFile);
                    LOGE("Engine", "Simple shader files not found");
                    return false;
                }

                fclose(vsFile);
                fclose(fsFile);

                // 初始化简单纹理着色器
                shaderSimple = std::make_unique<Shader>(vsPath.c_str(), fsPath.c_str(), true);
                shaderPBR = nullptr; // 确保PBR着色器为空

                if (shaderSimple && shaderSimple->ID)
                {
                    LOGI("Engine", "Simple shader created successfully, ID: %d", shaderSimple->ID);
                    shaderSimple->use();
                    shaderSimple->setInt("texture_diffuse1", 0);
                    shaderSimple->setBool("hasTexture", false);
                }
            }
            else if (currentShaderType == SHADER_PBR)
            {
                std::string pbrVsPath = config.internalStoragePath + "/pbr.vert";
                std::string pbrFsPath = config.internalStoragePath + "/pbr.frag";

                LOGI("Engine", "Loading PBR shader from: %s and %s", pbrVsPath.c_str(), pbrFsPath.c_str());

                // 检查文件是否存在
                FILE *pbrVsFile = fopen(pbrVsPath.c_str(), "r");
                FILE *pbrFsFile = fopen(pbrFsPath.c_str(), "r");

                if (!pbrVsFile || !pbrFsFile)
                {
                    if (pbrVsFile)
                        fclose(pbrVsFile);
                    if (pbrFsFile)
                        fclose(pbrFsFile);
                    LOGE("Engine", "PBR shader files not found");
                    return false;
                }

                fclose(pbrVsFile);
                fclose(pbrFsFile);

                // 初始化PBR着色器
                shaderPBR = std::make_unique<Shader>(pbrVsPath.c_str(), pbrFsPath.c_str(), true);
                shaderSimple = nullptr; // 确保简单着色器为空

                if (shaderPBR && shaderPBR->ID)
                {
                    LOGI("Engine", "PBR shader created successfully, ID: %d", shaderPBR->ID);
                    shaderPBR->use();
                    shaderPBR->setInt("albedoMap", 0);
                    shaderPBR->setInt("normalMap", 1);
                    shaderPBR->setInt("metallicMap", 2);
                    shaderPBR->setInt("roughnessMap", 3);
                    shaderPBR->setInt("aoMap", 4);
                    shaderPBR->setInt("emissiveMap", 5);
                }
            }

            return true;
        }
        catch (const std::exception &e)
        {
            LOGE("Engine", "Failed to load shaders: %s", e.what());
            return false;
        }
    }

    bool Engine::InitLineRenderer()
    {
        if (!lineRenderer)
        {
            lineRenderer = std::make_unique<LineRenderer>();
            lineRendererInitialized = lineRenderer->Initialize();
        }
        return lineRendererInitialized;
    }

    void Engine::RenderLines()
    {
        if (lineRenderer && lineRendererInitialized && camera)
        {
            glm::mat4 view = camera->GetViewMatrix();
            glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)config.width / (float)config.height,
                                                    0.1f, 100.0f);
            lineRenderer->Render(view, projection);
        }
    }
    int Engine::AddEntity(std::shared_ptr<Entity> entity)
    {
        int id = nextEntityId++;
        entities[id] = entity;
        // 自动注册到遮挡剔除系统
        if (occlusionCulling)
        {
            occlusionCulling->RegisterEntity(entity);
        }
        LOGI("Engine", "Entity added with ID: %d", id);
        return id;
    }

    void Engine::RemoveEntity(int id)
    {
        auto it = entities.find(id);
        if (it != entities.end())
        {
            // 在移除实体前，先从遮挡剔除系统注销
            if (occlusionCulling)
            {
                occlusionCulling->UnregisterEntity(it->second->GetId());
            }
            entities.erase(it);
            LOGI("Engine", "Entity removed, ID: %d", id);
        }
    }

    void Engine::ClearEntities()
    {
        // 清除遮挡剔除系统的注册
        if (occlusionCulling)
        {
            occlusionCulling->ClearEntities();
        }
        entities.clear();
        LOGI("Engine", "All entities cleared");
    }

    std::shared_ptr<Entity> Engine::GetEntity(int id)
    {
        auto it = entities.find(id);
        if (it != entities.end())
        {
            return it->second;
        }
        return nullptr;
    }

    bool Engine::initTestTriangle()
    {
        LOGI("Engine", "Initializing test triangle...");

        const char *vertexShaderSource =
            "#version 300 es\n"
            "layout (location = 0) in vec3 aPos;\n"
            "uniform mat4 uMVP;\n"
            "void main() {\n"
            "    gl_Position = uMVP * vec4(aPos, 1.0);\n"
            "}\n";

        const char *fragmentShaderSource =
            "#version 300 es\n"
            "precision mediump float;\n"
            "out vec4 FragColor;\n"
            "void main() {\n"
            "    FragColor = vec4(1.0, 1.0, 0.0, 1.0);\n"
            "}\n";

        // 编译顶点着色器
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            LOGI("Engine", "Test vertex shader compilation failed: %s", infoLog);
            return false;
        }

        // 编译片段着色器
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            LOGI("Engine", "Test fragment shader compilation failed: %s", infoLog);
            return false;
        }

        // 链接着色器程序
        testShaderProgram = glCreateProgram();
        glAttachShader(testShaderProgram, vertexShader);
        glAttachShader(testShaderProgram, fragmentShader);
        glLinkProgram(testShaderProgram);

        glGetProgramiv(testShaderProgram, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(testShaderProgram, 512, NULL, infoLog);
            LOGI("Engine", "Test shader program linking failed: %s", infoLog);
            return false;
        }

        testMVPLoc = glGetUniformLocation(testShaderProgram, "uMVP");

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        // 设置三角形顶点数据
        float vertices[] = {
            -0.5f, -0.5f, 0.0f,
            0.5f, -0.5f, 0.0f,
            0.0f, 0.5f, 0.0f};

        glGenVertexArrays(1, &testVAO);
        glGenBuffers(1, &testVBO);

        glBindVertexArray(testVAO);
        glBindBuffer(GL_ARRAY_BUFFER, testVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        LOGI("Engine", "Test triangle initialized successfully");
        return true;
    }

    void Engine::RenderTestTriangle()
    {
        if (!testTriangleEnabled || testShaderProgram == 0 || testVAO == 0)
        {
            return;
        }

        // 简单的旋转
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, testTriangleTime * 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 3.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f), (float)config.width / (float)config.height,
            0.1f, 100.0f);

        glm::mat4 mvp = projection * view * model;

        glUseProgram(testShaderProgram);
        glUniformMatrix4fv(testMVPLoc, 1, GL_FALSE, glm::value_ptr(mvp));

        glBindVertexArray(testVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindVertexArray(0);
        glUseProgram(0);
    }

    void Engine::HandleTouchEvent(int action, float x, float y)
    {
        if (!camera)
            return;

        static float lastTouchX = 0;
        static float lastTouchY = 0;
        static bool firstTouch = true;
        static bool isDragging = false;

        switch (action)
        {
        case 0: // ACTION_DOWN
            firstTouch = true;
            isDragging = true;
            lastTouchX = x;
            lastTouchY = y;
            break;

        case 1: // ACTION_UP
        case 3: // ACTION_CANCEL
            isDragging = false;
            firstTouch = true;
            break;

        case 2: // ACTION_MOVE
            if (!isDragging)
                return;

            if (firstTouch)
            {
                lastTouchX = x;
                lastTouchY = y;
                firstTouch = false;
                return;
            }

            float xoffset = (x - lastTouchX) * config.touchSensitivity;
            float yoffset = (y - lastTouchY) * config.touchSensitivity;

            if (fabs(xoffset) < 0.5f && fabs(yoffset) < 0.5f)
            {
                return;
            }

            camera->ProcessMouseMovement(xoffset, yoffset);

            lastTouchX = x;
            lastTouchY = y;
            break;
        }
    }

    void Engine::HandlePinchEvent(float scaleFactor)
    {
        if (!camera)
            return;

        const float MAX_SCALE_FACTOR = 1.5f;
        const float MIN_SCALE_FACTOR = 0.5f;

        if (scaleFactor > MAX_SCALE_FACTOR)
            scaleFactor = MAX_SCALE_FACTOR;
        if (scaleFactor < MIN_SCALE_FACTOR)
            scaleFactor = MIN_SCALE_FACTOR;

        float zoomDelta = (scaleFactor - 1.0f) * 20.0f;
        camera->ProcessMouseScroll(zoomDelta);
    }

    void Engine::HandleRotateEvent(float angle)
    {
        // 可选的旋转处理
    }
    bool Engine::LoadSkybox(const std::vector<std::string> &faces)
    {
        LOGI("Engine", "Loading skybox with %zu faces", faces.size());

        if (!skybox)
        {
            skybox = std::make_unique<Skybox>();
        }

        bool result = skybox->Initialize(faces);
        if (result)
        {
            LOGI("Engine", "Skybox loaded successfully");
            skyboxEnabled = true;
        }
        else
        {
            LOGE("Engine", "Failed to load skybox");
        }

        return result;
    }
    void Engine::HandleKeyEvent(int keyCode, int action)
    {
        if (!camera)
            return;

        float dt = deltaTime > 0 ? deltaTime : 0.016f;
        bool isDown = (action == 0);

        if (!isDown)
            return;

        switch (keyCode)
        {
        case 19:
        case 51: // DPAD_UP / W
            camera->ProcessKeyboard(FORWARD, dt);
            break;
        case 20:
        case 47: // DPAD_DOWN / S
            camera->ProcessKeyboard(BACKWARD, dt);
            break;
        case 21:
        case 29: // DPAD_LEFT / A
            camera->ProcessKeyboard(LEFT, dt);
            break;
        case 22:
        case 32: // DPAD_RIGHT / D
            camera->ProcessKeyboard(RIGHT, dt);
            break;
        case 24: // VOLUME_UP
            camera->ProcessMouseScroll(1.0f);
            break;
        case 25: // VOLUME_DOWN
            camera->ProcessMouseScroll(-1.0f);
            break;
        }
    }

    void Engine::SetTouchSensitivity(float sensitivity)
    {
        config.touchSensitivity = sensitivity;
    }

    // // 通用方法实现
    // void Engine::AddRenderObject(std::shared_ptr<RenderObject> obj) {
    //     renderObjects.push_back(obj);
    // }

    // void Engine::RemoveRenderObject(std::shared_ptr<RenderObject> obj) {
    //     renderObjects.erase(
    //         std::remove(renderObjects.begin(), renderObjects.end(), obj),
    //         renderObjects.end()
    //     );
    // }

    // void Engine::ClearRenderObjects() {
    //     renderObjects.clear();
    // }

    void Engine::SetUpdateCallback(std::function<void(float)> callback)
    {
        updateCallback = callback;
    }

    void Engine::SetRenderCallback(std::function<void()> callback)
    {
        renderCallback = callback;
    }

    void Engine::SetCameraPosition(const glm::vec3 &pos)
    {
        if (camera)
        {
            camera->Position = pos;
        }
    }

    void Engine::SetCameraZoom(float zoom)
    {
        if (camera)
        {
            camera->Zoom = zoom;
        }
    }

    unsigned int Engine::LoadTexture(const std::string &path)
    {
        unsigned int textureID;
        glGenTextures(1, &textureID);

        LOGI("Engine", "Loading texture from: %s", path.c_str());

        FILE *file = fopen(path.c_str(), "rb");
        if (!file)
        {
            LOGE("Engine", "Texture file does not exist: %s", path.c_str());
            return 0;
        }
        fclose(file);

        int width, height, nrComponents;
        unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrComponents, STBI_rgb_alpha);

        if (data)
        {
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
            return textureID;
        }
        else
        {
            LOGE("Engine", "Texture failed to load at path: %s", path.c_str());
            return 0;
        }
    }

    std::shared_ptr<ModelObject> Engine::LoadModel(const std::string &path)
    {
        return std::make_shared<ModelObject>(path);
    }

    int Engine::AddLight(const Light &light)
    {
        if (lights.size() >= maxLights)
        {
            LOGE("Engine", "Cannot add more lights, max lights reached: %d", maxLights);
            return -1;
        }
        lights.push_back(light);
        LOGI("Engine", "Light added, total lights: %zu", lights.size());
        return lights.size() - 1;
    }

    void Engine::RemoveLight(int index)
    {
        if (index >= 0 && index < lights.size())
        {
            lights.erase(lights.begin() + index);
            LOGI("Engine", "Light removed, remaining lights: %zu", lights.size());
        }
    }

    void Engine::ClearLights()
    {
        lights.clear();
        LOGI("Engine", "All lights cleared");
    }

    Light &Engine::GetLight(int index)
    {
        static Light defaultLight;
        if (index >= 0 && index < lights.size())
        {
            return lights[index];
        }
        return defaultLight;
    }

    void Engine::UpdateLight(int index, const Light &light)
    {
        if (index >= 0 && index < lights.size())
        {
            lights[index] = light;
        }
    }

    void Engine::SetMaxLights(int max)
    {
        maxLights = max;
        lights.reserve(maxLights);
    }
    // // 在文件末尾添加

    // int Engine::AddPrimitive(std::shared_ptr<Primitive> primitive) {
    //     int id = nextPrimitiveId++;
    //     primitives[id] = primitive;
    //     LOGI("Engine", "Primitive added with ID: %d", id);
    //     return id;
    // }

    // void Engine::RemovePrimitive(int id) {
    //     auto it = primitives.find(id);
    //     if (it != primitives.end()) {
    //         primitives.erase(it);
    //         LOGI("Engine", "Primitive removed, ID: %d", id);
    //     }
    // }

    // void Engine::ClearPrimitives() {
    //     primitives.clear();
    //     LOGI("Engine", "All primitives cleared");
    // }

    // std::shared_ptr<Primitive> Engine::GetPrimitive(int id) {
    //     auto it = primitives.find(id);
    //     if (it != primitives.end()) {
    //         return it->second;
    //     }
    //     return nullptr;
    // }

} // namespace Engine

#endif // PLATFORM_ANDROID