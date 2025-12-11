#pragma once

#include <vector>
#include "Shader.h"
#include "SceneManager.h"
#include "DynamicSceneObject.h"
#include "terrain\TerrainSceneObject.h"
#include "MyPoissonSample.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct InstanceDataGPU {
	glm::mat4 model;
	glm::vec4 sphere; // xyz center (world), w radius
};

struct InstanceBatch {
	std::string name;
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ebo = 0;
	GLsizei indexCount = 0;
	GLuint texture = 0;
	glm::vec3 materialAmbient = glm::vec3(1.0f);
	glm::vec3 materialSpecular = glm::vec3(0.0f);
	float materialShininess = 1.0f;

	GLuint instanceBuffer = 0;
	GLuint visibleIndexBuffer = 0;
	GLuint indirectBuffer = 0;
	uint32_t numInstances = 0;
	glm::vec3 sphereCenter = glm::vec3(0.0f);
	float sphereRadius = 1.0f;
};

class SceneRenderer
{
public:
	SceneRenderer();
	virtual ~SceneRenderer();

private:
	ShaderProgram *m_shaderProgram = nullptr;
	glm::mat4 m_projMat;
	glm::mat4 m_viewMat;
	int m_frameWidth;
	int m_frameHeight;	

	std::vector<DynamicSceneObject*> m_dynamicSOs;
	TerrainSceneObject* m_terrainSO = nullptr;

	// deferred g-buffer
	GLuint m_gbufferFBO = 0;
	GLuint m_gbufferTextures[5] = { 0, 0, 0, 0, 0 }; // pos, normal, ambient, diffuse, specular
	GLuint m_gbufferDepth = 0;

	// display pass
	ShaderProgram* m_displayProgram = nullptr;
	GLint m_displayModeHandle = -1;
	GLint m_displayLightDirHandle = -1;
	GLint m_displayCamPosHandle = -1;
	GLint m_displayViewMatHandle = -1;
	GLuint m_screenVAO = 0;
	GLuint m_screenVBO = 0;
	GLuint m_screenEBO = 0;
	int m_gbufferDisplayMode = 5; // default diffuse (original look)
	GLint m_displayUVScaleHandle = -1;
	GLint m_displayUVBiasHandle = -1;
	int m_curViewportX = 0;
	int m_curViewportY = 0;
	int m_curViewportW = 0;
	int m_curViewportH = 0;

	// gpu-driven instancing
	std::vector<InstanceBatch> m_instanceBatches;
	ShaderProgram* m_cullProgram = nullptr;
	GLint m_cullNumInstancesHandle = -1;
	GLint m_cullVPHandle = -1;
	glm::mat4 m_cullVP = glm::mat4(1.0f);

public:
	void resize(const int w, const int h);
	bool initialize(const int w, const int h, ShaderProgram* shaderProgram);

	void setProjection(const glm::mat4 &proj);
	void setView(const glm::mat4 &view);
	void setViewport(const int x, const int y, const int w, const int h);
	void setCullingVP(const glm::mat4& vp) { m_cullVP = vp; }
	void appendDynamicSceneObject(DynamicSceneObject *obj);
	void appendTerrainSceneObject(TerrainSceneObject* tSO);

// pipeline
public:
	void startNewFrame();
	void renderPass(int gbufferDisplayMode);
	void setGBufferDisplayMode(int mode) { m_gbufferDisplayMode = mode; }

private:
	void clear(const glm::vec4 &clearColor = glm::vec4(0.0, 0.0, 0.0, 1.0), const float depth = 1.0);
	bool setUpShader();
	bool createGBuffer(const int w, const int h);
	void destroyGBuffer();
	bool setUpDisplayShader();
	void renderGeometryPass();
	void renderDisplayPass();
	void ensureScreenQuad();
	void setUpInstanceBatches();
	void renderInstanceBatches();
	void dispatchCulling(struct InstanceBatch& batch);
	void updateFrustumPlanes();
	GLuint loadTexture(const std::string& path);
};
