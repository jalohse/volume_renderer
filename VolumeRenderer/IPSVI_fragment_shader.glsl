#version 420 core

uniform sampler3D tex;
uniform sampler2D illumCacheIn;
uniform sampler2D illumCacheOut;
uniform int numSamples;
uniform float minimumValue;
uniform vec4 tfVals;
uniform mat4 rgbaVals;

out vec4 color;

in vec3 texCoor;
in vec3 camerapos;

vec4 ambientColor = vec4(0.5, 0, 0, .5);

vec4 classify(float val) {
	vec4 col = vec4(0,0,0,0);
	if(val > minimumValue && val < tfVals.x) {
		col += rgbaVals[0];
	} else if(val >= tfVals.x && val < tfVals.y) {
		col += rgbaVals[1];
	} else if(val >= tfVals.y && val < tfVals.z) {
		col += rgbaVals[2];
	} else if(val >= tfVals.z && val < tfVals.w) {
		col += rgbaVals[3];
	}
	return col;
}

void main() {
	vec3 dir = normalize((texCoor - vec3(0.5)) - camerapos);
	vec4 builtUpColor = vec4(0,0,0, 0);
	float alpha = 0.0f;
	vec3 step = dir * (1.0 / numSamples);
	vec3 samplePosition = texCoor;
	vec2 illumPrevPos = texCoor.xy;
	vec4 illumIn = texture(illumCacheIn, illumPrevPos);
	for(int i = 0; i < numSamples; i++){
		if(alpha < 1.0) {
			vec4 texVal = texture(tex, samplePosition);
			vec2 illumPos = samplePosition.xy;
			illumIn += texVal;
			vec4 sampleCol = classify(texVal.r);
			alpha += sampleCol.a;
			vec4 illumOut;
			illumOut.rgb = (1.0 - sampleCol.a) * illumIn.rgb + sampleCol.a * sampleCol.rgb;
			illumOut.a = (1.0 - sampleCol.a) * illumIn.a + sampleCol.a;
			builtUpColor += sampleCol;
			imageStore(illumCacheOut, ivec2(illumPrevPos), sampleCol);
			samplePosition += step;
		} else {
			i = numSamples;
		}
	}
	color = builtUpColor;

}