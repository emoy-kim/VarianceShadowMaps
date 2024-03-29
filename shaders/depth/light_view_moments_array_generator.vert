#version 460

uniform mat4 WorldMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 ModelViewProjectionMatrix;
uniform mat4 LightViewProjectionMatrix[3];
uniform int TextureIndex;

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec2 v_tex_coord;

void main()
{
   gl_Position = LightViewProjectionMatrix[TextureIndex] * WorldMatrix * vec4(v_position, 1.0f);
}