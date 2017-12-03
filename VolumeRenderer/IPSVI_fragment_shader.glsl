#version 420 core

uniform sampler3D tex;
uniform layout(binding=3, rgba8ui) uimage2D illumCacheIn;
uniform layout(binding=4, rgba8ui) uimage2D illumCacheOut;
uniform int numSamples;
uniform float minimumValue;
uniform vec4 tfVals;
uniform mat4 rgbaVals;
uniform mat4 cameraTransformation;
uniform mat4 perspective;
uniform mat4 view;
uniform vec3 imageCacheOrigin;
uniform vec3 imageCacheNormal;
uniform vec3 imageCacheRight;
uniform vec3 imageCacheUp;

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

vec2 calculateImageCachePosition(vec3 samplePosition) {
	vec4 worldSamplePos =  perspective * view * cameraTransformation * vec4(samplePosition,1);
	worldSamplePos /= worldSamplePos.w;
	vec3 diagonal = vec3(worldSamplePos) - imageCacheOrigin;
	float distance = abs(dot(diagonal, imageCacheNormal));
    vec3 worldProjected = diagonal - (-distance * imageCacheNormal);

    // transforms world coordinates (have to be lying on the IC plane) to IC pixel space
    return vec2(round(dot(worldProjected, imageCacheRight)), round(dot(worldProjected, imageCacheUp)));
}


void main() {
	vec3 dir = normalize((texCoor - vec3(0.5)) - camerapos);
	vec4 builtUpColor = vec4(0,0,0, 0);
	float alpha = 0.0f;
	vec3 step = dir * (1.0 / numSamples);
	vec3 samplePosition = texCoor;
	vec2 illumPrevPos = calculateImageCachePosition(texCoor);
	vec4 illumIn = imageLoad(illumCacheIn, ivec2(illumPrevPos));
	for(int i = 0; i < numSamples; i++){
		if(alpha < 1.0) {
			vec4 texVal = texture(tex, samplePosition);
			vec2 illumPos = calculateImageCachePosition(samplePosition);
			illumIn = imageLoad(illumCacheIn, ivec2(illumPos));
			vec4 sampleCol = classify(texVal.r);
			alpha += sampleCol.a;
			vec4 illumOut;
			illumOut.rgb = (1.0 - sampleCol.a) * illumIn.rgb + sampleCol.a * sampleCol.rgb;
			illumOut.a = (1.0 - sampleCol.a) * illumIn.a + sampleCol.a;
			builtUpColor += sampleCol;
			illumPrevPos = illumPos;
			imageStore(illumCacheOut, ivec2(illumPrevPos), uvec4(sampleCol * 255.0f));
			samplePosition += step;
		} else {
			i = numSamples;
		}
	}
	color = builtUpColor;

}