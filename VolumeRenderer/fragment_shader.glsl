#version 330 core

uniform sampler3D tex;

out vec4 color;

in vec3 texCoor;

vec4 ambientColor = vec4(0.5, 0, 0, 1);


void main() {
	vec4 builtUpColor = vec4(0,0,0, 0);
	float alpha = 0.0f;

			vec4 texVal = texture(tex, texCoor);
			if(texVal.r > 0.3 && texVal.r < 0.4) {
				builtUpColor += vec4(0, 0, 1, 0.2);
			} else if(texVal.r >= 0.4 && texVal.r < 0.6) {
				builtUpColor += vec4(1, 0, 0, 0.2);
			}
			else if(texVal.r >= 0.6) {
				builtUpColor += vec4(0, 1, 0, 0.2);
			}
	color = ambientColor;

}