#version 330 core

layout(location = 0) in vec3 pos; 

uniform mat4 cameraTransformation;
uniform mat4 perspective;
uniform mat4 view;

void main() {
	gl_Position = perspective * view * cameraTransformation * vec4(pos, 1);
}