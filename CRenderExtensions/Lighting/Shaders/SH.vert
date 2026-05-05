#version 430 core
layout (location = 0) in vec3 APosition;
layout (location = 1) in vec4 AColor;
layout (location = 2) in vec3 ANormal;
layout (location = 3) in vec2 AUV;
layout (location = 4) in mat4 instanceMatrix;
layout (location = 8) in vec4 instanceColor;
layout (location = 9) in vec2 instanceUV;

uniform mat4 perspectiveMatrix;
uniform vec3 CamPosition;

out vec4 color;
out vec4 vertPos;
out vec3 vertNormal;

void main()
{
	vec4 WorldPos = instanceMatrix * vec4(APosition, 1.0);
	vec3 RelPos = WorldPos.xyz - CamPosition;
	gl_Position = perspectiveMatrix * vec4(RelPos, 1.0);

	vertPos = WorldPos;

	vertNormal = ANormal;

	color = instanceColor;
};