#include "TerrainSceneObject.h"



TerrainSceneObject::TerrainSceneObject(const int numChunk, const float* chunkVertices, const int numChunkVertex, const unsigned int* chunkIndices, const int numChunkIndex) :
	m_numChunk(numChunk)
{
	this->initializeChunkGeometry(chunkVertices, numChunkVertex, chunkIndices, numChunkIndex);
	
	this->m_chunkModelMats = new glm::mat4[this->m_numChunk];
	//this->m_chunkBoundingVolumes = new IBoundingVolume*[this->m_numChunk];
	this->m_chunkVisibilityFlags = new bool[this->m_numChunk];
	for (int i = 0; i < this->m_numChunk; i++) {
		this->m_chunkModelMats[i] = glm::mat4(1.0);
		//this->m_chunkBoundingVolumes[i] = nullptr;
		this->m_chunkVisibilityFlags[i] = true;
	}
}


TerrainSceneObject::~TerrainSceneObject()
{
	for (int i = 0; i < this->m_numChunk; i++) {
		//delete this->m_chunkBoundingVolumes[i];
	}

	delete[] this->m_chunkModelMats;
	//delete[] this->m_chunkBoundingVolumes;
	delete[] this->m_chunkVisibilityFlags;
}

void TerrainSceneObject::setElevationTextureHandle(const GLuint texHandle) {
	this->m_evelationMapHandle = texHandle;
}
void TerrainSceneObject::setNormalTextureHandle(const GLuint texHandle){
	this->m_normalMapHandle = texHandle;
}
void TerrainSceneObject::setAlbedoTextureHandle(const GLuint texHandle){
	this->m_albedoMapHandle = texHandle;
}

void TerrainSceneObject::update() {
	// bind Buffer
	glBindVertexArray(this->m_vao);

	glActiveTexture(SceneManager::Instance()->m_elevationTexUnit);
	glBindTexture(GL_TEXTURE_2D, this->m_evelationMapHandle);

	glActiveTexture(SceneManager::Instance()->m_normalTexUnit);
	glBindTexture(GL_TEXTURE_2D, this->m_normalMapHandle);

	glActiveTexture(SceneManager::Instance()->m_albedoTexUnit);
	glBindTexture(GL_TEXTURE_2D, this->m_albedoMapHandle);

	glUniformMatrix4fv(SceneManager::Instance()->m_terrainVToUVMatHandle, 1, false, glm::value_ptr(this->m_worldVertexToElevationMapUvMat));

	glUniform1i(SceneManager::Instance()->m_fs_pixelProcessIdHandle, SceneManager::Instance()->m_fs_terrainPass);

	// render several chunks
	for (int i = 0; i < this->m_numChunk; i++) {
		if (this->m_chunkVisibilityFlags[i] == false) {
			continue;
		}

		glUniformMatrix4fv(SceneManager::Instance()->m_modelMatHandle, 1, false, glm::value_ptr(this->m_chunkModelMats[i]));

		int indicesPointer = 0;
		glDrawElements(GL_TRIANGLES, this->m_numIndex, GL_UNSIGNED_INT, (GLvoid*)(indicesPointer));
	}
}

void TerrainSceneObject::initializeChunkGeometry(const float* chunkVertices, const int numChunkVertex, const unsigned int* chunkIndices, const int numChunkIndex) {
	// Create Geometry Data Buffer
	GLuint dataBufferHandle;
	glCreateBuffers(1, &dataBufferHandle);
	glNamedBufferData(dataBufferHandle, numChunkVertex * 12, chunkVertices, GL_STATIC_DRAW);

	// Create Indices Buffer
	GLuint indexBufferHandle;
	glCreateBuffers(1, &indexBufferHandle);
	glNamedBufferData(indexBufferHandle, numChunkIndex * 4, chunkIndices, GL_STATIC_DRAW);

	this->m_numIndex = numChunkIndex;

	// create VAO
	glGenVertexArrays(1, &(this->m_vao));
	glBindVertexArray(this->m_vao);
	// only vertex
	glBindBuffer(GL_ARRAY_BUFFER, dataBufferHandle);
	glVertexAttribPointer(SceneManager::Instance()->m_vertexHandle, 3, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(SceneManager::Instance()->m_vertexHandle);
	// bind index
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferHandle);
	// done
	glBindVertexArray(0);
}
/*
void TerrainSceneObject::setChunkBoundingVolume(const int idx, IBoundingVolume* bv) {
	this->m_chunkBoundingVolumes[idx] = bv;
}
*/
void TerrainSceneObject::setChunkTransformMatrix(const int idx, const glm::mat4& m) {
	this->m_chunkModelMats[idx] = m;
}
void TerrainSceneObject::setWorldVertexToElevationMapUVMatrix(const glm::mat4& m) {
	this->m_worldVertexToElevationMapUvMat = m;
}
bool TerrainSceneObject::viewFrustumCullingTest(const glm::vec4* frustumPlaneEquations) {
	/*
	for (int i = 0; i < this->m_numChunk; i++) {
		// apply transform
		this->m_chunkBoundingVolumes[i]->transform(this->m_chunkModelMats[i]);
		this->m_chunkVisibilityFlags[i] = !this->m_chunkBoundingVolumes[i]->viewFrustumCulling(frustumPlaneEquations);
	}
	*/

	return false;
}
