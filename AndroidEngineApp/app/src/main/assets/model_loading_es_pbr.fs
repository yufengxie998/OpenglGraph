#version 300 es
precision highp float;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in vec3 ViewPos;

out vec4 FragColor;

// PBR 纹理
uniform sampler2D albedoMap;      // 基础颜色
uniform sampler2D normalMap;       // 法线贴图
uniform sampler2D metallicMap;     // 金属度
uniform sampler2D roughnessMap;    // 粗糙度
uniform sampler2D aoMap;           // 环境光遮蔽
uniform sampler2D emissiveMap;     // 自发光

// 如果某些贴图不存在，使用统一值
uniform vec3 albedoColor;
uniform float metallicUniform;
uniform float roughnessUniform;
uniform float aoUniform;

// 光照参数
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];
uniform int lightCount;

// 是否使用纹理
uniform bool useAlbedoMap;
uniform bool useNormalMap;
uniform bool useMetallicMap;
uniform bool useRoughnessMap;
uniform bool useAOMap;
uniform bool useEmissiveMap;

const float PI = 3.14159265359;

// 法线贴图转换函数
vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;
    
    // 计算TBN矩阵
    vec3 Q1  = dFdx(FragPos);
    vec3 Q2  = dFdy(FragPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);
    
    vec3 N  = normalize(Normal);
    vec3 T  = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    
    return normalize(TBN * tangentNormal);
}

// 法线分布函数 D (GGX)
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / denom;
}

// 几何函数 G (Smith's Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// 菲涅尔函数 F (Schlick's approximation)
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    // 获取材质属性
    vec3 albedo = albedoColor;
    if (useAlbedoMap) {
        albedo = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2)); // sRGB to linear
    }
    
    float metallic = metallicUniform;
    if (useMetallicMap) {
        metallic = texture(metallicMap, TexCoords).r;
    }
    
    float roughness = roughnessUniform;
    if (useRoughnessMap) {
        roughness = texture(roughnessMap, TexCoords).r;
    }
    
    float ao = aoUniform;
    if (useAOMap) {
        ao = texture(aoMap, TexCoords).r;
    }
    
    vec3 emissive = vec3(0.0);
    if (useEmissiveMap) {
        emissive = texture(emissiveMap, TexCoords).rgb;
    }
    
    // 获取法线
    vec3 N = normalize(Normal);
    if (useNormalMap) {
        N = getNormalFromMap();
    }
    
    vec3 V = normalize(ViewPos - FragPos);
    
    // 计算反射率 F0
    vec3 F0 = vec3(0.04); // 非金属的基础反射率
    F0 = mix(F0, albedo, metallic);
    
    // 反射率方程
    vec3 Lo = vec3(0.0);
    
    for (int i = 0; i < lightCount; ++i)
    {
        // 计算每个光源
        vec3 L = normalize(lightPositions[i] - FragPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - FragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;
        
        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
        vec3 specular = numerator / denominator;
        
        // 漫反射
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        float NdotL = max(dot(N, L), 0.0);
        
        // 添加到最终颜色
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    // 环境光照（简化版，使用环境光遮蔽）
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo + emissive;
    
    // HDR 色调映射
    color = color / (color + vec3(1.0));
    
    // Gamma 校正
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}