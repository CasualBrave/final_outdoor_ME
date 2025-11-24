#define GLM_ENABLE_EXPERIMENTAL
#include "MyTerrain.h"
#include <glm\gtx\quaternion.hpp>
#include <glm\mat4x4.hpp>
#include <glm\gtx\transform.hpp>


MyTerrain::MyTerrain() : m_numChunk(4)
{
	this->m_chunkModelMats = new glm::mat4[this->m_numChunk];
	for (int i = 0; i < this->m_numChunk; i++) {
		this->m_chunkModelMats[i] = glm::mat4(1.0);
	}
}


MyTerrain::~MyTerrain()
{
	delete[] this->m_chunkModelMats;
}

TerrainSceneObject* MyTerrain::sceneObject() {
	return this->m_terrainSO;
}

void MyTerrain::init(const float chunkSize) {
	MyTerrainData* mtd = MyTerrainData::fromMYTD("assets\\outdoor\\elevationMap_2.mytd");
	mtd->loadChunkDataFromFile("assets\\outdoor\\terrain.chunkdata");
	this->setupTerrainSceneObject(this->m_numChunk, 512, mtd->m_chunkVertices, mtd->m_numChunkVertex, mtd->m_chunkIndices, mtd->m_numChunkIndex, mtd);

	this->m_terrainData = mtd;
	this->m_terrainData->m_worldVtoElevationUVMat = this->m_worldVtoElevationUVMat;
}
void MyTerrain::setupTerrainSceneObject(const int numChunk, const int chunkSize, const float* chunkVertices, const int numChunkVertex, const unsigned int* chunkIndices, const int numChunkIndex, const MyTerrainData* td) {
	this->m_terrainSO = new TerrainSceneObject(numChunk, chunkVertices, numChunkVertex, chunkIndices, numChunkIndex);

	auto createTexture = [](const void* data, const int numComp, const int width, const int height, const GLint internalFormat, const GLenum imageFormat, const GLenum type, const GLint wrapMode, const GLint minMagFilter) -> GLuint {
		// use fast 4-byte alignment (default anyway) if possible
		if ((width * numComp) % 4 != 0) {
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		}

		// Create Texture
		GLuint texHandle;
		glGenTextures(1, &texHandle);
		glBindTexture(GL_TEXTURE_2D, texHandle);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, imageFormat, type, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minMagFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, minMagFilter);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

		glBindTexture(GL_TEXTURE_2D, 0);

		return texHandle;
		};

	// elevation map
	GLuint elevationTexHandle = createTexture(td->m_elevationMap, 4, td->m_elevationMapWidth, td->m_elevationMapHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_CLAMP_TO_EDGE, GL_NEAREST);
	this->m_terrainSO->setElevationTextureHandle(elevationTexHandle);
	// normal map
	GLuint normalTexHandle = createTexture(td->m_normalMap, 4, td->m_normalMapWidth, td->m_normalMapHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_CLAMP_TO_EDGE, GL_LINEAR);
	this->m_terrainSO->setNormalTextureHandle(normalTexHandle);
	// albedo map
	GLuint albedoTexHandle = createTexture(td->m_albedoMap, 4, td->m_albedoMapWidth, td->m_albedoMapHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_CLAMP_TO_EDGE, GL_LINEAR);
	this->m_terrainSO->setAlbedoTextureHandle(albedoTexHandle);

	glm::mat4 worldVtoElevationUVMat = glm::scale(glm::vec3(0.5 / chunkSize, 1.0, 0.5 / chunkSize));
	worldVtoElevationUVMat[3] = glm::vec4(0.5, 0.0, 0.5, 1.0);

	this->m_terrainSO->setWorldVertexToElevationMapUVMatrix(worldVtoElevationUVMat);

	for (int i = 0; i < this->m_numChunk; i++) {
		glm::quat quaternion = glm::quat(glm::radians(glm::vec3(0.0, 90.0 * i, 0.0)));
		this->m_chunkModelMats[i] = glm::toMat4(quaternion);
	}

	this->m_worldVtoElevationUVMat = worldVtoElevationUVMat;
}

void MyTerrain::updateState(const glm::mat4& viewMat, const glm::vec3& viewOrg, const glm::mat4& projMat, const glm::vec4* frustumPlaneEquations) {
	for (int i = 0; i < this->m_numChunk; i++) {
		this->m_chunkModelMats[i][3] = glm::vec4(viewOrg.x, 0.0, viewOrg.z, 1.0);
		this->m_terrainSO->setChunkTransformMatrix(i, this->m_chunkModelMats[i]);
	}

	// chunk culling test
	this->m_terrainSO->viewFrustumCullingTest(frustumPlaneEquations);
}

const glm::mat4 MyTerrain::worldVtoElevationUVMat() const {
	return this->m_worldVtoElevationUVMat;
}

const MyTerrainData* MyTerrain::terrainData() const {
	return this->m_terrainData;
}