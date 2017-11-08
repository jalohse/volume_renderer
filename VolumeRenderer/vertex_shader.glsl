#version 330 core

layout(location = 0) in vec3 pos; 
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 texCoordinate;

out vec3 texCoor;
out vec3 camerapos;

uniform mat4 cameraTransformation;
uniform mat4 perspective;
uniform mat4 view;
uniform vec3 cameraPos;

void main() {
	gl_Position = perspective * view * cameraTransformation * vec4(pos, 1);
	texCoor =  texCoordinate;
	mat4 inverse = inverse(view * cameraTransformation);
	camerapos = vec3(inverse * vec4(cameraPos, 1));
}