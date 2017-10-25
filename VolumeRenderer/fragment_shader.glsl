#version 330 core

uniform sampler3D tex;
uniform vec3 rayStart;
uniform vec3 rayEnd;
uniform vec3 viewDir;

out vec4 color;

in vec3 texCoor;

vec4 ambientColor = vec4(0.5, 0, 0, .5);


void main() {
	vec4 builtUpColor = vec4(0,0,0, 0);
	float alpha = 0.0f;
	float t = 0;
	float step = 1.0 / 10.0;
	for(int i = 0; i < 100; i++){
		vec3 samplePosition = rayStart + (t * viewDir);
		vec4 texVal = texture(tex, vec3(texCoor.x, texCoor.y, t));
		if(texVal.r > 0.3 && texVal.r < 0.4) {
			builtUpColor += vec4(0, 0, 1, 0.2);
		} else if(texVal.r >= 0.4 && texVal.r < 0.6) {
			builtUpColor += vec4(1, 0, 0, 0.2);
		} else if(texVal.r >= 0.6) {
			builtUpColor += vec4(0, 1, 0, 0.2);
		}
		t += step;
	}
	color = builtUpColor + ambientColor;

}