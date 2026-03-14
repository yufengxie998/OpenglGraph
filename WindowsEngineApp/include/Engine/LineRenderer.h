/**
 * @file LineRenderer.h
 * @brief 线条渲染器类的头文件
 * 
 * 该文件定义了LineRenderer类，用于实现各种线条的绘制功能。
 * 支持实线、虚线、贝塞尔曲线、样条曲线、正弦曲线、多项式曲线等，
 * 提供线条生命周期管理和批量渲染功能。
 * 
 * @author 谢兴国   1452492081@qq.com
 * @version 1.0
 * @date 2025-10-12
 * 
 * @note 支持线条样式自定义和生命周期管理，顶点缓冲区动态更新
 */
#ifndef _H_LINE_RENDERER_H
#define _H_LINE_RENDERER_H

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#ifdef PLATFORM_ANDROID
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif
#include "dataType.h"

// 线段结构体
struct Line {
    glm::vec3 start;
    glm::vec3 end;
    glm::vec3 color;
    float lifetime;
    float remainingLife;
    LineStyle style;           // 线段样式
    float dashLength;          // 虚线长度
    float gapLength;           // 间隙长度
    bool isVisible;            // 是否可见
    
    Line(const glm::vec3& s, const glm::vec3& e, const glm::vec3& c, 
         float life, LineStyle sty = LineStyle::SOLID,
         float dash = 0.1f, float gap = 0.05f)
        : start(s), end(e), color(c), lifetime(life), remainingLife(life),
          style(sty), dashLength(dash), gapLength(gap), isVisible(true) {}
};

// 顶点数据结构（用于批量渲染）
struct LineVertex {
    glm::vec3 position;
    glm::vec3 color;
    
    LineVertex() = default;
    LineVertex(const glm::vec3& pos, const glm::vec3& col) 
        : position(pos), color(col) {}
};

class LineRenderer {
public:
    LineRenderer();
    ~LineRenderer();
    
    bool Initialize();
    void Shutdown();
    void Update(float deltaTime);
    void Render(const glm::mat4& view, const glm::mat4& projection);
    void Clear();
    
    // 添加实线
    void AddLine(const glm::vec3& start, const glm::vec3& end, 
                 const glm::vec3& color, float lifetime = -1.0f);
    
    // 添加虚线
    void AddDashedLine(const glm::vec3& start, const glm::vec3& end,
                       const glm::vec3& color, float dashLength = 0.1f, 
                       float gapLength = 0.05f, float lifetime = -1.0f);
    
    // 添加带样式的线
    void AddStyledLine(const glm::vec3& start, const glm::vec3& end,
                       const glm::vec3& color, LineStyle style,
                       float dashLength = 0.1f, float gapLength = 0.05f,
                       float lifetime = -1.0f);
    
    // 设置默认虚线参数
    void SetDefaultDashParams(float dashLength, float gapLength);
    
    // 获取线条数量
    size_t GetLineCount() const { return lines.size(); }
    
    // 设置线宽
    void SetLineWidth(float width);
    
    // 贝塞尔曲线
    void AddBezierCurve(const glm::vec3& p0, const glm::vec3& p1,
                        const glm::vec3& p2, const glm::vec3& p3,
                        const glm::vec3& color, LineStyle style = LineStyle::SOLID,
                        int segments = 20, float lifetime = -1.0f);
    
    // 圆形
    void AddCircle(const glm::vec3& center, float radius, const glm::vec3& color,
                   LineStyle style = LineStyle::SOLID, int segments = 32,
                   float lifetime = -1.0f);

                   // 样条曲线（Catmull-Rom样条）
void  AddSplineCurve(const std::vector<glm::vec3>& points,
                                  const glm::vec3& color, int segmentsPerSegment,
                                  float lifetime) ;
void AddSineWave(const glm::vec3& start, const glm::vec3& direction,
                                    float amplitude, float frequency, float length,
                                    const glm::vec3& color, int segments, float lifetime);
    
    // 多项式曲线
void AddPolynomialCurve(const glm::vec3& startPoint,
    const glm::vec3& direction,
    const glm::vec3& upDirection,
    float c0, float c1, float c2, float c3,
    float xMin, float xMax,
    const glm::vec3& color,
    int segments, float lifetime);
    // 多项式曲线（局部坐标系版本）
void AddPolynomialCurveLocal(const glm::vec3& center,
    const glm::vec3& xAxis,
    const glm::vec3& yAxis,
    const glm::vec3& zAxis,
    float c0, float c1, float c2, float c3,
    float xMin, float xMax,
    const glm::vec3& color,
    int segments, float lifetime);
    // 强制更新顶点缓冲区
    void UpdateVertexBuffer();

private:
    // 将虚线线段分解为多个实线线段
    std::vector<std::pair<glm::vec3, glm::vec3>> GenerateDashedSegments(
        const glm::vec3& start, const glm::vec3& end, 
        float dashLength, float gapLength);
    
    // 重建顶点缓冲区
    void RebuildVertexBuffer();
    // 计算多项式值的辅助函数
float EvaluatePolynomial(float x, float c0, float c1, float c2, float c3) {
    // y = c0 + c1*x + (c2/2)*x^2 + (c3/6)*x^3
    float x2 = x * x;
    float x3 = x2 * x;
    return c0 + c1 * x + (c2 / 2.0f) * x2 + (c3 / 6.0f) * x3;
}
    // 成员变量
    std::vector<Line> lines;
    
    GLuint VAO, VBO;
    GLuint shaderProgram;
    GLint viewLoc, projLoc;
    float lineWidth;
    
    // 默认虚线参数
    float defaultDashLength;
    float defaultGapLength;
    
    // 批量渲染数据
    std::vector<LineVertex> vertices;
    bool vertexBufferDirty;
    size_t maxVertices;
    
    // 着色器源码
    const char* vertexShaderSource;
    const char* fragmentShaderSource;
};

#endif // LINE_RENDERER_H