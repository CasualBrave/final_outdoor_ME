#include "DynamicSceneObject.h"


bool DynamicSceneObject::s_globalUseNormalMap = true;

DynamicSceneObject::DynamicSceneObject(const int maxNumVertex, const int maxNumIndex, const bool normalFlag, const bool uvFlag)
{
	// the data should be INTERLEAF format

	int strideV = 3; // position
	if (normalFlag == true) { strideV += 3; } // normal
	if (normalFlag == true) { strideV += 3; } // tangent
	if (uvFlag == true) { strideV += 2; } // uv is vec2
	const int totalBufferDataByte = maxNumVertex * strideV * 4;

	this->m_indexCount = maxNumIndex;

	this->m_dataBuffer = new float[totalBufferDataByte / 4];
	this->m_indexBuffer = new unsigned int[maxNumIndex];

	// Create Geometry Data Buffer
	glCreateBuffers(1, &(this->m_dataBufferHandle));
	glNamedBufferData(this->m_dataBufferHandle, totalBufferDataByte, this->m_dataBuffer, GL_DYNAMIC_DRAW);

	// Create Indices Buffer
	glCreateBuffers(1, &m_indexBufferHandle);
	glNamedBufferData(m_indexBufferHandle, maxNumIndex * 4, this->m_indexBuffer, GL_DYNAMIC_DRAW);

	// create vao
	glGenVertexArrays(1, &(this->m_vao));
	glBindVertexArray(this->m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, this->m_dataBufferHandle);
	int byteOffset = 0;
	glVertexAttribPointer(SceneManager::Instance()->m_vertexHandle, 3, GL_FLOAT, false, strideV * 4, (void*)(byteOffset));
	byteOffset += 12;
	glEnableVertexAttribArray(SceneManager::Instance()->m_vertexHandle);
	if (normalFlag) {
		glVertexAttribPointer(SceneManager::Instance()->m_normalHandle, 3, GL_FLOAT, false, strideV * 4, (void*)(byteOffset));
		byteOffset += 12;
		glEnableVertexAttribArray(SceneManager::Instance()->m_normalHandle);
		// tangent follows normal
		glVertexAttribPointer(SceneManager::Instance()->m_tangentHandle, 3, GL_FLOAT, false, strideV * 4, (void*)(byteOffset));
		byteOffset += 12;
		glEnableVertexAttribArray(SceneManager::Instance()->m_tangentHandle);
	}
	if (uvFlag) {
		glVertexAttribPointer(SceneManager::Instance()->m_uvHandle, 2, GL_FLOAT, false, strideV * 4, (void*)(byteOffset));
		byteOffset += 8;
		glEnableVertexAttribArray(SceneManager::Instance()->m_uvHandle);
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferHandle);
	glBindVertexArray(0);
}


DynamicSceneObject::~DynamicSceneObject()
{
	if (this->m_useAlbedoTex && this->m_albedoTexHandle != 0) {
		glDeleteTextures(1, &this->m_albedoTexHandle);
	}
	if (this->m_useNormalTex && this->m_normalTexHandle != 0) {
		glDeleteTextures(1, &this->m_normalTexHandle);
	}
	delete[] this->m_dataBuffer;
	delete[] this->m_indexBuffer;
}

void DynamicSceneObject::update() {
	// bind Buffer
	glBindVertexArray(this->m_vao);
	// model matrix
	glUniformMatrix4fv(SceneManager::Instance()->m_modelMatHandle, 1, false, glm::value_ptr(this->m_modelMat));

	if (this->m_useAlbedoTex) {
		glActiveTexture(SceneManager::Instance()->m_albedoTexUnit);
		glBindTexture(GL_TEXTURE_2D, this->m_albedoTexHandle);
	}
	int useNormalFlag = 0;
	if (this->m_useNormalTex && DynamicSceneObject::s_globalUseNormalMap) {
		glActiveTexture(SceneManager::Instance()->m_normalTexUnit);
		glBindTexture(GL_TEXTURE_2D, this->m_normalTexHandle);
		useNormalFlag = 1;
	}
	// set sampler for fragment shader normal map (separate handle)
	glUniform1i(SceneManager::Instance()->m_fs_normalTexHandle, SceneManager::Instance()->m_fs_normalTexIdx);
	glUniform1i(SceneManager::Instance()->m_useNormalMapHandle, useNormalFlag);

	glUniform3fv(SceneManager::Instance()->m_materialAmbientHandle, 1, glm::value_ptr(this->m_materialAmbient));
	glUniform3fv(SceneManager::Instance()->m_materialSpecularHandle, 1, glm::value_ptr(this->m_materialSpecular));
	glUniform1f(SceneManager::Instance()->m_materialShininessHandle, this->m_materialShininess);

	glUniform1i(SceneManager::Instance()->m_fs_pixelProcessIdHandle, this->m_pixelFunctionId);
	glDrawElements(this->m_primitive, this->m_indexCount, GL_UNSIGNED_INT, nullptr);
}

float* DynamicSceneObject::dataBuffer() { return this->m_dataBuffer; }
unsigned int *DynamicSceneObject::indexBuffer() { return this->m_indexBuffer; }

void DynamicSceneObject::updateDataBuffer(const int byteOffset, const int dataByte) { 
	float *data = this->m_dataBuffer + byteOffset / 4;
	glNamedBufferSubData(this->m_dataBufferHandle, byteOffset, dataByte, data); 
}
void DynamicSceneObject::updateIndexBuffer(const int byteOffset, const int dataByte) { 
	unsigned int *data = this->m_indexBuffer + byteOffset / 4;
	glNamedBufferSubData(this->m_indexBufferHandle, byteOffset, dataByte, data); 
}

void DynamicSceneObject::setPixelFunctionId(const int functionId){
	this->m_pixelFunctionId = functionId;
}
void DynamicSceneObject::setPrimitive(const GLenum primitive) {
	this->m_primitive = primitive;
}
void DynamicSceneObject::setModelMat(const glm::mat4& modelMat){
	this->m_modelMat = modelMat;
}
void DynamicSceneObject::setAlbedoTexture(const GLuint texHandle) {
	this->m_albedoTexHandle = texHandle;
	this->m_useAlbedoTex = (texHandle != 0);
}
void DynamicSceneObject::setMaterial(const glm::vec3& ambient, const glm::vec3& specular, const float shininess) {
	this->m_materialAmbient = ambient;
	this->m_materialSpecular = specular;
	this->m_materialShininess = shininess;
}
void DynamicSceneObject::setNormalTexture(const GLuint texHandle) {
	this->m_normalTexHandle = texHandle;
	this->m_useNormalTex = (texHandle != 0);
}
void DynamicSceneObject::setGlobalNormalMapToggle(const bool flag) {
	DynamicSceneObject::s_globalUseNormalMap = flag;
}
