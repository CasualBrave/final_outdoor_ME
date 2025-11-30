#version 430 core

layout(location=0) in vec3 v_vertex;
layout(location=1) in vec3 v_normal;
layout(location=2) in vec3 v_tangent;
layout(location=3) in vec2 v_uv;

out vec3 f_worldPos;      // world-space position
out vec2 f_uv;
out vec3 f_lightDirTS;    // tangent-space light direction
out vec3 f_eyeDirTS;      // tangent-space view direction
out vec3 f_normalWS;      // world-space normal (vertex)
out vec3 f_tangentWS;     // world-space tangent
out vec3 f_bitangentWS;   // world-space bitangent

layout(location = 0) uniform mat4 modelMat;
layout(location = 5) uniform sampler2D elevationMap;
layout(location = 6) uniform sampler2D normalMap;
layout(location = 7) uniform mat4 viewMat;
layout(location = 8) uniform mat4 projMat;
layout(location = 9) uniform mat4 terrainVToUVMat;
layout(location = 1) uniform int vertexProcessIdx;
layout(location = 17) uniform vec3 lightDirWorld;
layout(location = 18) uniform vec3 cameraPosWorld;

// ========== 動態物件（飛機、石頭等） ==========
void commonProcess(){
    // TBN
    mat3 normalMat = transpose(inverse(mat3(modelMat)));
    vec3 T = normalize(normalMat * v_tangent);
    vec3 N = normalize(normalMat * v_normal);
    vec3 B = normalize(cross(N, T));

    vec4 worldVertex = modelMat * vec4(v_vertex, 1.0);
    f_worldPos = worldVertex.xyz;
    f_uv       = v_uv;
    f_normalWS   = N;
    f_tangentWS  = T;
    f_bitangentWS = B;

    // light/view direction in tangent space
    vec3 L = normalize(lightDirWorld);
    vec3 V = normalize(cameraPosWorld - f_worldPos);
    f_lightDirTS = vec3(dot(L, T), dot(L, B), dot(L, N));
    f_eyeDirTS   = vec3(dot(V, T), dot(V, B), dot(V, N));

    gl_Position = projMat * (viewMat * worldVertex);
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

    float angle = radians(180.0);

	mat3 rotY = mat3(
		vec3( cos(angle), 0.0, sin(angle)),
		vec3( 0.0,        1.0, 0.0       ),
		vec3(-sin(angle), 0.0, cos(angle))
	);

    // Construct approximate TBN for terrain
    vec3 N = normalize(rotY * normalWS);
    vec3 T = normalize(vec3(1.0, 0.0, 0.0));
    vec3 B = normalize(cross(N, T));
    T = normalize(cross(B, N));

    f_worldPos    = worldV.xyz;
    f_uv          = uv;
    f_normalWS    = N;
    f_tangentWS   = T;
    f_bitangentWS = B;

    vec3 L = normalize(lightDirWorld);
    vec3 V = normalize(cameraPosWorld - f_worldPos);
    f_lightDirTS = vec3(dot(L, T), dot(L, B), dot(L, N));
    f_eyeDirTS   = vec3(dot(V, T), dot(V, B), dot(V, N));

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
