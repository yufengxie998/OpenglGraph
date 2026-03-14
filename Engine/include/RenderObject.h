#ifndef _H_RENDEROBJECT_H_
#define _H_RENDEROBJECT_H_
#include "shader.h"
#include "dataType.h"
namespace Engine
{
    // 渲染对象基类
    class RenderObject
    {
    public:
        virtual ~RenderObject() = default;
        virtual void Update(float deltaTime) = 0;
        virtual void Render(Shader &shader, ShaderType shaderType) = 0;
        virtual void SetPosition(const glm::vec3 &pos) = 0;
        virtual void SetRotation(const glm::vec3 &axis, float angle) = 0;
        virtual void SetScale(const glm::vec3 &scale) = 0;
        glm::mat4 GetModelMatrix() const { return modelMatrix; }

    protected:
        glm::mat4 modelMatrix = glm::mat4(1.0f);
    };
}

#endif