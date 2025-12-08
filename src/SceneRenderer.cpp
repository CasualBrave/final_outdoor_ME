#include "SceneRenderer.h"


SceneRenderer::SceneRenderer()
{
}


SceneRenderer::~SceneRenderer()
{
	this->destroyGBuffer();
	if (this->m_screenVAO != 0) {
		glDeleteVertexArrays(1, &this->m_screenVAO);
		glDeleteBuffers(1, &this->m_screenVBO);
		glDeleteBuffers(1, &this->m_screenEBO);
	}
	if (this->m_displayProgram != nullptr) {
		delete this->m_displayProgram;
		this->m_displayProgram = nullptr;
	}
}
void SceneRenderer::startNewFrame() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	this->clear();
}
void SceneRenderer::renderPass(int gbufferDisplayMode){
	this->m_gbufferDisplayMode = gbufferDisplayMode;
	this->renderGeometryPass();
	this->renderDisplayPass();
}

// =======================================
void SceneRenderer::resize(const int w, const int h){
	this->m_frameWidth = w;
	this->m_frameHeight = h;
	this->destroyGBuffer();
	this->createGBuffer(w, h);
}
bool SceneRenderer::initialize(const int w, const int h, ShaderProgram* shaderProgram){
	this->m_shaderProgram = shaderProgram;

	this->resize(w, h);
	const bool flag = this->setUpShader();
	
	if (!flag) {
		return false;
	}	
	if (!this->setUpDisplayShader()) {
		return false;
	}
	this->ensureScreenQuad();
	
	glEnable(GL_DEPTH_TEST);

	return true;
}
void SceneRenderer::setProjection(const glm::mat4 &proj){
	this->m_projMat = proj;
}
void SceneRenderer::setView(const glm::mat4 &view){
	this->m_viewMat = view;
}
void SceneRenderer::setViewport(const int x, const int y, const int w, const int h) {
	glViewport(x, y, w, h);
	this->m_curViewportX = x;
	this->m_curViewportY = y;
	this->m_curViewportW = w;
	this->m_curViewportH = h;
}
void SceneRenderer::appendDynamicSceneObject(DynamicSceneObject *obj){
	this->m_dynamicSOs.push_back(obj);
}
void SceneRenderer::appendTerrainSceneObject(TerrainSceneObject* tSO) {
	this->m_terrainSO = tSO;
}
void SceneRenderer::clear(const glm::vec4 &clearColor, const float depth){
	static const float COLOR[] = { 0.0, 0.0, 0.0, 1.0 };
	static const float DEPTH[] = { 1.0 };

	glClearBufferfv(GL_COLOR, 0, COLOR);
	glClearBufferfv(GL_DEPTH, 0, DEPTH);
}
bool SceneRenderer::setUpShader(){
	if (this->m_shaderProgram == nullptr) {
		return false;
	}

	this->m_shaderProgram->useProgram();

	// shader attributes binding
	const GLuint programId = this->m_shaderProgram->programId();

	SceneManager *manager = SceneManager::Instance();
	manager->m_vertexHandle = 0;
	manager->m_normalHandle = 1;
	manager->m_tangentHandle = 2;
	manager->m_uvHandle = 3;

	// =================================
	manager->m_modelMatHandle = 0;
	manager->m_viewMatHandle = 7;
	manager->m_projMatHandle = 8;
	manager->m_terrainVToUVMatHandle = 9;
	manager->m_lightDirWorldHandle = 17;
	manager->m_cameraPosHandle = 18;
	manager->m_materialAmbientHandle = 11;
	manager->m_materialSpecularHandle = 12;
	manager->m_materialShininessHandle = 13;
	manager->m_useNormalMapHandle = 14;
	manager->m_fs_normalTexHandle = 15;

	manager->m_albedoMapHandle = 4;
	manager->m_albedoMapTexIdx = 0;
	glUniform1i(manager->m_albedoMapHandle, manager->m_albedoMapTexIdx);

	manager->m_elevationMapHandle = 5;
	manager->m_elevationMapTexIdx = 3;
	glUniform1i(manager->m_elevationMapHandle, manager->m_elevationMapTexIdx);
	
	manager->m_normalMapHandle = 6;
	manager->m_normalMapTexIdx = 2;
	glUniform1i(manager->m_normalMapHandle, manager->m_normalMapTexIdx);
	// fragment normal sampler (e.g., magic stone)
	manager->m_fs_normalTexIdx = 2;
	glUniform1i(manager->m_fs_normalTexHandle, manager->m_fs_normalTexIdx);
	
	manager->m_albedoTexUnit = GL_TEXTURE0;
	manager->m_elevationTexUnit = GL_TEXTURE3;
	manager->m_normalTexUnit = GL_TEXTURE2;

	manager->m_vs_vertexProcessIdHandle = 1;
	manager->m_vs_commonProcess = 0;
	manager->m_vs_terrainProcess = 3;

	manager->m_fs_pixelProcessIdHandle = 2;
	manager->m_fs_pureColor = 5;
	manager->m_fs_texturePass = 6;
	manager->m_fs_terrainPass = 7;
	
	return true;
}

bool SceneRenderer::setUpDisplayShader() {
	Shader* vs = new Shader(GL_VERTEX_SHADER);
	vs->createShaderFromFile("shaders\\gbufferDisplayVertex.glsl");
	Shader* fs = new Shader(GL_FRAGMENT_SHADER);
	fs->createShaderFromFile("shaders\\gbufferDisplayFragment.glsl");

	this->m_displayProgram = new ShaderProgram();
	this->m_displayProgram->init();
	this->m_displayProgram->attachShader(vs);
	this->m_displayProgram->attachShader(fs);
	this->m_displayProgram->checkStatus();
	this->m_displayProgram->linkProgram();

	vs->releaseShader();
	fs->releaseShader();
	delete vs;
	delete fs;

	this->m_displayProgram->useProgram();
	// sampler bindings
	glUniform1i(0, 0); // gPositionTex -> unit0
	glUniform1i(1, 1); // gNormalTex   -> unit1
	glUniform1i(2, 2); // gAmbientTex  -> unit2
	glUniform1i(3, 3); // gDiffuseTex  -> unit3
	glUniform1i(4, 4); // gSpecularTex -> unit4
	this->m_displayModeHandle = 5;
	this->m_displayUVScaleHandle = 6;
	this->m_displayUVBiasHandle = 7;
	this->m_displayLightDirHandle = 8;
	this->m_displayCamPosHandle = 9;
	this->m_displayViewMatHandle = 10;

	return true;
}

bool SceneRenderer::createGBuffer(const int w, const int h) {
	// create FBO
	glGenFramebuffers(1, &this->m_gbufferFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, this->m_gbufferFBO);

	// attachments formats
	GLenum attachments[5] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2,
		GL_COLOR_ATTACHMENT3,
		GL_COLOR_ATTACHMENT4
	};
	// pos/normal use 16F, others 8-bit
	GLint formats[5] = {
		GL_RGBA16F, // world pos
		GL_RGBA16F, // world normal
		GL_RGBA8,   // ambient
		GL_RGBA8,   // diffuse
		GL_RGBA8    // specular
	};

	for (int i = 0; i < 5; ++i) {
		glGenTextures(1, &this->m_gbufferTextures[i]);
		glBindTexture(GL_TEXTURE_2D, this->m_gbufferTextures[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, formats[i], w, h, 0, GL_RGBA, (i < 2) ? GL_FLOAT : GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachments[i], GL_TEXTURE_2D, this->m_gbufferTextures[i], 0);
	}

	// depth buffer
	glGenRenderbuffers(1, &this->m_gbufferDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, this->m_gbufferDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, this->m_gbufferDepth);

	glDrawBuffers(5, attachments);
	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return status == GL_FRAMEBUFFER_COMPLETE;
}

void SceneRenderer::destroyGBuffer() {
	if (this->m_gbufferDepth != 0) {
		glDeleteRenderbuffers(1, &this->m_gbufferDepth);
		this->m_gbufferDepth = 0;
	}
	for (int i = 0; i < 5; ++i) {
		if (this->m_gbufferTextures[i] != 0) {
			glDeleteTextures(1, &this->m_gbufferTextures[i]);
			this->m_gbufferTextures[i] = 0;
		}
	}
	if (this->m_gbufferFBO != 0) {
		glDeleteFramebuffers(1, &this->m_gbufferFBO);
		this->m_gbufferFBO = 0;
	}
}

void SceneRenderer::renderGeometryPass() {
	SceneManager *manager = SceneManager::Instance();	

	glBindFramebuffer(GL_FRAMEBUFFER, this->m_gbufferFBO);
	GLenum attachments[5] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2,
		GL_COLOR_ATTACHMENT3,
		GL_COLOR_ATTACHMENT4
	};
	glDrawBuffers(5, attachments);
	const float CLEAR_COLOR[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	for (int i = 0; i < 5; ++i) {
		glClearBufferfv(GL_COLOR, i, CLEAR_COLOR);
	}
	const float DEPTH[] = { 1.0f };
	glClearBufferfv(GL_DEPTH, 0, DEPTH);

	this->m_shaderProgram->useProgram();
	glUniformMatrix4fv(manager->m_projMatHandle, 1, false, glm::value_ptr(this->m_projMat));
	glUniformMatrix4fv(manager->m_viewMatHandle, 1, false, glm::value_ptr(this->m_viewMat));
	// directional light in world space
	const glm::vec3 lightDirWorld = glm::normalize(glm::vec3(0.4f, 0.5f, 0.8f));
	glUniform3fv(manager->m_lightDirWorldHandle, 1, glm::value_ptr(lightDirWorld));
	// camera position in world (inverse of viewMat)
	glm::mat4 invView = glm::inverse(this->m_viewMat);
	glm::vec3 camPos = glm::vec3(invView[3]);
	glUniform3fv(manager->m_cameraPosHandle, 1, glm::value_ptr(camPos));

	if (this->m_terrainSO != nullptr) {
		glUniform1i(SceneManager::Instance()->m_vs_vertexProcessIdHandle, SceneManager::Instance()->m_vs_terrainProcess);
		this->m_terrainSO->update();
	}

	if (this->m_dynamicSOs.size() > 0) {
		glUniform1i(SceneManager::Instance()->m_vs_vertexProcessIdHandle, SceneManager::Instance()->m_vs_commonProcess);
		for (DynamicSceneObject *obj : this->m_dynamicSOs) {
			obj->update();
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneRenderer::ensureScreenQuad() {
	if (this->m_screenVAO != 0) { return; }
	const float quad[] = {
		// pos      // uv
		-1.0f, -1.0f, 0.0f, 0.0f,
		 1.0f, -1.0f, 1.0f, 0.0f,
		 1.0f,  1.0f, 1.0f, 1.0f,
		-1.0f,  1.0f, 0.0f, 1.0f
	};
	const unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };

	glGenVertexArrays(1, &this->m_screenVAO);
	glGenBuffers(1, &this->m_screenVBO);
	glGenBuffers(1, &this->m_screenEBO);

	glBindVertexArray(this->m_screenVAO);
	glBindBuffer(GL_ARRAY_BUFFER, this->m_screenVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_screenEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
}

void SceneRenderer::renderDisplayPass() {
	if (this->m_displayProgram == nullptr) { return; }
	this->m_displayProgram->useProgram();
	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, this->m_gbufferTextures[0]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, this->m_gbufferTextures[1]);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, this->m_gbufferTextures[2]);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, this->m_gbufferTextures[3]);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, this->m_gbufferTextures[4]);

	glUniform1i(this->m_displayModeHandle, this->m_gbufferDisplayMode);
	const float uvScaleX = (this->m_frameWidth  > 0) ? (float)this->m_curViewportW / (float)this->m_frameWidth  : 1.0f;
	const float uvScaleY = (this->m_frameHeight > 0) ? (float)this->m_curViewportH / (float)this->m_frameHeight : 1.0f;
	const float uvBiasX  = (this->m_frameWidth  > 0) ? (float)this->m_curViewportX / (float)this->m_frameWidth  : 0.0f;
	const float uvBiasY  = (this->m_frameHeight > 0) ? (float)this->m_curViewportY / (float)this->m_frameHeight : 0.0f;
	glUniform2f(this->m_displayUVScaleHandle, uvScaleX, uvScaleY);
	glUniform2f(this->m_displayUVBiasHandle, uvBiasX, uvBiasY);

	// light & camera for default lighting view
	const glm::vec3 lightDirWorld = glm::normalize(glm::vec3(0.4f, 0.5f, 0.8f));
	glm::mat4 invView = glm::inverse(this->m_viewMat);
	glm::vec3 camPos = glm::vec3(invView[3]);
	glUniform3fv(this->m_displayLightDirHandle, 1, glm::value_ptr(lightDirWorld));
	glUniform3fv(this->m_displayCamPosHandle, 1, glm::value_ptr(camPos));
	glUniformMatrix4fv(this->m_displayViewMatHandle, 1, GL_FALSE, glm::value_ptr(this->m_viewMat));

	glBindVertexArray(this->m_screenVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	glEnable(GL_DEPTH_TEST);
}
