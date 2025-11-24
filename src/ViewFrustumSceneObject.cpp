#define GLM_ENABLE_EXPERIMENTAL
#include "ViewFrustumSceneObject.h"

#include <glm\mat4x4.hpp>
#include <glm\gtx\transform.hpp>
#include <glm\gtc\type_ptr.hpp>



ViewFrustumSceneObject::ViewFrustumSceneObject(const int numCascade, const GLuint fsFunctionHandle, const int pixelProcessFuncBinding) : NUM_CASCADE(numCascade)
{
	// initialize dynamic scene object
	const int MAX_NUM_VERTEX = (numCascade + 1) * 4;
	int MAX_NUM_INDEX = numCascade * 24;
	
	if (this->m_lineMode) {
		MAX_NUM_INDEX = (numCascade * 4 + (numCascade + 1) * 4) * 2;
	}
	
	this->m_totalDataByte = MAX_NUM_VERTEX * 12;
	this->m_dynamicSO = new DynamicSceneObject(MAX_NUM_VERTEX, MAX_NUM_INDEX, false, false);
	

	// initialize index
	unsigned int *indexBuffer = this->m_dynamicSO->indexBuffer();
	unsigned int indexBufferOffset = 0;
	for (unsigned int i = 0; i < numCascade + 1; i++) {
		const unsigned int currLayerStartIdx = i * 4;
		const unsigned int nextLayerStartIdx = (i + 1) * 4;			

		// z-direction line
		if (i < numCascade) {
			for (unsigned int j = 0; j < 4; j++) {
				indexBuffer[indexBufferOffset + 0] = nextLayerStartIdx + j;
				indexBuffer[indexBufferOffset + 1] = currLayerStartIdx + j;
				indexBufferOffset = indexBufferOffset + 2;
			}
		}			

		// x-direction line
		for (unsigned int j = 0; j < 3; j++) {
			indexBuffer[indexBufferOffset + 0] = currLayerStartIdx + j;
			indexBuffer[indexBufferOffset + 1] = currLayerStartIdx + j + 1;
			indexBufferOffset = indexBufferOffset + 2;
		}
		indexBuffer[indexBufferOffset + 0] = currLayerStartIdx + 3;
		indexBuffer[indexBufferOffset + 1] = currLayerStartIdx + 0;
		indexBufferOffset = indexBufferOffset + 2;
	}

	this->m_dynamicSO->updateIndexBuffer(0, MAX_NUM_INDEX * 4);
	
	this->m_dynamicSO->setPrimitive(GL_LINES);
	this->m_dynamicSO->setPixelFunctionId(SceneManager::Instance()->m_fs_pureColor);		
}


ViewFrustumSceneObject::~ViewFrustumSceneObject()
{
}

void ViewFrustumSceneObject::updateDataBuffer() {
	this->m_dynamicSO->updateDataBuffer(0, this->m_totalDataByte);
}

DynamicSceneObject *ViewFrustumSceneObject::sceneObject() const {
	return this->m_dynamicSO;
}

float *ViewFrustumSceneObject::cascadeDataBuffer(const int layerIdx) {
	float *dataBuffer = this->m_dynamicSO->dataBuffer();
	return dataBuffer + layerIdx * 12;
}

void ViewFrustumSceneObject::updateState(const glm::mat4 &viewMat, const glm::vec3 &viewPos) {
	glm::mat4 tMat = glm::translate(viewPos);
	glm::mat4 viewT = glm::transpose(viewMat);
	glm::vec4 forward = -1.0f * glm::vec4(viewT[2].x, viewT[2].y, viewT[2].z, 0.0);
	glm::vec4 x = -1.0f * glm::vec4(viewT[0].x, viewT[0].y, viewT[0].z, 0.0);
	glm::vec4 y = glm::vec4(viewT[1].x, viewT[1].y, viewT[1].z, 0.0);

	glm::mat4 rMat;
	rMat[0] = x;
	rMat[1] = y;
	rMat[2] = forward;
	rMat[3] = glm::vec4(0.0, 0.0, 0.0, 1.0);

	this->m_dynamicSO->setModelMat(tMat * rMat);
}

