#version 300 es
precision highp float;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in vec3 ViewPos;

out vec4 FragColor;

// PBR 纹理
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform sampler2D emissiveMap;

// 材质属性
uniform vec3 albedoColor;
uniform float metallicUniform;
uniform float roughnessUniform;
uniform float aoUniform;

// 光源系统
#define MAX_LIGHTS 16

struct Light {
    int type;           // 0:环境光, 1:点光源, 2:聚光灯
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    
    // 点光源衰减
    float constant;
    float linear;
    float quadratic;
    
    // 聚光灯参数
    float cutOff;
    float outerCutOff;
};

uniform Light lights[MAX_LIGHTS];
uniform int lightCount;

// 纹理使用标志
uniform bool useAlbedoMap;
uniform bool useNormalMap;
uniform bool useMetallicMap;
uniform bool useRoughnessMap;
uniform bool useAOMap;
uniform bool useEmissiveMap;

const float PI = 3.14159265359;

// 法线贴图转换
vec3 getNormalFromMap() {
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;
    
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

// 法线分布函数
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / denom;
}

// 几何函数
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// 菲涅尔函数
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// 计算点光源衰减
float getAttenuation(Light light, float distance) {
    return 1.0 / (light.constant + light.linear * distance + 
                  light.quadratic * (distance * distance));
}

// 计算聚光灯强度
float getSpotIntensity(Light light, vec3 lightDir) {
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    return intensity;
}

void main() {
    // 获取材质属性
    vec3 albedo = albedoColor;
    if (useAlbedoMap) {
        albedo = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));
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
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    
    // 计算直接光照
    vec3 Lo = vec3(0.0);
    
    for (int i = 0; i < lightCount; i++) {
        Light light = lights[i];
        
        // 环境光特殊处理
        if (light.type == 0) {
            Lo += albedo * light.color * light.intensity * ao;
            continue;
        }
        
        vec3 L;
        float attenuation = 1.0;
        float spotIntensity = 1.0;
        
        if (light.type == 1) { // 点光源
            L = normalize(light.position - FragPos);
            float distance = length(light.position - FragPos);
            attenuation = getAttenuation(light, distance);
        } else if (light.type == 2) { // 聚光灯
            L = normalize(light.position - FragPos);
            float distance = length(light.position - FragPos);
            attenuation = getAttenuation(light, distance);
            spotIntensity = getSpotIntensity(light, L);
        }
        
        vec3 H = normalize(V + L);
        float distance = length(light.position - FragPos);
        
        vec3 radiance = light.color * light.intensity * attenuation * spotIntensity;
        
        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    // 最终颜色
    vec3 color = Lo + emissive;
    
    // HDR 色调映射
    color = color / (color + vec3(1.0));
    
    // Gamma 校正
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}