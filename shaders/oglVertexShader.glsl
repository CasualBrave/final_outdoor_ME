#version 430 core

layout(location=0) in vec3 v_vertex;
layout(location=1) in vec3 v_normal ;
layout(location=2) in vec3 v_uv ;

out vec3 f_viewVertex ;
out vec3 f_uv ;

layout(location = 0) uniform mat4 modelMat ;
layout(location = 5) uniform sampler2D elevationMap ;
layout(location = 6) uniform sampler2D normalMap ;
layout(location = 7) uniform mat4 viewMat ;
layout(location = 8) uniform mat4 projMat ;
layout(location = 9) uniform mat4 terrainVToUVMat;
layout(location = 1) uniform int vertexProcessIdx ;


void commonProcess(){
	vec4 worldVertex = modelMat * vec4(v_vertex, 1.0) ;
	vec4 worldNormal = modelMat * vec4(v_normal, 0.0) ;

	vec4 viewVertex = viewMat * worldVertex ;
	vec4 viewNormal = viewMat * worldNormal ;
	
	f_viewVertex = viewVertex.xyz;
	f_uv = v_uv ;

	gl_Position = projMat * viewVertex ;
}

void terrainProcess(){
	vec4 worldV = modelMat * vec4(v_vertex, 1.0) ;
	// calculate uv
	vec4 uv = terrainVToUVMat * worldV ;
	uv.y = uv.z ;
	// get height from map
	float h = texture(elevationMap, uv.xy).r ;
	worldV.y = h;		
	// get normal from map
	vec4 normalTex = texture(normalMap, uv.xy) ;
	// [0, 1] -> [-1, 1]
	normalTex = normalTex * 2.0 - 1.0 ;
		
	// transformation	
	vec4 viewVertex = viewMat * worldV ;
	vec4 viewNormal = viewMat * vec4(normalTex.rgb, 0) ;	
	
	f_viewVertex = viewVertex.xyz;
	f_uv = uv.xyz ;

	gl_Position = projMat * viewVertex ;
}

void main(){
	if(vertexProcessIdx == 0){
		commonProcess() ;
	}
	else if(vertexProcessIdx == 3){
		terrainProcess() ;
	}
	else{
		commonProcess() ;
	}	
}