#pragma once


#include "TerrainSceneObject.h"
#include "MyTerrainData.h"

class MyTerrain
{
public:
	MyTerrain();
	virtual ~MyTerrain();

public:
	void init(const float chunkSize);
	void setupTerrainSceneObject(const int numChunk, const int chunkSize, const float* chunkVertices, const int numChunkVertex, const unsigned int* chunkIndices, const int numChunkIndex, const MyTerrainData* td);

public:
	void updateState(const glm::mat4& viewMat, const glm::vec3& viewOrg, const glm::mat4& projMat, const glm::vec4* frustumPlaneEquations) ;

public:
	TerrainSceneObject* sceneObject();

public:
	const glm::mat4 worldVtoElevationUVMat() const;
	const MyTerrainData* terrainData() const;

private:
	TerrainSceneObject* m_terrainSO = nullptr;
	MyTerrainData* m_terrainData = nullptr;
	const int m_numChunk;

	glm::mat4 m_worldVtoElevationUVMat ;
	glm::mat4* m_chunkModelMats = nullptr;

};

