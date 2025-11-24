#version 430 core

in vec3 f_worldPos;      // from VS
in vec3 f_worldNormal;   // from VS
in vec2 f_uv;

layout(location = 0) out vec4 fragColor;

layout(location = 2)  uniform int   pixelProcessId;
layout(location = 4)  uniform sampler2D albedoTexture;

// 注意：這裡我們需要 viewMat 來做 world -> view
layout(location = 7)  uniform mat4 viewMat;

// 你原本就有的 uniform（view-space light direction + material） + normal map 開關
layout(location = 10) uniform vec3 lightDirView;      // 已經是 view-space
layout(location = 11) uniform vec3 materialAmbient;
layout(location = 12) uniform vec3 materialSpecular;
layout(location = 13) uniform float materialShininess;
layout(location = 14) uniform int useNormalMap;
layout(location = 15) uniform sampler2D normalTexture;

// =================== Fog ===================
vec4 withFog(vec4 color){
    const vec4  FOG_COLOR = vec4(0.0, 0.0, 0.0, 1.0);
    const float MAX_DIST = 400.0;
    const float MIN_DIST = 350.0;

    // 先把 worldPos 轉成 view-space
    vec3 viewPos = (viewMat * vec4(f_worldPos, 1.0)).xyz;

    float dis = length(viewPos);
    float fogFactor = (MAX_DIST - dis) / (MAX_DIST - MIN_DIST);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    fogFactor = fogFactor * fogFactor;

    return mix(FOG_COLOR, color, fogFactor);
}

// =================== Gamma ===================
vec4 applyGamma(vec4 color){
    color.rgb = pow(color.rgb, vec3(0.5));
    return color;
}

// =================== Blinn-Phong ===================
vec4 blinnPhong(vec3 baseColor){
    // world-normal -> view-space normal
    vec3 N = normalize(mat3(viewMat) * f_worldNormal);
    if (useNormalMap == 1) {
        vec3 nTex = texture(normalTexture, f_uv).xyz * 2.0 - 1.0;
        N = normalize(N + nTex);
    }

    // view-position（同 fog 用的那個）
    vec3 viewPos = (viewMat * vec4(f_worldPos, 1.0)).xyz;

    // view-space V, L, H
    vec3 V = normalize(-viewPos);
    vec3 L = normalize(lightDirView);   // 你在 CPU 裡算好的 view-space 光方向
    vec3 H = normalize(L + V);

    // 常數光照參數
    const vec3 Ia = vec3(0.2);
    const vec3 Id = vec3(0.64);
    const vec3 Is = vec3(0.16);

    float ndl = max(dot(N, L), 0.0);
    float ndh = max(dot(N, H), 0.0);
    float spec = (ndl > 0.0) ? pow(ndh, materialShininess) : 0.0;

    vec3 ambient  = Ia * baseColor * materialAmbient;
    vec3 diffuse  = Id * baseColor * ndl;
    vec3 specular = Is * materialSpecular * spec;

    return vec4(ambient + diffuse + specular, 1.0);
}

// =================== Passes ===================
void terrainPass(){
    vec4 texel  = texture(albedoTexture, f_uv);
    vec4 shaded = blinnPhong(texel.rgb);
    fragColor   = applyGamma(withFog(shaded));
    fragColor.a = 1.0;
}

void texturePass(){
    vec4 texel  = texture(albedoTexture, f_uv);
    vec4 shaded = blinnPhong(texel.rgb);
    fragColor   = applyGamma(withFog(shaded));
    fragColor.a = 1.0;
}

void pureColor(){
    vec4 shadedColor = vec4(1.0, 0.0, 0.0, 1.0);
    fragColor = applyGamma(withFog(shadedColor));
}

void main(){
    if(pixelProcessId == 5){
        pureColor();
    }
    else if(pixelProcessId == 6){
        texturePass();
    }
    else if(pixelProcessId == 7){
        terrainPass();
    }
    else{
        pureColor();
    }
}
