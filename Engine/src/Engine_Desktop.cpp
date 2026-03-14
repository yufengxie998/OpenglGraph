#ifndef PLATFORM_ANDROID

#include "Engine.h"
#include "LineRenderer.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// #include "entity.h"

// STB Image 实现
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace FrustumMath;
namespace Engine
{

    // 静态成员初始化
    Engine *Engine::s_instance = nullptr;
    float Engine::deltaTime = 0.0f;
    float Engine::lastFrame = 0.0f;
    float Engine::lastX = 400;
    float Engine::lastY = 300;
    bool Engine::firstMouse = true;

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
        std::cout << "Initializing Engine for Desktop" << std::endl;

        this->config = config;

        // 初始化 GLFW
        if (!glfwInit())
        {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(config.width, config.height, config.title.c_str(), NULL, NULL);
        if (!window)
        {
            std::cerr << "Failed to create GLFW window" << std::endl;
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
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            return false;
        }

        // 初始化 OpenGL 状态
        if (config.enableDepthTest)
        {
            glEnable(GL_DEPTH_TEST);
        }

        if (config.enableWireframe)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }

        // 设置纹理翻转
        stbi_set_flip_vertically_on_load(true);

        // 初始化摄像机
        camera = std::make_unique<Camera>(config.cameraPosition);

        // 初始化着色器
        if (!InitShaders())
        {
            std::cerr << "Failed to initialize shaders" << std::endl;
            return false;
        }

        // 初始化线条渲染器
        if (config.enableLineRenderer)
        {
            InitLineRenderer();
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
        std::cout << "Engine initialized successfully" << std::endl;
        return true;
    }

    void Engine::Shutdown()
    {
        std::cout << "Shutting down Engine" << std::endl;

        if (lineRenderer)
        {
            lineRenderer->Shutdown();
            lineRenderer.reset();
        }

        entities.clear();
        camera.reset();
        shader.reset();

        if (window)
        {
            glfwDestroyWindow(window);
            window = nullptr;
        }

        glfwTerminate();
    }

    void Engine::Run()
    {
        isRunning = true;

        while (isRunning && !glfwWindowShouldClose(window))
        {
            float currentFrame = glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            glfwPollEvents();

            // 清除缓冲区
            glClearColor(config.clearColor[0], config.clearColor[1],
                         config.clearColor[2], config.clearColor[3]);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // 先渲染天空盒（如果启用）
            if (skybox && skyboxEnabled && camera)
            {
                glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom),
                                                        (float)config.width / (float)config.height, 0.1f, 1000.0f);
                glm::mat4 view = camera->GetViewMatrix();
                skybox->Render(view, projection);
            }

            // 更新回调
            if (updateCallback)
            {
                updateCallback(deltaTime);
            }
            // 递归更新所有Entity（包括它们的变换和特定逻辑）
            for (auto &pair : entities)
            {
                if (pair.second)
                {
                    pair.second->Update(deltaTime); // 调用每个Entity的Update方法
                }
            }

            // 创建视锥体
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

            // 渲染 - 根据当前着色器类型使用对应的着色器
            Shader *activeShader = nullptr;

            if (currentShaderType == SHADER_SIMPLE)
            {
                activeShader = shaderSimple.get();
            }
            else
            {
                activeShader = shaderPBR.get();
            }

            if (activeShader && activeShader->ID)
            {
                activeShader->use();

                glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom),
                                                        (float)config.width / (float)config.height,
                                                        0.1f, 100.0f);
                glm::mat4 view = camera->GetViewMatrix();

                // 设置通用 uniforms
                activeShader->setMat4("projection", projection);
                activeShader->setMat4("view", view);

                // 根据着色器类型设置特定的 uniforms
                if (currentShaderType == SHADER_SIMPLE)
                {
                    static bool logged = false;
                    if (!logged)
                    {
                        std::cout << "Engine: Using SIMPLE shader" << std::endl;
                        logged = true;
                    }
                    // activeShader->setBool("hasTexture", ...);
                }
                else if (currentShaderType == SHADER_PBR)
                {
                    static bool logged = false;
                    if (!logged)
                    {
                        std::cout << "Engine: Using PBR shader" << std::endl;
                        logged = true;
                    }
                    // PBR着色器需要设置额外的 uniforms
                    activeShader->setVec3("viewPos", camera->Position);

                    // 设置光源

                    // 上传光源数据到着色器
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

                // 渲染回调
                if (renderCallback)
                {
                    renderCallback();
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
                if (frameCount % 5000 == 0)
                {
                    float visibilityRate = totalEntities > 0 ? (float)renderedEntities / totalEntities * 100.0f : 0.0f;
                    std::cout << "Frustum culling: " << renderedEntities << "/" << totalEntities
                              << " entities visible (" << visibilityRate << "%)" << std::endl;
                }
            }

            RenderLines();
            glfwSwapBuffers(window);
        }
    }

    void Engine::Stop()
    {
        isRunning = false;
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

    void Engine::RenderFrame(float deltaTime)
    {
        // 桌面平台不使用此方法
        return;
    }

    bool Engine::InitShaders()
    {
        try
        {
            std::cout << "PC Engine::InitShaders started" << std::endl;

            // 根据当前着色器类型只初始化对应的着色器
            if (currentShaderType == SHADER_SIMPLE)
            {
                std::cout << "Loading SIMPLE shader" << std::endl;
                shaderSimple = std::make_unique<Shader>("1.model_loading.vs", "1.model_loading.fs", true);
                shaderPBR = nullptr;

                if (shaderSimple && shaderSimple->ID)
                {
                    shaderSimple->use();
                    shaderSimple->setInt("texture_diffuse1", 0);
                    shaderSimple->setBool("hasTexture", false);
                    std::cout << "Simple shader created successfully, ID: " << shaderSimple->ID << std::endl;
                }
            }
            else if (currentShaderType == SHADER_PBR)
            {
                std::cout << "Loading PBR shader" << std::endl;
                shaderPBR = std::make_unique<Shader>("pbr.vert", "pbr.frag", true);
                shaderSimple = nullptr;

                if (shaderPBR && shaderPBR->ID)
                {
                    shaderPBR->use();
                    shaderPBR->setInt("albedoMap", 0);
                    shaderPBR->setInt("normalMap", 1);
                    shaderPBR->setInt("metallicMap", 2);
                    shaderPBR->setInt("roughnessMap", 3);
                    shaderPBR->setInt("aoMap", 4);
                    shaderPBR->setInt("emissiveMap", 5);
                    shaderPBR->setInt("lightCount", 0);
                    std::cout << "PBR shader created successfully, ID: " << shaderPBR->ID << std::endl;
                }
            }

            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Failed to load shaders: " << e.what() << std::endl;
            return false;
        }
    }

    bool Engine::InitLineRenderer()
    {
        if (!lineRenderer)
        {
            lineRenderer = std::make_unique<LineRenderer>();
            lineRendererInitialized = lineRenderer->Initialize();
            if (lineRendererInitialized)
            {
                std::cout << "LineRenderer initialized successfully!" << std::endl;
            }
            else
            {
                std::cerr << "Failed to initialize LineRenderer!" << std::endl;
            }
        }
        return lineRendererInitialized;
    }

    void Engine::RenderLines()
    {
        if (lineRenderer && lineRendererInitialized && camera)
        {
            glm::mat4 view = camera->GetViewMatrix();
            glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom),
                                                    (float)config.width / (float)config.height,
                                                    0.1f, 100.0f);
            lineRenderer->Render(view, projection);
        }
    }

    // GLFW 回调函数
    void Engine::FramebufferSizeCallback(GLFWwindow *window, int width, int height)
    {
        glViewport(0, 0, width, height);
        if (s_instance)
        {
            s_instance->config.width = width;
            s_instance->config.height = height;
        }
    }

    void Engine::MouseCallback(GLFWwindow *window, double xposIn, double yposIn)
    {
        if (!s_instance || !s_instance->camera)
            return;

        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (firstMouse)
        {
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

    void Engine::ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
    {
        if (s_instance && s_instance->camera)
        {
            s_instance->camera->ProcessMouseScroll(static_cast<float>(yoffset));
        }
    }

    void Engine::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        // Tab键：切换鼠标模式
        static bool tabPressed = false;
        if (key == GLFW_KEY_TAB && action == GLFW_PRESS && !tabPressed)
        {
            tabPressed = true;

            int mode = glfwGetInputMode(window, GLFW_CURSOR);
            if (mode == GLFW_CURSOR_DISABLED)
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                firstMouse = true;
                std::cout << "UI Mode: Mouse is free for clicking" << std::endl;
            }
            else
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                firstMouse = true;
                std::cout << "3D Mode: Mouse controls camera" << std::endl;
            }
        }

        if (key == GLFW_KEY_TAB && action == GLFW_RELEASE)
        {
            tabPressed = false;
        }

        // 摄像机键盘控制
        if (s_instance && s_instance->camera && action == GLFW_PRESS)
        {
            float dt = deltaTime;
            if (key == GLFW_KEY_W)
                s_instance->camera->ProcessKeyboard(FORWARD, dt);
            if (key == GLFW_KEY_S)
                s_instance->camera->ProcessKeyboard(BACKWARD, dt);
            if (key == GLFW_KEY_A)
                s_instance->camera->ProcessKeyboard(LEFT, dt);
            if (key == GLFW_KEY_D)
                s_instance->camera->ProcessKeyboard(RIGHT, dt);
        }
    }

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

        int width, height, nrComponents;
        unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);

        if (data)
        {
            GLenum format;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;
            else
                format = GL_RGB;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
            std::cout << "Texture loaded successfully: " << path << std::endl;
            return textureID;
        }
        else
        {
            std::cerr << "Texture failed to load at path: " << path << std::endl;
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

} // namespace Engine

#endif // !PLATFORM_ANDROID