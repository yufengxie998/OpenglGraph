#ifndef _H_WINDOWS_COMPAT_H
#define _H_WINDOWS_COMPAT_H
// 光源类型枚举
enum class LightType
{
    AMBIENT = 0, // 环境光（全局光照）
    POINT = 1,   // 点光源
    SPOT = 2     // 聚光灯
};

// 着色器类型枚举
enum ShaderType
{
    SHADER_SIMPLE = 0,
    SHADER_PBR = 1
};

// 线段样式枚举
enum class LineStyle {
    SOLID,      // 实线
    DASHED,     // 虚线
    DOTTED,     // 点线
    DASH_DOTTED // 点划线
};

#endif // WINDOWS_COMPAT_H
