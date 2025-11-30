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

vec3 viewVec(vec3 v){
    return normalize(v) * 0.5 + 0.5;
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
    } else {
        color = texture(gDiffuseTex, uv).rgb;
    }
    fragColor = vec4(color, 1.0);
}
