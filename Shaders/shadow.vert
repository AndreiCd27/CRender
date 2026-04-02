#version 330 core
layout (location = 0) in vec3 APosition;
layout (location = 4) in mat4 instanceMatrix;

uniform mat4 lightPerspMatrix;

void main()
{
	gl_Position = lightPerspMatrix * instanceMatrix * vec4(APosition, 1.0);
};