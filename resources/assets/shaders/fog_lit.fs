#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragWorldPosition;
in vec3 fragWorldNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 cameraPosition;
uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;
uniform float fogDensity;
uniform int fogEnabled;
uniform vec3 ambientColor;
uniform int directionalLightEnabled;
uniform vec3 directionalLightDirection;
uniform vec3 directionalLightColor;
uniform float directionalLightIntensity;

out vec4 finalColor;

void main()
{
    vec4 baseColor = texture(texture0, fragTexCoord)*colDiffuse*fragColor;
    vec3 normal = normalize(fragWorldNormal);
    vec3 litColor = ambientColor;

    if (directionalLightEnabled == 1)
    {
        vec3 lightToSurface = normalize(-directionalLightDirection);
        float ndotl = max(dot(normal, lightToSurface), 0.0);
        ndotl = floor(ndotl*8.0)/8.0;
        litColor += directionalLightColor*directionalLightIntensity*ndotl;
    }

    vec4 texelColor = vec4(baseColor.rgb*clamp(litColor, 0.0, 1.5), baseColor.a);

    if (fogEnabled == 1)
    {
        float distanceToCamera = distance(fragWorldPosition, cameraPosition);
        float fogFactor = clamp((fogEnd - distanceToCamera)/max(fogEnd - fogStart, 0.001), 0.0, 1.0);
        fogFactor = pow(fogFactor, fogDensity);
        fogFactor = floor(fogFactor*32.0)/32.0;
        texelColor.rgb = mix(fogColor, texelColor.rgb, fogFactor);
    }

    finalColor = texelColor;
}
