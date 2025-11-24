#pragma once

#include <glad\glad.h>

// Singleton

class SceneManager
{
private:
	SceneManager(){}
	

public:	

	virtual ~SceneManager() {}

	static SceneManager *Instance() {
		static SceneManager *m_instance = nullptr;
		if (m_instance == nullptr) {
			m_instance = new SceneManager();
		}
		return m_instance;
	}
	
	GLuint m_vertexHandle = 0;
	GLuint m_normalHandle = 0;
	GLuint m_uvHandle = 0;

	GLuint m_modelMatHandle = 0;
	GLuint m_viewMatHandle = 0;
	GLuint m_projMatHandle = 0;
	GLuint m_terrainVToUVMatHandle = 0;



	GLuint m_albedoMapHandle = 0;
	GLuint m_normalMapHandle = 0;
	GLuint m_elevationMapHandle = 0;
	
	GLuint m_fs_pixelProcessIdHandle = 0;
	GLuint m_vs_vertexProcessIdHandle = 0;

	GLenum m_albedoTexUnit = 0;
	GLenum m_normalTexUnit = 0;
	GLenum m_elevationTexUnit = 0;


	int m_albedoMapTexIdx = 0;
	int m_elevationMapTexIdx = 0;
	int m_normalMapTexIdx = 0;

	int m_vs_commonProcess = 0;
	int m_vs_terrainProcess = 0;	
	
	int m_fs_pureColor = 0;	
	int m_fs_terrainPass = 0;
	int m_fs_texturePass = 0;

	// lighting / material (Phong)
	GLuint m_lightDirHandle = 0;
	GLuint m_materialAmbientHandle = 0;
	GLuint m_materialSpecularHandle = 0;
	GLuint m_materialShininessHandle = 0;
	GLuint m_useNormalMapHandle = 0;

	// fragment shader normal sampler (for magic stone, etc.)
	GLuint m_fs_normalTexHandle = 0;
	int m_fs_normalTexIdx = 0;
};
