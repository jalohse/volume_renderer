#version 330 core

uniform sampler3D tex;
uniform vec3 rayStart;
uniform vec3 rayEnd;
uniform vec3 viewDir;
uniform float minimumValue;
uniform vec4 tfVals;
uniform mat4 rgbaVals;

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
		vec4 texVal = texture(tex, vec3(texCoor.x, texCoor.y, t);
		if(texVal.r > minimumValue && texVal.r < tfVals.x) {
			builtUpColor += rgbaVals[0];
		} else if(texVal.r >= tfVals.x && texVal.r < tfVals.y) {
			builtUpColor += rgbaVals[1];
		} else if(texVal.r >= tfVals.y && texVal.r < tfVals.z) {
			builtUpColor += rgbaVals[2];
		} else if(texVal.r >= tfVals.z && texVal.r < tfVals.w) {
			builtUpColor += rgbaVals[3];
		}
		t += step;
	}
	color = builtUpColor + ambientColor;

}