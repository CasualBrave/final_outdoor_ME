#pragma once

#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../SceneManager.h"

class TerrainSceneObject
{
public:
	TerrainSceneObject(const int numChunk, const float* chunkVertices, const int numChunkVertex, const unsigned int* chunkIndices, const int numChunkIndex);
	virtual ~TerrainSceneObject();

	void update();

public:
	bool viewFrustumCullingTest(const glm::vec4* frustumPlaneEquations);

public:
	void initializeChunkGeometry(const float* chunkVertices, const int numChunkVertex, const unsigned int* chunkIndices, const int numIndex);
	//void setChunkBoundingVolume(const int idx, IBoundingVolume* bv);
	void setChunkTransformMatrix(const int idx, const glm::mat4& m);
	void setWorldVertexToElevationMapUVMatrix(const glm::mat4& m);
	void setElevationTextureHandle(const GLuint texHandle);
	void setNormalTextureHandle(const GLuint texHandle);
	void setAlbedoTextureHandle(const GLuint texHandle);

private:
	const int m_numChunk;
	glm::mat4* m_chunkModelMats = nullptr;
	//IBoundingVolume** m_chunkBoundingVolumes = nullptr;
	bool* m_chunkVisibilityFlags = nullptr;
	glm::mat4 m_worldVertexToElevationMapUvMat;

	int m_numIndex;

	GLuint m_evelationMapHandle;
	GLuint m_normalMapHandle;
	GLuint m_albedoMapHandle;
	GLuint m_vao;

};

