#pragma once

#include <vector>
#include "Shader.h"
#include "SceneManager.h"
#include "DynamicSceneObject.h"
#include "terrain\TerrainSceneObject.h"


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

public:
	void resize(const int w, const int h);
	bool initialize(const int w, const int h, ShaderProgram* shaderProgram);

	void setProjection(const glm::mat4 &proj);
	void setView(const glm::mat4 &view);
	void setViewport(const int x, const int y, const int w, const int h);
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
};
