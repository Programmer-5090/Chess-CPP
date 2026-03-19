#version 430 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 uProjection;
uniform mat4 uModel;

out vec2 TexCoord;

void main()
{
    TexCoord = aTexCoord;
    gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);
}
