#pragma once

#include <vector>
#include "Shader.h"
#include "SceneManager.h"
#include "DynamicSceneObject.h"
#include "terrain/TerrainSceneObject.h"
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
	bool useOcclusion = true; // foliage only
	bool isOccluder = true;   // rendered before HZB build
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
	GLuint m_gbufferDepthTex = 0; // depth texture for HZB
	// viewport-sized depth texture for mipmap visualization (avoids mixing with cleared outside-viewport depth)
	GLuint m_depthVizTex = 0;
	GLuint m_depthVizFBO = 0;
	int m_depthVizW = 0;
	int m_depthVizH = 0;
	int m_depthVizLevels = 1;
	// max-reduced depth pyramid for visualization (R32F)
	GLuint m_depthVizPyramidTex = 0;
	ShaderProgram* m_depthVizProgram = nullptr;
	GLint m_depthVizDstLevelHandle = -1;
	GLint m_depthVizSrcLevelHandle = -1;
	GLuint m_depthPyramidTex = 0; // R32F pyramid for occlusion
	int m_depthNumLevels = 1;
	int m_depthFixedLevel = 0;
	bool m_hzbBuiltThisFrame = false;
	ShaderProgram* m_hzbProgram = nullptr;
	GLint m_hzbSrcLevelHandle = -1;
	GLint m_hzbDstLevelHandle = -1;

	// display pass
	ShaderProgram* m_displayProgram = nullptr;
	GLint m_displayModeHandle = -1;
	GLint m_displayLightDirHandle = -1;
	GLint m_displayCamPosHandle = -1;
	GLint m_displayViewMatHandle = -1;
	GLint m_displayDepthMipLevelHandle = -1;
	GLint m_displayInvProjHandle = -1;
	GLuint m_screenVAO = 0;
	GLuint m_screenVBO = 0;
	GLuint m_screenEBO = 0;
	int m_gbufferDisplayMode = 5; // default diffuse (original look)
	int m_depthDisplayLevel = 0;
	GLint m_displayUVScaleHandle = -1;
	GLint m_displayUVBiasHandle = -1;
	bool m_depthVizEnabled = false;
	float m_depthVisFar = 512.0f;
	float m_depthVisGamma = 1.0f;
	int m_curViewportX = 0;
	int m_curViewportY = 0;
	int m_curViewportW = 0;
	int m_curViewportH = 0;
	// Region in the full-resolution G-buffer to sample from during display pass.
	int m_displaySampleViewportX = 0;
	int m_displaySampleViewportY = 0;
	int m_displaySampleViewportW = 0;
	int m_displaySampleViewportH = 0;

	// gpu-driven instancing
	std::vector<InstanceBatch> m_instanceBatches;
	ShaderProgram* m_cullProgram = nullptr;
	GLint m_cullNumInstancesHandle = -1;
	GLint m_cullFrustumHandle = -1;
	glm::mat4 m_cullVP = glm::mat4(1.0f);
	glm::mat4 m_cullView = glm::mat4(1.0f);
	glm::vec4 m_frustumPlanes[6];
	glm::vec4 m_cullPlanesOverride[6];
	bool m_hasCullPlanesOverride = false;
	glm::vec3 m_cullCamPos = glm::vec3(0.0f);
	bool m_cullDoneThisFrame = false;

public:
	void resize(const int w, const int h);
	bool initialize(const int w, const int h, ShaderProgram* shaderProgram);

	void setProjection(const glm::mat4 &proj);
	void setView(const glm::mat4 &view);
	void setViewport(const int x, const int y, const int w, const int h);
	// Override the region used for sampling G-buffer/depth during display pass (useful for split debug views).
	void setDisplaySampleViewport(const int x, const int y, const int w, const int h) {
		m_displaySampleViewportX = x;
		m_displaySampleViewportY = y;
		m_displaySampleViewportW = w;
		m_displaySampleViewportH = h;
	}
	void setCullingVP(const glm::mat4& vp) { m_cullVP = vp; }
	void setCullingView(const glm::mat4& view) { m_cullView = view; }
	void setCullingPlanes(const glm::vec4 planes[6]) {
		for (int i = 0; i < 6; ++i) m_cullPlanesOverride[i] = planes[i];
		m_hasCullPlanesOverride = true;
	}
	void clearCullingPlanesOverride() { m_hasCullPlanesOverride = false; }
	void setDepthDisplayLevel(const int level) { m_depthDisplayLevel = level; }
	void appendDynamicSceneObject(DynamicSceneObject *obj);
	void appendTerrainSceneObject(TerrainSceneObject* tSO);

// pipeline
public:
	void startNewFrame();
	void renderPass(int gbufferDisplayMode);
	void setGBufferDisplayMode(int mode) { m_gbufferDisplayMode = mode; }
	void renderDisplayOnly(int gbufferDisplayMode);
	void setDepthVizEnabled(const bool enabled) { m_depthVizEnabled = enabled; }
	void setDepthVisFar(const float farZ) { m_depthVisFar = farZ; }
	void setDepthVisGamma(const float gamma) { m_depthVisGamma = gamma; }

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
	void renderInstanceBatches(bool foliageOnly);
	void dispatchCulling(struct InstanceBatch& batch);
	GLuint loadTexture(const std::string& path);
	void createDepthPyramid(const int w, const int h);
	void destroyDepthPyramid();
	bool setUpHZBShader();
	void buildDepthPyramid();
	void ensureDepthVizTex(const int w, const int h);
	void destroyDepthVizTex();
	bool setUpDepthVizShader();
	void ensureDepthVizPyramid(const int w, const int h);
	void destroyDepthVizPyramid();
	void buildDepthVizPyramid();
};
