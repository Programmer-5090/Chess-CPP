#version 430 core

in vec2 TexCoord;

uniform vec4 uColor;
uniform sampler2D uTexture;
uniform bool uUseTexture;

out vec4 FragColor;

void main()
{
    vec4 texColor = uUseTexture ? texture(uTexture, TexCoord) : vec4(1.0);
    FragColor = texColor * uColor;
}
