/**
 * @file Plane.h
 * @brief 平面几何体类的头文件
 * 
 * 该文件定义了Plane类，继承自Primitive，用于生成和管理平面几何体。
 * 支持自定义宽度、深度和分段数，自动生成顶点数据、法线、纹理坐标等。
 * 
 * @author 谢兴国   1452492081@qq.com
 * @version 1.0
 * @date 2025-10-12
 * 
 * @note 继承自Primitive基类，位于Geometry命名空间中
 */
#ifndef PLANE_H
#define PLANE_H

#include "Primitive.h"
namespace Geometry{
class Plane : public Primitive {
public:
    Plane(float width = 10.0f, float depth = 10.0f, int segments = 1);
    virtual ~Plane();
        // 实现 Entity 的纯虚函数
        virtual void Update(float deltaTime) override {
            Primitive::Update(deltaTime); // 调用基类的Update
        }
        
        virtual void Render(Shader &shader, ShaderType shaderType) override {
            // 设置模型矩阵
            shader.setMat4("model", transform.getModelMatrix());
            // 调用绘制方法
            Draw(shader.ID, shaderType);
        }
    void Draw(GLuint shaderProgram, ShaderType shaderType) override;
    void SetSize(float width, float depth);
    void SetSegments(int segments);

private:
    void GenerateGeometry();
    
    float width;
    float depth;
    int segments;
};
}
#endif // PLANE_H