#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in mat4 aInstanceMatrix;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

out vec3 vNormal;
out vec3 vFragPos;
out vec2 vTexCoords;

void main()
{
    // We do this so the cube rotates on its own axis 
    // Extract translation from instance matrix
    vec3 instancePos = vec3(aInstanceMatrix[3]);
    
    // Apply rotation to local position, then add instance translation
    vec3 rotatedPos = vec3(uModel * vec4(aPos, 0.0)) + instancePos;
    
    vec4 worldPos = vec4(rotatedPos, 1.0);
    vFragPos    = vec3(worldPos);
    vNormal     = uNormalMatrix * aNormal;
    vTexCoords  = aTexCoords;
    gl_Position = uProjection * uView * worldPos;
}