#version 430 core
in vec3 vNormal;
in vec3 vFragPos;
in vec2 vTexCoords;

uniform vec3      uLightPos;
uniform vec3      uViewPos;
uniform vec3      uLightColor;
uniform float     uLightConstant;
uniform float     uLightLinear;
uniform float     uLightQuadratic;
uniform vec3      uAmbientColor;
uniform float     uAmbientStrength;
uniform vec3      uObjectColor;
uniform vec3      uSpecularColor;
uniform float     uShininess;
uniform bool      uUseTexture;
uniform bool      uUseSpecularMap;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

out vec4 FragColor;

void main()
{
    // Attenuation
    float dist        = length(uLightPos - vFragPos);
    float attenuation = 1.0 / (uLightConstant
                              + uLightLinear    * dist
                              + uLightQuadratic * dist * dist);

    // Ambient
    vec3 ambient = uAmbientStrength * uAmbientColor * uLightColor;

    // Diffuse
    vec3  norm     = normalize(vNormal);
    vec3  lightDir = normalize(uLightPos - vFragPos);
    float diff     = max(dot(norm, lightDir), 0.0);
    vec3  diffuse  = diff * uLightColor * attenuation;

    // Specular (Blinn-Phong)
    vec3  viewDir    = normalize(uViewPos - vFragPos);
    vec3  halfDir    = normalize(lightDir + viewDir);
    float spec       = pow(max(dot(norm, halfDir), 0.0), uShininess);
    vec3  specSample = uUseSpecularMap
                         ? vec3(texture(texture_specular1, vTexCoords))
                         : uSpecularColor;
    vec3 specular = spec * specSample * uLightColor * attenuation;

    // Base colour
    vec3 baseColor = uUseTexture
        ? vec3(texture(texture_diffuse1, vTexCoords))
        : uObjectColor;

    FragColor = vec4((ambient + diffuse + specular) * baseColor, 1.0);
}
