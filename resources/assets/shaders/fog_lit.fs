#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragWorldPosition;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 cameraPosition;
uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;
uniform float fogDensity;
uniform int fogEnabled;

out vec4 finalColor;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord)*colDiffuse*fragColor;

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
