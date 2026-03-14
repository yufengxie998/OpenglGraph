#version 300 es
precision highp float;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform sampler2D texture_diffuse1;
uniform bool hasTexture;

void main() {
    if (hasTexture) {
        FragColor = texture(texture_diffuse1, TexCoords);
    } else {
        FragColor = vec4(1.0, 0.0, 1.0, 1.0); // 紫色表示无纹理
    }
}