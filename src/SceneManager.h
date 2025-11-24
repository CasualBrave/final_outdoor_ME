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
	
	GLuint m_vertexHandle;
	GLuint m_normalHandle;
	GLuint m_uvHandle;

	GLuint m_modelMatHandle;
	GLuint m_viewMatHandle;
	GLuint m_projMatHandle;
	GLuint m_terrainVToUVMatHandle;



	GLuint m_albedoMapHandle;
	GLuint m_normalMapHandle;
	GLuint m_elevationMapHandle;
	
	GLuint m_fs_pixelProcessIdHandle;
	GLuint m_vs_vertexProcessIdHandle;

	GLenum m_albedoTexUnit;
	GLenum m_normalTexUnit;
	GLenum m_elevationTexUnit;


	int m_albedoMapTexIdx;
	int m_elevationMapTexIdx;
	int m_normalMapTexIdx;

	int m_vs_commonProcess;
	int m_vs_terrainProcess;	
	
	int m_fs_pureColor;	
	int m_fs_terrainPass;
};

