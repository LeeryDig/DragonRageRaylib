#version 330

#define MAX_DIRECTIONAL_LIGHTS 2
#define MAX_POINT_LIGHTS 4
#define MAX_SPOT_LIGHTS 2

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

uniform int directionalLightCount;
uniform int directionalLightEnabled[MAX_DIRECTIONAL_LIGHTS];
uniform vec3 directionalLightDirection[MAX_DIRECTIONAL_LIGHTS];
uniform vec3 directionalLightColor[MAX_DIRECTIONAL_LIGHTS];
uniform float directionalLightIntensity[MAX_DIRECTIONAL_LIGHTS];

uniform int pointLightCount;
uniform int pointLightEnabled[MAX_POINT_LIGHTS];
uniform vec3 pointLightPosition[MAX_POINT_LIGHTS];
uniform vec3 pointLightColor[MAX_POINT_LIGHTS];
uniform float pointLightIntensity[MAX_POINT_LIGHTS];
uniform float pointLightRange[MAX_POINT_LIGHTS];

uniform int spotLightCount;
uniform int spotLightEnabled[MAX_SPOT_LIGHTS];
uniform vec3 spotLightPosition[MAX_SPOT_LIGHTS];
uniform vec3 spotLightDirection[MAX_SPOT_LIGHTS];
uniform vec3 spotLightColor[MAX_SPOT_LIGHTS];
uniform float spotLightIntensity[MAX_SPOT_LIGHTS];
uniform float spotLightRange[MAX_SPOT_LIGHTS];
uniform float spotLightInnerCone[MAX_SPOT_LIGHTS];
uniform float spotLightOuterCone[MAX_SPOT_LIGHTS];

out vec4 finalColor;

void main()
{
    vec4 baseColor = texture(texture0, fragTexCoord)*colDiffuse*fragColor;
    vec3 normal = normalize(fragWorldNormal);
    vec3 litColor = ambientColor;

    for (int i = 0; i < MAX_DIRECTIONAL_LIGHTS; i++)
    {
        if (i >= directionalLightCount) break;
        if (directionalLightEnabled[i] == 0) continue;
        vec3 lightToSurface = normalize(-directionalLightDirection[i]);
        float ndotl = max(dot(normal, lightToSurface), 0.0);
        ndotl = floor(ndotl*8.0)/8.0;
        litColor += directionalLightColor[i]*directionalLightIntensity[i]*ndotl;
    }

    for (int i = 0; i < MAX_POINT_LIGHTS; i++)
    {
        if (i >= pointLightCount) break;
        if (pointLightEnabled[i] == 0) continue;
        vec3 toLight = pointLightPosition[i] - fragWorldPosition;
        float dist = length(toLight);
        float range = max(pointLightRange[i], 0.001);
        float attenuation = clamp(1.0 - dist/range, 0.0, 1.0);
        attenuation = floor(attenuation*8.0)/8.0;
        vec3 lightDir = normalize(toLight);
        float ndotl = max(dot(normal, lightDir), 0.0);
        ndotl = floor(ndotl*8.0)/8.0;
        litColor += pointLightColor[i]*pointLightIntensity[i]*ndotl*attenuation;
    }

    for (int i = 0; i < MAX_SPOT_LIGHTS; i++)
    {
        if (i >= spotLightCount) break;
        if (spotLightEnabled[i] == 0) continue;
        vec3 toLight = spotLightPosition[i] - fragWorldPosition;
        float dist = length(toLight);
        float range = max(spotLightRange[i], 0.001);
        vec3 lightDir = normalize(toLight);
        vec3 fromLightToFrag = normalize(fragWorldPosition - spotLightPosition[i]);
        float coneDot = dot(fromLightToFrag, normalize(spotLightDirection[i]));
        float innerCone = cos(radians(spotLightInnerCone[i]));
        float outerCone = cos(radians(spotLightOuterCone[i]));
        float cone = clamp((coneDot - outerCone)/max(innerCone - outerCone, 0.001), 0.0, 1.0);
        cone = floor(cone*8.0)/8.0;
        float attenuation = clamp(1.0 - dist/range, 0.0, 1.0);
        attenuation = floor(attenuation*8.0)/8.0;
        float ndotl = max(dot(normal, lightDir), 0.0);
        ndotl = floor(ndotl*8.0)/8.0;
        litColor += spotLightColor[i]*spotLightIntensity[i]*ndotl*attenuation*cone;
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
