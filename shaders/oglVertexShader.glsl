#version 430 core

layout(location=0) in vec3 v_vertex;
layout(location=1) in vec3 v_normal;
layout(location=2) in vec2 v_uv;

out vec3 f_worldPos;      // world-space position
out vec3 f_worldNormal;   // world-space normal
out vec2 f_uv;

layout(location = 0) uniform mat4 modelMat;
layout(location = 5) uniform sampler2D elevationMap;
layout(location = 6) uniform sampler2D normalMap;
layout(location = 7) uniform mat4 viewMat;
layout(location = 8) uniform mat4 projMat;
layout(location = 9) uniform mat4 terrainVToUVMat;
layout(location = 1) uniform int vertexProcessIdx;

// ========== 動態物件（飛機、石頭等） ==========
void commonProcess(){
    // model-space normal -> world-space normal
    mat3 normalMat = transpose(inverse(mat3(modelMat)));

    vec4 worldVertex = modelMat * vec4(v_vertex, 1.0);
    vec3 worldNormal = normalize(normalMat * v_normal);

    f_worldPos    = worldVertex.xyz;
    f_worldNormal = worldNormal;
    f_uv          = v_uv;

    // 這裡直接做 world -> view -> clip
    vec4 viewVertex = viewMat * worldVertex;
    gl_Position = projMat * viewVertex;
}

// ========== 地形（height map + normal map） ==========
void terrainProcess(){
    // 頂點先丟到 world space（chunk transform）
    vec4 worldV = modelMat * vec4(v_vertex, 1.0);

    // 用 worldV 計算 elevation/normal 的 UV（照 template）
    vec4 uv4 = terrainVToUVMat * worldV;
    uv4.y = uv4.z;
    vec2 uv = uv4.xy;

    // 從高度貼圖取 height
    float h = texture(elevationMap, uv).r;
    worldV.y = h;

    // 從 normal map 取法向： [0,1] -> [-1,1]
    vec3 normalWS = texture(normalMap, uv).xyz * 2.0 - 1.0;

    // ★ 這裡把 normalWS 視為「世界空間 normal」
    // 不再乘 normalMat，避免 chunk 之間接縫
    f_worldPos    = worldV.xyz;
	f_uv          = uv;

	float angle = radians(180.0);

	mat3 rotY = mat3(
		vec3( cos(angle), 0.0, sin(angle)),
		vec3( 0.0,        1.0, 0.0       ),
		vec3(-sin(angle), 0.0, cos(angle))
	);

	f_worldNormal = normalize(rotY * normalWS);

    // 投影
    vec4 viewVertex = viewMat * worldV;
    gl_Position = projMat * viewVertex;
}

void main(){
    if(vertexProcessIdx == 0){
        commonProcess();
    }
    else if(vertexProcessIdx == 3){
        terrainProcess();
    }
    else{
        commonProcess();
    }
}
