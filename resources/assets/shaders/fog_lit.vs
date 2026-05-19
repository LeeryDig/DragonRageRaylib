#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 2) in vec3 vertexNormal;
layout(location = 3) in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;

out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragWorldPosition;
out vec3 fragWorldNormal;

void main()
{
    vec4 worldPosition = matModel*vec4(vertexPosition, 1.0);
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragWorldPosition = worldPosition.xyz;
    fragWorldNormal = normalize(mat3(transpose(inverse(matModel)))*vertexNormal);
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
