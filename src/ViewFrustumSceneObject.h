#pragma once

#include "DynamicSceneObject.h"

class ViewFrustumSceneObject
{
public:
	ViewFrustumSceneObject(const int numCascade, const GLuint fsFunctionHandle, const int pixelProcessFuncBinding);
	virtual ~ViewFrustumSceneObject();

	DynamicSceneObject *sceneObject() const;

public:
	float *cascadeDataBuffer(const int layerIdx);
	void updateDataBuffer();
	void updateState(const glm::mat4 &viewMat, const glm::vec3 &viewPos);

private:
	const int NUM_CASCADE;
	DynamicSceneObject *m_dynamicSO;
	int m_totalDataByte;

	bool m_lineMode = true;
};

