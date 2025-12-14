#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "SceneManager.h"

class DynamicSceneObject
{
public:
	DynamicSceneObject(const int maxNumVertex, const int maxNumIndex, const bool normalFlag, const bool uvFlag);
	virtual ~DynamicSceneObject();

	void update();

	float* dataBuffer();
	unsigned int* indexBuffer();

	void updateDataBuffer(const int byteOffset, const int dataByte);
	void updateIndexBuffer(const int byteOffset, const int dataByte);

	void setPixelFunctionId(const int functionId);
	void setPrimitive(const GLenum primitive);
	void setModelMat(const glm::mat4& modelMat);
	void setAlbedoTexture(const GLuint texHandle);
	void setMaterial(const glm::vec3& ambient, const glm::vec3& specular, const float shininess);
	void setNormalTexture(const GLuint texHandle);
	static void setGlobalNormalMapToggle(const bool flag);

	// Minimal accessors for special rendering passes (e.g., shadow map).
	GLuint vao() const { return m_vao; }
	GLenum primitive() const { return m_primitive; }
	int indexCount() const { return m_indexCount; }
	int pixelFunctionId() const { return m_pixelFunctionId; }
	const glm::mat4& modelMat() const { return m_modelMat; }

private:
	GLuint m_indexBufferHandle;
	float* m_dataBuffer = nullptr;
	unsigned int* m_indexBuffer = nullptr;

	GLuint m_vao;
	GLuint m_dataBufferHandle;
	GLenum m_primitive;
	int m_pixelFunctionId;
	int m_indexCount;

	glm::mat4 m_modelMat;
	bool m_useAlbedoTex = false;
	GLuint m_albedoTexHandle = 0;
	glm::vec3 m_materialAmbient = glm::vec3(1.0f);
	glm::vec3 m_materialSpecular = glm::vec3(0.0f);
	float m_materialShininess = 1.0f;

	bool m_useNormalTex = false;
	GLuint m_normalTexHandle = 0;

	static bool s_globalUseNormalMap;
};
