/**
 * @file Cylinder.h
 * @brief 圆柱体几何体类的头文件
 * 
 * 该文件定义了Cylinder类，用于生成和管理圆柱体几何体。
 * 支持自定义半径、高度和分段数，自动生成顶点数据、法线、
 * 纹理坐标、切线/副切线以及索引缓冲区。
 * 
 * @author 谢兴国   1452492081@qq.com
 * @version 1.0
 * @date 2025-10-12
 * 
 * @note 继承自Primitive基类，包含完整的几何体生成逻辑
 */
#ifndef CYLINDER_H
#define CYLINDER_H

#include "Primitive.h"
namespace Geometry{
class Cylinder : public Primitive {
public:
    Cylinder(float radius = 1.0f, float height = 2.0f, int sectors = 32);
    virtual ~Cylinder();
    // 实现 Entity 的纯虚函数
    virtual void Update(float deltaTime) override
    {
        Primitive::Update(deltaTime);
    }

    virtual void Render(Shader &shader, ShaderType shaderType) override
    {
        shader.setMat4("model", transform.getModelMatrix());
        Draw(shader.ID, shaderType);
    }
    void Draw(GLuint shaderProgram, ShaderType shaderType) override;
    void SetRadius(float r);
    void SetHeight(float h);
    void SetSectors(int s);

private:
    void GenerateGeometry();
    void GenerateCap(bool top);
    
    float radius;
    float height;
    int sectors;
};
}
#endif // CYLINDER_H