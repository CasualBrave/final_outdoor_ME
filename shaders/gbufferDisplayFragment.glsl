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

        vec4 shaded = vec4(Ia * ambient + Id * diffuse * ndl + Is * specColor * spec, 1.0);
        color = applyGamma(applyFog(shaded, viewPos)).rgb;
    } else {
        color = texture(gDiffuseTex, uv).rgb;
    }
    fragColor = vec4(color, 1.0);
}
