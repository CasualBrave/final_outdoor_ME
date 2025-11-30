#version 430 core

in vec3 f_worldPos;      // from VS
in vec2 f_uv;
in vec3 f_lightDirTS;
in vec3 f_eyeDirTS;
in vec3 f_normalWS;
in vec3 f_tangentWS;
in vec3 f_bitangentWS;

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAmbient;
layout(location = 3) out vec4 gDiffuse;
layout(location = 4) out vec4 gSpecular;

layout(location = 2)  uniform int   pixelProcessId;
layout(location = 4)  uniform sampler2D albedoTexture;

layout(location = 11) uniform vec3 materialAmbient;
layout(location = 12) uniform vec3 materialSpecular;
layout(location = 13) uniform float materialShininess;
layout(location = 14) uniform int useNormalMap;
layout(location = 15) uniform sampler2D normalTexture;

vec3 computeWorldNormal(){
    vec3 N = normalize(f_normalWS);
    if (useNormalMap == 1) {
        vec3 nTex = texture(normalTexture, f_uv).xyz * 2.0 - 1.0;
        mat3 TBN = mat3(normalize(f_tangentWS), normalize(f_bitangentWS), normalize(f_normalWS));
        N = normalize(TBN * nTex);
    }
    return N;
}

void writeGBuffer(vec3 baseColor, vec3 normalWS){
    gPosition = vec4(f_worldPos, 1.0);
    gNormal   = vec4(normalize(normalWS), 1.0);
    gAmbient  = vec4(baseColor * materialAmbient, 1.0);
    gDiffuse  = vec4(baseColor, 1.0);
    gSpecular = vec4(materialSpecular, materialShininess);
}

// =================== Passes ===================
void terrainPass(){
    vec3 texel  = texture(albedoTexture, f_uv).rgb;
    vec3 N = computeWorldNormal();
    writeGBuffer(texel, N);
}

void texturePass(){
    vec3 texel  = texture(albedoTexture, f_uv).rgb;
    vec3 N = computeWorldNormal();
    writeGBuffer(texel, N);
}

void pureColor(){
    vec3 baseColor = vec3(1.0, 0.0, 0.0);
    vec3 normalWS = normalize(f_normalWS);
    writeGBuffer(baseColor, normalWS);
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
