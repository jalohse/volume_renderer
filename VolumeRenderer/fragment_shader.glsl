#version 330 core

uniform sampler3D tex;
uniform int numSamples;
uniform float minimumValue;
uniform vec4 tfVals;
uniform mat4 rgbaVals;

out vec4 color;

in vec3 texCoor;
in vec3 camerapos;

vec4 ambientColor = vec4(0.5, 0, 0, .5);


void main() {
	vec3 dir = normalize((texCoor - vec3(0.5)) - camerapos);
	vec4 builtUpColor = vec4(0,0,0, 0);
	float alpha = 0.0f;
	vec3 step = dir * (1.0 / numSamples);
	vec3 samplePosition = texCoor;
	for(int i = 0; i < numSamples; i++){
		if(alpha < 1.0) {
			vec4 texVal = texture(tex, samplePosition);
			if(texVal.r > minimumValue && texVal.r < tfVals.x) {
				builtUpColor += rgbaVals[0];
				alpha += rgbaVals[0][3];
			} else if(texVal.r >= tfVals.x && texVal.r < tfVals.y) {
				builtUpColor += rgbaVals[1];
				alpha += rgbaVals[1][3];
			} else if(texVal.r >= tfVals.y && texVal.r < tfVals.z) {
				builtUpColor += rgbaVals[2];
				alpha += rgbaVals[2][3];
			} else if(texVal.r >= tfVals.z && texVal.r < tfVals.w) {
				builtUpColor += rgbaVals[3];
				alpha += rgbaVals[3][3];
			}
			samplePosition += step;
		} else {
			i = numSamples;
		}
	}
	color = builtUpColor;

}