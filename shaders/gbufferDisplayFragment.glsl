#version 430 core

in vec2 v_uv;
out vec4 fragColor;

layout(location = 0) uniform sampler2D gPositionTex;
layout(location = 1) uniform sampler2D gNormalTex;
layout(location = 2) uniform sampler2D gAmbientTex;
layout(location = 3) uniform sampler2D gDiffuseTex;
layout(location = 4) uniform sampler2D gSpecularTex;
layout(location = 5) uniform int displayMode; // 0:pos,1:normal,2:ambient,3:diffuse,4:specular
layout(location = 6) uniform vec2 uvScale;
layout(location = 7) uniform vec2 uvBias;
layout(location = 8) uniform vec3 lightDirWorld;
layout(location = 9) uniform vec3 cameraPosWorld;
layout(location = 10) uniform mat4 viewMat;
layout(location = 11) uniform sampler2D depthPyramid;
layout(location = 12) uniform int depthMipLevel;
layout(location = 13) uniform mat4 invProj;
layout(location = 14) uniform float depthVisFar;
layout(location = 16) uniform float depthVisGamma; // 1.0 = no curve
// Cascaded shadow mapping
layout(location = 17) uniform int shadowEnabled;
layout(location = 18) uniform int cascadeVizEnabled;
layout(location = 19) uniform vec3 cascadeFar; // (50, 200, 500)
layout(location = 20) uniform mat4 lightVP[3];
layout(location = 32) uniform sampler2DArrayShadow shadowMap;
layout(location = 34) uniform mat4 cullViewMat;

vec3 viewVec(vec3 v){
    return normalize(v) * 0.5 + 0.5;
}

vec4 applyFog(vec4 color, vec3 viewPos){
    const vec4  FOG_COLOR = vec4(0.0, 0.0, 0.0, 1.0);
    const float MAX_DIST = 400.0;
    const float MIN_DIST = 350.0;

    float dis = length(viewPos);
    float fogFactor = (MAX_DIST - dis) / (MAX_DIST - MIN_DIST);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    fogFactor = fogFactor * fogFactor;

    return mix(FOG_COLOR, color, fogFactor);
}

vec4 applyGamma(vec4 color){
    color.rgb = pow(color.rgb, vec3(0.5));
    return color;
}

bool chooseCascadeMapBased(vec3 worldPos, out int chosen, out vec3 uvz);

float sampleShadow(vec3 worldPos, vec3 normalWS) {
    if (shadowEnabled == 0) return 1.0;
	int chosen = -1;
	vec3 uvz = vec3(0.0);
	if (!chooseCascadeMapBased(worldPos, chosen, uvz)) return 1.0;

	// Bias: reduce acne (world-space normal vs world-space light direction).
	vec3 Lw = normalize(lightDirWorld);
	float ndl = max(dot(normalize(normalWS), Lw), 0.0);
	float bias = max(0.0008 * (1.0 - ndl), 0.0003);

	ivec3 ts = textureSize(shadowMap, 0);
	vec2 texel = 1.0 / vec2(max(ts.x, 1), max(ts.y, 1));

	float sum = 0.0;
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) {
			vec2 o = vec2(x, y) * texel;
			sum += texture(shadowMap, vec4(uvz.xy + o, float(chosen), uvz.z - bias));
		}
	}
	return sum / 9.0;
}

bool chooseCascadeMapBased(vec3 worldPos, out int chosen, out vec3 uvz) {
	chosen = -1;
	uvz = vec3(0.0);
	for (int c = 0; c < 3; ++c) {
		vec4 clip = lightVP[c] * vec4(worldPos, 1.0);
		if (abs(clip.w) < 1e-6) continue;
		vec3 ndc = clip.xyz / clip.w;
		vec3 t = ndc * 0.5 + 0.5;
		if (t.x >= 0.0 && t.x <= 1.0 && t.y >= 0.0 && t.y <= 1.0 && t.z >= 0.0 && t.z <= 1.0) {
			chosen = c;
			uvz = t;
			return true;
		}
	}
	return false;
}

void main(){
    vec2 uv = v_uv * uvScale + uvBias;
    vec3 color = vec3(0.0);
    if(displayMode == 0){
        color = viewVec(texture(gPositionTex, uv).xyz);
    } else if(displayMode == 1){
        color = viewVec(texture(gNormalTex, uv).xyz);
    } else if(displayMode == 2){
        color = texture(gAmbientTex, uv).rgb;
    } else if(displayMode == 3){
        color = texture(gDiffuseTex, uv).rgb;
    } else if(displayMode == 4){
        color = texture(gSpecularTex, uv).rgb;
    } else if(displayMode == 5){
        // default: Blinn-Phong 與原 forward 版本一致（含 fog + gamma）
        float rawDepth = texture(depthPyramid, uv).r;
        vec3 P = texture(gPositionTex, uv).xyz;
        vec3 Nworld = normalize(texture(gNormalTex, uv).xyz);
        vec3 ambient = texture(gAmbientTex, uv).rgb;
        vec3 diffuse = texture(gDiffuseTex, uv).rgb;
        vec4 specPacked = texture(gSpecularTex, uv);
        vec3 specColor = specPacked.rgb;
        float shininess = max(specPacked.a, 1.0);

        vec3 viewPos = (viewMat * vec4(P, 1.0)).xyz;
        vec3 N = normalize(mat3(viewMat) * Nworld);
        vec3 L = normalize((viewMat * vec4(lightDirWorld, 0.0)).xyz);
        vec3 V = normalize(-viewPos);
        vec3 H = normalize(L + V);

        const vec3 Ia = vec3(0.2);
        const vec3 Id = vec3(0.64);
        const vec3 Is = vec3(0.16);

        float ndl = max(dot(N, L), 0.0);
        float ndh = max(dot(N, H), 0.0);
        float spec = (ndl > 0.0) ? pow(ndh, shininess) : 0.0;

        // Background pixels have depth==1 and cleared G-buffer; keep them untouched (sky stays black).
        float shadow = (rawDepth >= 0.999999) ? 1.0 : sampleShadow(P, Nworld);
        vec3 direct = Id * diffuse * ndl + Is * specColor * spec;
        vec4 shaded = vec4(Ia * ambient + shadow * direct, 1.0);
        vec3 outColor = applyGamma(applyFog(shaded, viewPos)).rgb;

        // Visualize cascades (RGB mixing) using map-based cascade selection:
        // pick the tightest cascade that covers this pixel in shadow texture space.
        if (cascadeVizEnabled != 0 && rawDepth < 0.999999) {
			int cas = -1;
			vec3 uvz = vec3(0.0);
			if (chooseCascadeMapBased(P, cas, uvz)) {
				vec3 tint = (cas == 0) ? vec3(1.0, 0.0, 0.0)
					: (cas == 1) ? vec3(0.0, 1.0, 0.0)
					: vec3(0.0, 0.0, 1.0);
				outColor = clamp(outColor * 0.6 + tint * 0.4, 0.0, 1.0);
			}
        }

        color = outColor;
    } else if(displayMode == 6){
        // depth mip visualization
        float depthVal = textureLod(depthPyramid, uv, float(depthMipLevel)).r;
        // Linearized view-space z normalized by far
        vec4 ndcV = vec4(uv * 2.0 - 1.0, depthVal * 2.0 - 1.0, 1.0);
        vec4 viewV = invProj * ndcV;
        viewV /= viewV.w;
        float denom = max(depthVisFar, 0.001);
        float d = clamp(-viewV.z / denom, 0.0, 1.0);
        float g = max(depthVisGamma, 0.001);
        d = pow(d, g);
        color = vec3(d);
    } else {
        color = texture(gDiffuseTex, uv).rgb;
    }
    fragColor = vec4(color, 1.0);
}
