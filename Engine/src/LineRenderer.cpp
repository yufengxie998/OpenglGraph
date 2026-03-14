#include "LineRenderer.h"
#include <extend/logger.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

#ifdef PLATFORM_ANDROID
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

LineRenderer::LineRenderer()
    : VAO(0), VBO(0), shaderProgram(0), viewLoc(-1), projLoc(-1),
      lineWidth(1.0f), defaultDashLength(0.1f), defaultGapLength(0.05f),
      vertexBufferDirty(true), maxVertices(0)
{

    // 初始化着色器源码
    vertexShaderSource =
        "#version 300 es\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aColor;\n"
        "out vec3 Color;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    Color = aColor;\n"
        "    gl_Position = projection * view * vec4(aPos, 1.0);\n"
        "}\n";

    fragmentShaderSource =
        "#version 300 es\n"
        "precision mediump float;\n"
        "in vec3 Color;\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = vec4(Color, 1.0);\n"
        "}\n";
}

LineRenderer::~LineRenderer()
{
    Shutdown();
}

bool LineRenderer::Initialize()
{
    LOGI("LineRenderer", "LineRenderer::Initialize started");

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
        LOGE("LineRenderer", "Vertex shader compilation failed: %s", infoLog);
        return false;
    }
    LOGI("LineRenderer", "Vertex shader compiled successfully");

    // 编译片段着色器
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        LOGE("LineRenderer", "Fragment shader compilation failed: %s", infoLog);
        return false;
    }
    LOGI("LineRenderer", "Fragment shader compiled successfully");

    // 链接着色器程序
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        LOGE("LineRenderer", "Shader program linking failed: %s", infoLog);
        return false;
    }

    // 获取uniform位置
    viewLoc = glGetUniformLocation(shaderProgram, "view");
    projLoc = glGetUniformLocation(shaderProgram, "projection");
    LOGI("LineRenderer", "Uniform locations - view: %d, proj: %d", viewLoc, projLoc);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 创建VAO和VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // 配置VAO
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void *)0);
    glEnableVertexAttribArray(0);

    // 颜色属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex),
                          (void *)offsetof(LineVertex, color));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    LOGI("LineRenderer", "LineRenderer::Initialize completed successfully");
    return true;
}

void LineRenderer::Shutdown()
{
    if (VAO)
        glDeleteVertexArrays(1, &VAO);
    if (VBO)
        glDeleteBuffers(1, &VBO);
    if (shaderProgram)
        glDeleteProgram(shaderProgram);
    lines.clear();
    vertices.clear();
}

void LineRenderer::Update(float deltaTime)
{
    bool needsRebuild = false;

    // 更新线条生命周期
    for (auto it = lines.begin(); it != lines.end();)
    {
        if (it->lifetime > 0)
        {
            it->remainingLife -= deltaTime;
            if (it->remainingLife <= 0)
            {
                it = lines.erase(it);
                needsRebuild = true;
                continue;
            }
        }
        ++it;
    }

    // 如果线条有变化，标记顶点缓冲区为脏
    if (needsRebuild)
    {
        vertexBufferDirty = true;
    }
}

void LineRenderer::Render(const glm::mat4 &view, const glm::mat4 &projection)
{
    if (lines.empty())
        return;

    // 如果需要，重建顶点缓冲区
    if (vertexBufferDirty)
    {
        RebuildVertexBuffer();
    }

    if (vertices.empty())
        return;

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(VAO);

    // 设置线宽
    glLineWidth(lineWidth);

    // 批量绘制所有线条
    glDrawArrays(GL_LINES, 0, vertices.size());

    glBindVertexArray(0);
    glUseProgram(0);
}

void LineRenderer::Clear()
{
    lines.clear();
    vertices.clear();
    vertexBufferDirty = true;
}

void LineRenderer::AddLine(const glm::vec3 &start, const glm::vec3 &end,
                           const glm::vec3 &color, float lifetime)
{
    lines.emplace_back(start, end, color, lifetime, LineStyle::SOLID);
    vertexBufferDirty = true;
    // LOGI("LineRenderer", "Line added - total lines: %zu", lines.size());
}

void LineRenderer::AddDashedLine(const glm::vec3 &start, const glm::vec3 &end,
                                 const glm::vec3 &color, float dashLength,
                                 float gapLength, float lifetime)
{
    lines.emplace_back(start, end, color, lifetime, LineStyle::DASHED,
                       dashLength, gapLength);
    vertexBufferDirty = true;
}

void LineRenderer::AddStyledLine(const glm::vec3 &start, const glm::vec3 &end,
                                 const glm::vec3 &color, LineStyle style,
                                 float dashLength, float gapLength, float lifetime)
{
    lines.emplace_back(start, end, color, lifetime, style, dashLength, gapLength);
    vertexBufferDirty = true;
}

void LineRenderer::SetDefaultDashParams(float dashLength, float gapLength)
{
    defaultDashLength = dashLength;
    defaultGapLength = gapLength;
}

void LineRenderer::SetLineWidth(float width)
{
    lineWidth = width;
}

void LineRenderer::UpdateVertexBuffer()
{
    if (vertexBufferDirty)
    {
        RebuildVertexBuffer();
    }
}

void LineRenderer::RebuildVertexBuffer()
{
    vertices.clear();

    // 为每条线生成顶点数据
    for (const auto &line : lines)
    {
        if (!line.isVisible)
            continue;

        if (line.style == LineStyle::SOLID)
        {
            // 实线：添加两个顶点（起点和终点）
            vertices.emplace_back(line.start, line.color);
            vertices.emplace_back(line.end, line.color);
        }
        else
        {
            // 虚线：分解为多个小线段
            auto segments = GenerateDashedSegments(line.start, line.end,
                                                   line.dashLength, line.gapLength);

            for (const auto &seg : segments)
            {
                vertices.emplace_back(seg.first, line.color);
                vertices.emplace_back(seg.second, line.color);
            }
        }
    }

    // 更新VBO
    if (!vertices.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        // 如果顶点数量超过当前缓冲区大小，重新分配
        size_t dataSize = vertices.size() * sizeof(LineVertex);
        if (vertices.size() > maxVertices)
        {
            glBufferData(GL_ARRAY_BUFFER, dataSize, vertices.data(), GL_DYNAMIC_DRAW);
            maxVertices = vertices.size();
        }
        else
        {
            // 更新现有缓冲区的一部分
            glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, vertices.data());
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    vertexBufferDirty = false;
    // LOGI("LineRenderer", "Vertex buffer rebuilt - %zu vertices for %zu lines",
    //      vertices.size(), lines.size());
}

std::vector<std::pair<glm::vec3, glm::vec3>> LineRenderer::GenerateDashedSegments(
    const glm::vec3 &start, const glm::vec3 &end, float dashLength, float gapLength)
{

    std::vector<std::pair<glm::vec3, glm::vec3>> segments;

    glm::vec3 direction = end - start;
    float totalLength = glm::length(direction);

    if (totalLength < 0.001f)
        return segments;

    glm::vec3 unitDir = direction / totalLength;

    float currentPos = 0.0f;
    bool drawDash = true;

    while (currentPos < totalLength)
    {
        float segmentStart = currentPos;
        float segmentEnd;

        if (drawDash)
        {
            segmentEnd = std::min(currentPos + dashLength, totalLength);

            glm::vec3 dashStart = start + unitDir * segmentStart;
            glm::vec3 dashEnd = start + unitDir * segmentEnd;

            segments.emplace_back(dashStart, dashEnd);

            currentPos = segmentEnd;
            drawDash = false;
        }
        else
        {
            // 跳过间隙
            currentPos = std::min(currentPos + gapLength, totalLength);
            drawDash = true;
        }
    }

    return segments;
}

void LineRenderer::AddBezierCurve(const glm::vec3 &p0, const glm::vec3 &p1,
                                  const glm::vec3 &p2, const glm::vec3 &p3,
                                  const glm::vec3 &color, LineStyle style,
                                  int segments, float lifetime)
{
    if (segments < 2)
        segments = 2;

    auto bezierPoint = [](const glm::vec3 &p0, const glm::vec3 &p1,
                          const glm::vec3 &p2, const glm::vec3 &p3, float t)
    {
        float u = 1.0f - t;
        float tt = t * t;
        float uu = u * u;
        float uuu = uu * u;
        float ttt = tt * t;

        return uuu * p0 + 3.0f * uu * t * p1 + 3.0f * u * tt * p2 + ttt * p3;
    };

    glm::vec3 prevPoint = p0;

    for (int i = 1; i <= segments; i++)
    {
        float t = static_cast<float>(i) / segments;
        glm::vec3 currentPoint = bezierPoint(p0, p1, p2, p3, t);

        if (style == LineStyle::SOLID)
        {
            AddLine(prevPoint, currentPoint, color, lifetime);
        }
        else
        {
            AddStyledLine(prevPoint, currentPoint, color, style,
                          defaultDashLength, defaultGapLength, lifetime);
        }

        prevPoint = currentPoint;
    }
}

void LineRenderer::AddCircle(const glm::vec3 &center, float radius, const glm::vec3 &color,
                             LineStyle style, int segments, float lifetime)
{
    if (segments < 3)
        segments = 3;

    glm::vec3 prevPoint = center + glm::vec3(radius, 0.0f, 0.0f);

    for (int i = 1; i <= segments; i++)
    {
        float angle = 2.0f * M_PI * static_cast<float>(i) / segments;
        glm::vec3 currentPoint = center + glm::vec3(radius * cos(angle), 0.0f, radius * sin(angle));

        if (style == LineStyle::SOLID)
        {
            AddLine(prevPoint, currentPoint, color, lifetime);
        }
        else
        {
            AddStyledLine(prevPoint, currentPoint, color, style,
                          defaultDashLength, defaultGapLength, lifetime);
        }

        prevPoint = currentPoint;
    }
}

// 样条曲线（Catmull-Rom样条）
void LineRenderer::AddSplineCurve(const std::vector<glm::vec3> &points,
                                  const glm::vec3 &color, int segmentsPerSegment,
                                  float lifetime)
{
    if (points.size() < 2)
    {
        LOGE("LineRenderer", "Need at least 2 points for spline curve");
        return;
    }

    if (points.size() == 2)
    {
        // 只有两个点，直接画直线
        AddLine(points[0], points[1], color, lifetime);
        return;
    }

    // 对于Catmull-Rom样条，我们需要在每个相邻点对之间插入曲线
    for (size_t i = 0; i < points.size() - 1; i++)
    {
        // 获取控制点
        glm::vec3 p0 = (i == 0) ? points[i] : points[i - 1];
        glm::vec3 p1 = points[i];
        glm::vec3 p2 = points[i + 1];
        glm::vec3 p3 = (i == points.size() - 2) ? points[i + 1] : points[i + 2];

        glm::vec3 prevPoint = p1;

        for (int j = 1; j <= segmentsPerSegment; j++)
        {
            float t = static_cast<float>(j) / segmentsPerSegment;

            // Catmull-Rom样条插值公式
            // q(t) = 0.5 * [(2*P1) + (-P0+P2)*t + (2*P0-5*P1+4*P2-P3)*t^2 + (-P0+3*P1-3*P2+P3)*t^3]
            float t2 = t * t;
            float t3 = t2 * t;

            glm::vec3 currentPoint = 0.5f * ((2.0f * p1) +
                                             (-p0 + p2) * t +
                                             (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                                             (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);

            // 添加线段
            AddLine(prevPoint, currentPoint, color, lifetime);
            prevPoint = currentPoint;
        }
    }

    LOGI("LineRenderer", "Added spline curve with %zu points", points.size());
}

// 正弦曲线
void LineRenderer::AddSineWave(const glm::vec3 &start, const glm::vec3 &direction,
                               float amplitude, float frequency, float length,
                               const glm::vec3 &color, int segments, float lifetime)
{
    if (segments < 5)
        segments = 5;

    // 归一化方向向量
    glm::vec3 dirNorm = glm::normalize(direction);

    // 计算垂直于方向的向量作为波动方向
    // 默认使用Y轴作为向上方向，然后计算垂直向量
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    // 如果方向与Y轴平行，改用Z轴
    if (fabs(glm::dot(dirNorm, up)) > 0.99f)
    {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    // 计算垂直于方向的向量
    glm::vec3 right = glm::normalize(glm::cross(dirNorm, up));
    glm::vec3 waveDir = glm::normalize(glm::cross(right, dirNorm));

    float step = length / segments;
    glm::vec3 prevPoint = start;

    for (int i = 1; i <= segments; i++)
    {
        float t = static_cast<float>(i) / segments;
        float x = t * length;
        float y = amplitude * sin(2.0f * M_PI * frequency * t);

        glm::vec3 currentPoint = start + dirNorm * x + waveDir * y;

        AddLine(prevPoint, currentPoint, color, lifetime);
        prevPoint = currentPoint;
    }

    LOGI("LineRenderer", "Added sine wave with %d segments", segments);
}

// 多项式曲线
void LineRenderer::AddPolynomialCurve(const glm::vec3 &startPoint,
                                      const glm::vec3 &direction,
                                      const glm::vec3 &upDirection,
                                      float c0, float c1, float c2, float c3,
                                      float xMin, float xMax,
                                      const glm::vec3 &color,
                                      int segments, float lifetime)
{
    if (segments < 2)
        segments = 2;
    if (xMax <= xMin)
    {
        LOGE("LineRenderer", "Invalid x range: [%f, %f]", xMin, xMax);
        return;
    }

    // 归一化方向向量
    glm::vec3 dirNorm = glm::normalize(direction);
    glm::vec3 upNorm = glm::normalize(upDirection);

    // 计算垂直于方向和平面的向量
    glm::vec3 rightNorm = glm::normalize(glm::cross(dirNorm, upNorm));

    // 重新计算上方向以确保正交
    glm::vec3 finalUp = glm::normalize(glm::cross(rightNorm, dirNorm));

    float step = (xMax - xMin) / segments;

    // 第一个点
    float x = xMin;
    float y = EvaluatePolynomial(x, c0, c1, c2, c3);
    glm::vec3 prevPoint = startPoint + dirNorm * x + finalUp * y;

    for (int i = 1; i <= segments; i++)
    {
        x = xMin + i * step;
        y = EvaluatePolynomial(x, c0, c1, c2, c3);

        glm::vec3 currentPoint = startPoint + dirNorm * x + finalUp * y;

        AddLine(prevPoint, currentPoint, color, lifetime);
        prevPoint = currentPoint;
    }

    LOGI("LineRenderer", "Added polynomial curve with %d segments in range [%f, %f]",
         segments, xMin, xMax);
}

// 多项式曲线（局部坐标系版本）
void LineRenderer::AddPolynomialCurveLocal(const glm::vec3 &center,
                                           const glm::vec3 &xAxis,
                                           const glm::vec3 &yAxis,
                                           const glm::vec3 &zAxis,
                                           float c0, float c1, float c2, float c3,
                                           float xMin, float xMax,
                                           const glm::vec3 &color,
                                           int segments, float lifetime)
{
    if (segments < 2)
        segments = 2;
    if (xMax <= xMin)
    {
        LOGE("LineRenderer", "Invalid x range: [%f, %f]", xMin, xMax);
        return;
    }

    // 归一化坐标轴
    glm::vec3 xNorm = glm::normalize(xAxis);
    glm::vec3 yNorm = glm::normalize(yAxis);
    // zAxis参数保留供将来使用，但当前曲线在XY平面内

    float step = (xMax - xMin) / segments;

    // 第一个点
    float x = xMin;
    float y = EvaluatePolynomial(x, c0, c1, c2, c3);
    glm::vec3 prevPoint = center + xNorm * x + yNorm * y;

    for (int i = 1; i <= segments; i++)
    {
        x = xMin + i * step;
        y = EvaluatePolynomial(x, c0, c1, c2, c3);

        glm::vec3 currentPoint = center + xNorm * x + yNorm * y;

        AddLine(prevPoint, currentPoint, color, lifetime);
        prevPoint = currentPoint;
    }

    LOGI("LineRenderer", "Added local polynomial curve with %d segments", segments);
}