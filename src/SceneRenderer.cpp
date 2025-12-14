#include "SceneRenderer.h"
#include "FrustumUtils.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

static void computeFrustumCornersWS(const glm::mat4& viewMat, const glm::mat4& projMat, float nearD, float farD, glm::vec3 outCorners[8]);


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
	this->destroyShadowResources();
	if (this->m_shadowProgram != nullptr) {
		delete this->m_shadowProgram;
		this->m_shadowProgram = nullptr;
	}
	if (this->m_depthVizProgram != nullptr) {
		delete this->m_depthVizProgram;
		this->m_depthVizProgram = nullptr;
	}
	if (this->m_hzbProgram != nullptr) {
		delete this->m_hzbProgram;
		this->m_hzbProgram = nullptr;
	}
	if (this->m_cullProgram != nullptr) {
		delete this->m_cullProgram;
		this->m_cullProgram = nullptr;
	}
	for (auto& b : this->m_instanceBatches) {
		if (b.instanceBuffer) glDeleteBuffers(1, &b.instanceBuffer);
		if (b.visibleIndexBuffer) glDeleteBuffers(1, &b.visibleIndexBuffer);
		if (b.indirectBuffer) glDeleteBuffers(1, &b.indirectBuffer);
		if (b.vao) glDeleteVertexArrays(1, &b.vao);
		if (b.vbo) glDeleteBuffers(1, &b.vbo);
		if (b.ebo) glDeleteBuffers(1, &b.ebo);
		if (b.texture) glDeleteTextures(1, &b.texture);
	}
}
void SceneRenderer::startNewFrame() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	this->clear();
	// allow culling dispatch once per frame
	this->m_cullDoneThisFrame = false;
	this->m_hzbBuiltThisFrame = false;
	this->m_shadowBuiltThisFrame = false;
}
void SceneRenderer::renderPass(int gbufferDisplayMode){
	this->m_gbufferDisplayMode = gbufferDisplayMode;
	this->renderGeometryPass();
	this->renderDisplayPass();
}

void SceneRenderer::renderDisplayOnly(int gbufferDisplayMode) {
	this->m_gbufferDisplayMode = gbufferDisplayMode;
	this->renderDisplayPass();
}

void SceneRenderer::renderPassReuseVisibility(int gbufferDisplayMode) {
	this->m_gbufferDisplayMode = gbufferDisplayMode;
	this->renderGeometryPass(false, false);
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
	if (!this->setUpHZBShader()) {
		return false;
	}
	if (!this->setUpDepthVizShader()) {
		return false;
	}
	if (!this->setUpShadowShader()) {
		return false;
	}
	this->ensureScreenQuad();
	this->setUpInstanceBatches();
	
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
	// default: sample the same region we render into
	this->m_displaySampleViewportX = x;
	this->m_displaySampleViewportY = y;
	this->m_displaySampleViewportW = w;
	this->m_displaySampleViewportH = h;
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
	manager->m_vs_instanceProcess = 4;

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
	glUniform1i(11, 5); // depthPyramid -> unit5
	glUniform1i(this->m_shadowMapHandle, 6); // shadowMap -> unit6
	this->m_displayModeHandle = 5;
	this->m_displayUVScaleHandle = 6;
	this->m_displayUVBiasHandle = 7;
	this->m_displayLightDirHandle = 8;
	this->m_displayCamPosHandle = 9;
	this->m_displayViewMatHandle = 10;
	this->m_displayDepthMipLevelHandle = 12;
	this->m_displayInvProjHandle = 13;

	return true;
}

bool SceneRenderer::setUpHZBShader() {
	Shader* cs = new Shader(GL_COMPUTE_SHADER);
	cs->createShaderFromFile("shaders\\hzbBuild.comp");
	this->m_hzbProgram = new ShaderProgram();
	this->m_hzbProgram->init();
	this->m_hzbProgram->attachShader(cs);
	this->m_hzbProgram->checkStatus();
	this->m_hzbProgram->linkProgram();
	cs->releaseShader();
	delete cs;

	this->m_hzbProgram->useProgram();
	this->m_hzbSrcLevelHandle = 0;
	this->m_hzbDstLevelHandle = 1;
	return true;
}

bool SceneRenderer::setUpShadowShader() {
	Shader* vs = new Shader(GL_VERTEX_SHADER);
	vs->createShaderFromFile("shaders\\shadowDepthVertex.glsl");
	Shader* fs = new Shader(GL_FRAGMENT_SHADER);
	fs->createShaderFromFile("shaders\\shadowDepthFragment.glsl");

	this->m_shadowProgram = new ShaderProgram();
	this->m_shadowProgram->init();
	this->m_shadowProgram->attachShader(vs);
	this->m_shadowProgram->attachShader(fs);
	this->m_shadowProgram->checkStatus();
	this->m_shadowProgram->linkProgram();

	vs->releaseShader();
	fs->releaseShader();
	delete vs;
	delete fs;
	return true;
}

void SceneRenderer::destroyShadowResources() {
	if (this->m_shadowTexArray != 0) {
		glDeleteTextures(1, &this->m_shadowTexArray);
		this->m_shadowTexArray = 0;
	}
	if (this->m_shadowFBO != 0) {
		glDeleteFramebuffers(1, &this->m_shadowFBO);
		this->m_shadowFBO = 0;
	}
}

void SceneRenderer::ensureShadowResources() {
	if (this->m_shadowFBO != 0 && this->m_shadowTexArray != 0) return;

	glGenFramebuffers(1, &this->m_shadowFBO);

	glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &this->m_shadowTexArray);
	glTextureStorage3D(this->m_shadowTexArray, 1, GL_DEPTH_COMPONENT32F, this->m_shadowMapSize, this->m_shadowMapSize, 3);
	glTextureParameteri(this->m_shadowTexArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(this->m_shadowTexArray, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(this->m_shadowTexArray, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTextureParameteri(this->m_shadowTexArray, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	const float border[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTextureParameterfv(this->m_shadowTexArray, GL_TEXTURE_BORDER_COLOR, border);
	glTextureParameteri(this->m_shadowTexArray, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTextureParameteri(this->m_shadowTexArray, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
}

static void computeFrustumCornersWS(const glm::mat4& viewMat, const glm::mat4& projMat, float nearD, float farD, glm::vec3 outCorners[8]) {
	// Build 8 corners for the sub-frustum slice [nearD, farD] in world space.
	glm::mat4 invView = glm::inverse(viewMat);
	glm::mat4 invProj = glm::inverse(projMat);
	const glm::vec2 ndcCorners[4] = {
		{-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}
	};
	for (int i = 0; i < 4; ++i) {
		glm::vec4 cornerVS4 = invProj * glm::vec4(ndcCorners[i].x, ndcCorners[i].y, 1.0f, 1.0f);
		cornerVS4 /= cornerVS4.w;
		glm::vec3 ray = glm::vec3(cornerVS4);
		float nearT = nearD / (-ray.z);
		float farT = farD / (-ray.z);
		glm::vec3 nearVS = ray * nearT;
		glm::vec3 farVS = ray * farT;
		outCorners[i + 0] = glm::vec3(invView * glm::vec4(nearVS, 1.0f));
		outCorners[i + 4] = glm::vec3(invView * glm::vec4(farVS, 1.0f));
	}
}

void SceneRenderer::updateShadowMatrices() {
	const glm::vec3 lightDirWorld = glm::normalize(glm::vec3(0.4f, 0.5f, 0.8f));
	const glm::vec3 forward = glm::normalize(-lightDirWorld);
	glm::vec3 up(0.0f, 1.0f, 0.0f);
	if (std::abs(glm::dot(forward, up)) > 0.99f) up = glm::vec3(0.0f, 0.0f, 1.0f);

	// Always base cascades on player view/projection (culling view/VP).
	const glm::mat4 playerView = this->m_cullView;
	const glm::mat4 playerProj = this->m_cullVP * glm::inverse(this->m_cullView);

	for (int c = 0; c < 3; ++c) {
		glm::vec3 cornersWS[8];
		computeFrustumCornersWS(playerView, playerProj, this->m_shadowCascadeNear[c], this->m_shadowCascadeFar[c], cornersWS);

		glm::vec3 center(0.0f);
		for (const auto& p : cornersWS) center += p;
		center *= (1.0f / 8.0f);

		const glm::mat4 lightView = glm::lookAt(center - forward * 1000.0f, center, up);
		glm::vec3 minLS(FLT_MAX), maxLS(-FLT_MAX);
		for (const auto& p : cornersWS) {
			glm::vec3 ls = glm::vec3(lightView * glm::vec4(p, 1.0f));
			minLS = glm::min(minLS, ls);
			maxLS = glm::max(maxLS, ls);
		}

		// Padding to reduce clipping & acne.
		const float padXY = 10.0f;
		minLS.x -= padXY; minLS.y -= padXY;
		maxLS.x += padXY; maxLS.y += padXY;
		const float padZ = 50.0f;

		// Stabilize by snapping the ortho center to texel size.
		const float extentX = maxLS.x - minLS.x;
		const float extentY = maxLS.y - minLS.y;
		float centerX = 0.5f * (minLS.x + maxLS.x);
		float centerY = 0.5f * (minLS.y + maxLS.y);
		const float texelX = extentX / (float)this->m_shadowMapSize;
		const float texelY = extentY / (float)this->m_shadowMapSize;
		if (texelX > 0.0f) centerX = std::floor(centerX / texelX) * texelX;
		if (texelY > 0.0f) centerY = std::floor(centerY / texelY) * texelY;
		minLS.x = centerX - 0.5f * extentX;
		maxLS.x = centerX + 0.5f * extentX;
		minLS.y = centerY - 0.5f * extentY;
		maxLS.y = centerY + 0.5f * extentY;

		// glm::lookAt looks down -Z, so convert light-space z (likely negative) to near/far distances.
		const float zNear = std::max(0.1f, -maxLS.z - padZ);
		const float zFar  = std::max(zNear + 0.1f, -minLS.z + padZ);
		const glm::mat4 lightProj = glm::ortho(minLS.x, maxLS.x, minLS.y, maxLS.y, zNear, zFar);
		this->m_shadowLightVP[c] = lightProj * lightView;
	}
}

void SceneRenderer::buildShadowMaps() {
	if (!this->m_shadowEnabled || this->m_shadowProgram == nullptr) return;
	this->ensureShadowResources();
	if (this->m_shadowFBO == 0 || this->m_shadowTexArray == 0) return;

	this->updateShadowMatrices();

	glBindFramebuffer(GL_FRAMEBUFFER, this->m_shadowFBO);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0f, 4.0f);

	this->m_shadowProgram->useProgram();

	for (int layer = 0; layer < 3; ++layer) {
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, this->m_shadowTexArray, 0, layer);
		glViewport(0, 0, this->m_shadowMapSize, this->m_shadowMapSize);
		glClearDepth(1.0);
		glClear(GL_DEPTH_BUFFER_BIT);

		// Common shadow uniforms
		glUniformMatrix4fv(20, 1, GL_FALSE, glm::value_ptr(this->m_shadowLightVP[layer])); // lightVP

		// Dynamic objects: airplane + magic stone (skip pure-color debug objects)
		glUniform1i(21, 0); // useInstancing = 0
		for (DynamicSceneObject* obj : this->m_dynamicSOs) {
			if (!obj) continue;
			if (obj->pixelFunctionId() != SceneManager::Instance()->m_fs_texturePass) continue;
			glBindVertexArray(obj->vao());
			glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(obj->modelMat()));
			glDrawElements(obj->primitive(), obj->indexCount(), GL_UNSIGNED_INT, nullptr);
		}

		// Instance batches: airplane/stone are not here; cast for buildings + bush01/bush05.
		glUniform1i(21, 1); // useInstancing = 1 (uses VisibleBuffer indices)
		for (auto& batch : this->m_instanceBatches) {
			if (batch.numInstances == 0) continue;
			const bool castShadow =
				(batch.name == "bush01") || (batch.name == "bush05") ||
				(batch.name == "buildingV1") || (batch.name == "buildingV2");
			if (!castShadow) continue;
			glBindVertexArray(batch.vao);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, batch.instanceBuffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, batch.visibleIndexBuffer);
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, batch.indirectBuffer);
			glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0);
		}
	}

	glDisable(GL_POLYGON_OFFSET_FILL);
	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Restore viewport for subsequent passes.
	glViewport(this->m_curViewportX, this->m_curViewportY, this->m_curViewportW, this->m_curViewportH);
}

GLuint SceneRenderer::loadTexture(const std::string& path) {
	int w=0,h=0,ch=0;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, STBI_rgb_alpha);
	if(!data || w<=0 || h<=0){
		return 0;
	}
	GLuint tex=0;
	glGenTextures(1,&tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D,0);
	stbi_image_free(data);
	return tex;
}

void SceneRenderer::setUpInstanceBatches() {
	// build compute shader for culling
	Shader* cs = new Shader(GL_COMPUTE_SHADER);
	cs->createShaderFromFile("shaders\\cullInstances.comp");
	this->m_cullProgram = new ShaderProgram();
	this->m_cullProgram->init();
	this->m_cullProgram->attachShader(cs);
	this->m_cullProgram->checkStatus();
	this->m_cullProgram->linkProgram();
	cs->releaseShader();
	delete cs;
	this->m_cullProgram->useProgram();
	this->m_cullNumInstancesHandle = 0;
	this->m_cullFrustumHandle = 1;

	auto createBatch = [&](const std::string& name,
		const std::string& objPath,
		const std::string& texPath,
		const std::string& samplePath,
		const glm::vec3& sphereCenterOS,
		float sphereRadiusOS,
		const glm::vec3& ambient,
		const glm::vec3& specular,
		float shininess,
		bool useOcclusion,
		bool isOccluder) {
			InstanceBatch batch;
			batch.name = name;
			batch.materialAmbient = ambient;
			batch.materialSpecular = specular;
			batch.materialShininess = shininess;
			batch.useOcclusion = useOcclusion;
			batch.isOccluder = isOccluder;

			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(objPath,
				aiProcess_Triangulate |
				aiProcess_JoinIdenticalVertices |
				aiProcess_GenSmoothNormals |
				aiProcess_CalcTangentSpace);
			if(!scene || scene->mNumMeshes==0){ return; }
			const aiMesh* mesh = scene->mMeshes[0];
			bool hasUV = mesh->HasTextureCoords(0);
			int numVertices = (int)mesh->mNumVertices;
			int numIndices = (int)mesh->mNumFaces * 3;
			std::vector<float> vertices(numVertices * 11);
			for(unsigned int i=0;i<mesh->mNumVertices;i++){
				const aiVector3D& v = mesh->mVertices[i];
				const aiVector3D uv = hasUV ? mesh->mTextureCoords[0][i] : aiVector3D(0,0,0);
				vertices[i*11+0]=v.x; vertices[i*11+1]=v.y; vertices[i*11+2]=v.z;
				vertices[i*11+3]=mesh->mNormals[i].x; vertices[i*11+4]=mesh->mNormals[i].y; vertices[i*11+5]=mesh->mNormals[i].z;
				vertices[i*11+6]=mesh->mTangents ? mesh->mTangents[i].x : 0.0f;
				vertices[i*11+7]=mesh->mTangents ? mesh->mTangents[i].y : 0.0f;
				vertices[i*11+8]=mesh->mTangents ? mesh->mTangents[i].z : 0.0f;
				vertices[i*11+9]=uv.x; vertices[i*11+10]=uv.y;
			}
			std::vector<unsigned int> indices(numIndices);
			for(unsigned int f=0; f<mesh->mNumFaces; f++){
				const aiFace& face = mesh->mFaces[f];
				indices[f*3+0]=face.mIndices[0];
				indices[f*3+1]=face.mIndices[1];
				indices[f*3+2]=face.mIndices[2];
			}
			glGenVertexArrays(1,&batch.vao);
			glGenBuffers(1,&batch.vbo);
			glGenBuffers(1,&batch.ebo);
			glBindVertexArray(batch.vao);
			glBindBuffer(GL_ARRAY_BUFFER,batch.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,batch.ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
			int stride = 11*sizeof(float);
			glVertexAttribPointer(SceneManager::Instance()->m_vertexHandle,3,GL_FLOAT,GL_FALSE,stride,(void*)0);
			glEnableVertexAttribArray(SceneManager::Instance()->m_vertexHandle);
			glVertexAttribPointer(SceneManager::Instance()->m_normalHandle,3,GL_FLOAT,GL_FALSE,stride,(void*)(3*sizeof(float)));
			glEnableVertexAttribArray(SceneManager::Instance()->m_normalHandle);
			glVertexAttribPointer(SceneManager::Instance()->m_tangentHandle,3,GL_FLOAT,GL_FALSE,stride,(void*)(6*sizeof(float)));
			glEnableVertexAttribArray(SceneManager::Instance()->m_tangentHandle);
			glVertexAttribPointer(SceneManager::Instance()->m_uvHandle,2,GL_FLOAT,GL_FALSE,stride,(void*)(9*sizeof(float)));
			glEnableVertexAttribArray(SceneManager::Instance()->m_uvHandle);
			glBindVertexArray(0);
			batch.indexCount = numIndices;
			if(!texPath.empty()){
				batch.texture = loadTexture(texPath);
			}

			MyPoissonSample* sample = MyPoissonSample::fromFile(samplePath);
			batch.numInstances = sample->m_numSample;

			std::vector<InstanceDataGPU> instanceData(batch.numInstances);
			for(uint32_t i = 0; i < batch.numInstances; ++i){
				glm::vec3 pos(
					sample->m_positions[i*3+0],
					sample->m_positions[i*3+1],
					sample->m_positions[i*3+2]);
				glm::vec3 rad(
					sample->m_radians[i*3+0],
					sample->m_radians[i*3+1],
					sample->m_radians[i*3+2]);
				glm::quat q = glm::quat(rad);
				glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) * glm::toMat4(q);
				instanceData[i].model = model;
				glm::vec3 worldCenter = glm::vec3(model * glm::vec4(sphereCenterOS,1.0f));
				instanceData[i].sphere = glm::vec4(worldCenter, sphereRadiusOS);
			}
			glCreateBuffers(1,&batch.instanceBuffer);
			glNamedBufferData(batch.instanceBuffer, instanceData.size()*sizeof(InstanceDataGPU), instanceData.data(), GL_STATIC_DRAW);

			glCreateBuffers(1,&batch.visibleIndexBuffer);
			size_t visSize = (size_t)(batch.numInstances + 1) * sizeof(uint32_t);
			glNamedBufferData(batch.visibleIndexBuffer, visSize, nullptr, GL_DYNAMIC_DRAW);

			struct DrawCmd { uint32_t count, instanceCount, firstIndex, baseVertex, baseInstance; };
			DrawCmd cmd = { (uint32_t)batch.indexCount, 0u, 0u, 0u, 0u };
			glCreateBuffers(1,&batch.indirectBuffer);
			glNamedBufferData(batch.indirectBuffer, sizeof(DrawCmd), &cmd, GL_DYNAMIC_DRAW);

			delete sample;
			this->m_instanceBatches.push_back(batch);
	};

	createBatch("grassB",
		"assets\\outdoor\\grassB.obj",
		"assets\\outdoor\\grassB_albedo.png",
		"assets\\outdoor\\poissonPoints_621043_after.ppd2",
		glm::vec3(0.0f,0.66f,0.0f), 1.4f,
		glm::vec3(1.0f), glm::vec3(0.0f), 1.0f,
		true, false);

	createBatch("bush01",
		"assets\\outdoor\\bush01_lod2.obj",
		"assets\\outdoor\\bush01.png",
		"assets\\outdoor\\poissonPoints_1010.ppd2",
		glm::vec3(0.0f,2.55f,0.0f), 3.4f,
		glm::vec3(1.0f), glm::vec3(0.0f), 1.0f,
		true, false);

	createBatch("bush05",
		"assets\\outdoor\\bush05_lod2.obj",
		"assets\\outdoor\\bush05.png",
		"assets\\outdoor\\poissonPoints_2797.ppd2",
		glm::vec3(0.0f,1.76f,0.0f), 2.6f,
		glm::vec3(1.0f), glm::vec3(0.0f), 1.0f,
		true, false);

	createBatch("buildingV2",
		"assets\\outdoor\\Medieval_Building_LowPoly\\medieval_building_lowpoly_2.obj",
		"assets\\outdoor\\Medieval_Building_LowPoly\\Medieval_Building_LowPoly_V2_Albedo_small.png",
		"assets\\outdoor\\cityLots_sub_0.ppd2",
		glm::vec3(0.0f,4.57f,0.0f), 8.5f,
		glm::vec3(1.0f), glm::vec3(0.0f), 1.0f,
		false, true);

	createBatch("buildingV1",
		"assets\\outdoor\\Medieval_Building_LowPoly\\medieval_building_lowpoly_1.obj",
		"assets\\outdoor\\Medieval_Building_LowPoly\\Medieval_Building_LowPoly_V1_Albedo_small.png",
		"assets\\outdoor\\cityLots_sub_1.ppd2",
		glm::vec3(0.0f,4.57f,0.0f), 10.2f,
		glm::vec3(1.0f), glm::vec3(0.0f), 1.0f,
		false, true);
}

void SceneRenderer::dispatchCulling(InstanceBatch& batch){
	if(batch.numInstances == 0) return;
	// reset counters
	uint32_t zero = 0;
	glClearNamedBufferSubData(batch.visibleIndexBuffer, GL_R32UI, 0, sizeof(uint32_t), GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
	glClearNamedBufferSubData(batch.indirectBuffer, GL_R32UI, sizeof(uint32_t), sizeof(uint32_t), GL_RED_INTEGER, GL_UNSIGNED_INT, &zero); // instanceCount to 0

	this->m_cullProgram->useProgram();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, batch.instanceBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, batch.visibleIndexBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, batch.indirectBuffer);
	glUniform1ui(this->m_cullNumInstancesHandle, batch.numInstances);
	if (this->m_hasCullPlanesOverride) {
		glUniform4fv(this->m_cullFrustumHandle, 6, glm::value_ptr(this->m_cullPlanesOverride[0]));
	} else {
		extractFrustumPlanes(this->m_cullVP, this->m_frustumPlanes);
		glUniform4fv(this->m_cullFrustumHandle, 6, glm::value_ptr(this->m_frustumPlanes[0]));
	}
	// occlusion uniforms
	glUniform1i(9, (this->m_occlusionEnabled && batch.useOcclusion) ? 1 : 0);
	int fixedLevel = (this->m_occlusionFixedLevelOverride >= 0) ? this->m_occlusionFixedLevelOverride : (int)std::ceil((float)this->m_occlusionLevels * 0.5f);
	fixedLevel = std::clamp(fixedLevel, 0, std::max(0, this->m_occlusionLevels - 1));
	glUniform1i(10, fixedLevel);
	glUniformMatrix4fv(11, 1, GL_FALSE, glm::value_ptr(this->m_cullVP));
	glUniformMatrix4fv(12, 1, GL_FALSE, glm::value_ptr(this->m_cullView));
	glUniform1f(13, this->m_occlusionMaxViewDepth);
	glUniform1f(14, this->m_occlusionBias);
	glUniform2f(15, (float)this->m_frameWidth, (float)this->m_frameHeight);
	// bind depth pyramid on unit 5
	if (this->m_depthPyramidTex != 0) {
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, this->m_depthPyramidTex);
	}

	uint32_t groupSize = 256;
	uint32_t numGroup = (batch.numInstances + groupSize - 1) / groupSize;
	glDispatchCompute(numGroup, 1, 1);
	glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}

void SceneRenderer::renderInstanceBatches(bool foliageOnly){
	this->renderInstanceBatches(foliageOnly, true);
}

void SceneRenderer::renderInstanceBatches(bool foliageOnly, const bool recomputeVisibility){
	if(this->m_instanceBatches.empty()) return;
	SceneManager* manager = SceneManager::Instance();
		for(auto& batch : this->m_instanceBatches){
			// pass split: occluder pass renders occluders; foliage pass renders non-occluders
			if (foliageOnly && batch.isOccluder) continue;
			if (!foliageOnly && !batch.isOccluder) continue;
			if (recomputeVisibility) {
				this->dispatchCulling(batch);
			}
			this->m_shaderProgram->useProgram();
			// Instances never use fragment normal mapping in this project.
			glUniform1i(manager->m_useNormalMapHandle, 0);
			glBindVertexArray(batch.vao);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, batch.instanceBuffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, batch.visibleIndexBuffer);
		if(batch.texture){
			glActiveTexture(manager->m_albedoTexUnit);
			glBindTexture(GL_TEXTURE_2D, batch.texture);
		}
		glUniform1i(manager->m_fs_pixelProcessIdHandle, manager->m_fs_texturePass);
		glUniform1i(manager->m_vs_vertexProcessIdHandle, manager->m_vs_instanceProcess);
		glUniform3fv(manager->m_materialAmbientHandle,1,glm::value_ptr(batch.materialAmbient));
		glUniform3fv(manager->m_materialSpecularHandle,1,glm::value_ptr(batch.materialSpecular));
		glUniform1f(manager->m_materialShininessHandle,batch.materialShininess);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, batch.indirectBuffer);
		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0);
	}
	glBindVertexArray(0);
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

	// depth texture (player viewport size) for HZB
	glGenTextures(1, &this->m_gbufferDepthTex);
	glBindTexture(GL_TEXTURE_2D, this->m_gbufferDepthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->m_gbufferDepthTex, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	int maxDim = (w > h) ? w : h;
	this->m_depthNumLevels = (int)std::floor(std::log2((float)maxDim)) + 1;
	this->m_depthFixedLevel = (int)std::ceil(this->m_depthNumLevels * 100000.0f);

	glDrawBuffers(5, attachments);
	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return status == GL_FRAMEBUFFER_COMPLETE;
}

void SceneRenderer::destroyGBuffer() {
	if (this->m_gbufferDepthTex != 0) {
		glDeleteTextures(1, &this->m_gbufferDepthTex);
		this->m_gbufferDepthTex = 0;
	}
	this->destroyDepthVizTex();
	this->destroyDepthVizPyramid();
	if (this->m_depthPyramidTex != 0) {
		glDeleteTextures(1, &this->m_depthPyramidTex);
		this->m_depthPyramidTex = 0;
	}
	this->m_occlusionW = 0;
	this->m_occlusionH = 0;
	this->m_occlusionLevels = 1;
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

void SceneRenderer::ensureDepthVizTex(const int w, const int h) {
	if (w <= 0 || h <= 0) return;
	const int maxDim = (w > h) ? w : h;
	const int levels = (int)std::floor(std::log2((float)maxDim)) + 1;
	if (this->m_depthVizTex != 0 && this->m_depthVizW == w && this->m_depthVizH == h && this->m_depthVizLevels == levels) {
		return;
	}
	this->destroyDepthVizTex();
	this->m_depthVizW = w;
	this->m_depthVizH = h;
	this->m_depthVizLevels = std::max(1, levels);
	glCreateTextures(GL_TEXTURE_2D, 1, &this->m_depthVizTex);
	glTextureStorage2D(this->m_depthVizTex, this->m_depthVizLevels, GL_DEPTH_COMPONENT32F, w, h);
	glTextureParameteri(this->m_depthVizTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTextureParameteri(this->m_depthVizTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(this->m_depthVizTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(this->m_depthVizTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glCreateFramebuffers(1, &this->m_depthVizFBO);
	glNamedFramebufferTexture(this->m_depthVizFBO, GL_DEPTH_ATTACHMENT, this->m_depthVizTex, 0);
	glNamedFramebufferDrawBuffer(this->m_depthVizFBO, GL_NONE);
	glNamedFramebufferReadBuffer(this->m_depthVizFBO, GL_NONE);
}

void SceneRenderer::destroyDepthVizTex() {
	if (this->m_depthVizTex != 0) {
		glDeleteTextures(1, &this->m_depthVizTex);
		this->m_depthVizTex = 0;
	}
	if (this->m_depthVizFBO != 0) {
		glDeleteFramebuffers(1, &this->m_depthVizFBO);
		this->m_depthVizFBO = 0;
	}
	this->m_depthVizW = 0;
	this->m_depthVizH = 0;
	this->m_depthVizLevels = 1;
}

void SceneRenderer::ensureDepthVizPyramid(const int w, const int h) {
	if (w <= 0 || h <= 0) return;
	const int maxDim = (w > h) ? w : h;
	const int levels = (int)std::floor(std::log2((float)maxDim)) + 1;
	if (this->m_depthVizPyramidTex != 0 && this->m_depthVizW == w && this->m_depthVizH == h && this->m_depthVizLevels == levels) {
		return;
	}
	this->destroyDepthVizPyramid();
	this->m_depthVizW = w;
	this->m_depthVizH = h;
	this->m_depthVizLevels = std::max(1, levels);
	glCreateTextures(GL_TEXTURE_2D, 1, &this->m_depthVizPyramidTex);
	glTextureStorage2D(this->m_depthVizPyramidTex, this->m_depthVizLevels, GL_R32F, w, h);
	glTextureParameteri(this->m_depthVizPyramidTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTextureParameteri(this->m_depthVizPyramidTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(this->m_depthVizPyramidTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(this->m_depthVizPyramidTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void SceneRenderer::destroyDepthVizPyramid() {
	if (this->m_depthVizPyramidTex != 0) {
		glDeleteTextures(1, &this->m_depthVizPyramidTex);
		this->m_depthVizPyramidTex = 0;
	}
}

void SceneRenderer::ensureOcclusionPyramid(const int w, const int h) {
	if (w <= 0 || h <= 0) return;
	const int maxDim = (w > h) ? w : h;
	const int levels = (int)std::floor(std::log2((float)maxDim)) + 1;
	if (this->m_depthPyramidTex != 0 && this->m_occlusionW == w && this->m_occlusionH == h && this->m_occlusionLevels == levels) {
		return;
	}
	if (this->m_depthPyramidTex != 0) {
		glDeleteTextures(1, &this->m_depthPyramidTex);
		this->m_depthPyramidTex = 0;
	}
	this->m_occlusionW = w;
	this->m_occlusionH = h;
	this->m_occlusionLevels = std::max(1, levels);
	glCreateTextures(GL_TEXTURE_2D, 1, &this->m_depthPyramidTex);
	glTextureStorage2D(this->m_depthPyramidTex, this->m_occlusionLevels, GL_R32F, w, h);
	glTextureParameteri(this->m_depthPyramidTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTextureParameteri(this->m_depthPyramidTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(this->m_depthPyramidTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(this->m_depthPyramidTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

bool SceneRenderer::setUpDepthVizShader() {
	Shader* cs = new Shader(GL_COMPUTE_SHADER);
	cs->createShaderFromFile("shaders\\depthVizBuild.comp");
	this->m_depthVizProgram = new ShaderProgram();
	this->m_depthVizProgram->init();
	this->m_depthVizProgram->attachShader(cs);
	this->m_depthVizProgram->checkStatus();
	this->m_depthVizProgram->linkProgram();
	cs->releaseShader();
	delete cs;

	this->m_depthVizProgram->useProgram();
	this->m_depthVizDstLevelHandle = 0;
	this->m_depthVizSrcLevelHandle = 1;
	return true;
}

void SceneRenderer::buildDepthVizPyramid() {
	if (!this->m_depthVizEnabled || this->m_depthVizTex == 0 || this->m_depthVizPyramidTex == 0 || this->m_depthVizProgram == nullptr) return;
	this->m_depthVizProgram->useProgram();

	// Level 0: copy input depth -> R32F pyramid level 0.
	glBindImageTexture(0, this->m_depthVizPyramidTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
	glBindTextureUnit(1, this->m_depthVizTex);
	glUniform1i(this->m_depthVizDstLevelHandle, 0);
	glUniform1i(this->m_depthVizSrcLevelHandle, 0);
	int gx0 = (int)std::ceil((float)this->m_depthVizW / 8.0f);
	int gy0 = (int)std::ceil((float)this->m_depthVizH / 8.0f);
	glDispatchCompute(gx0, gy0, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	// Levels 1..N: max reduction from previous level (stored in the same R32F texture).
	glBindTextureUnit(1, this->m_depthVizPyramidTex);
	int srcW = this->m_depthVizW;
	int srcH = this->m_depthVizH;
	for (int level = 1; level < this->m_depthVizLevels; ++level) {
		int dstW = std::max(1, srcW >> 1);
		int dstH = std::max(1, srcH >> 1);
		glBindImageTexture(0, this->m_depthVizPyramidTex, level, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
		glUniform1i(this->m_depthVizDstLevelHandle, level);
		glUniform1i(this->m_depthVizSrcLevelHandle, level - 1);
		int gx = (int)std::ceil((float)dstW / 8.0f);
		int gy = (int)std::ceil((float)dstH / 8.0f);
		glDispatchCompute(gx, gy, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
		srcW = dstW;
		srcH = dstH;
	}
}

void SceneRenderer::createDepthPyramid(const int w, const int h) {
	if (this->m_depthPyramidTex != 0) {
		glDeleteTextures(1, &this->m_depthPyramidTex);
		this->m_depthPyramidTex = 0;
	}
	if (w <= 0 || h <= 0) return;
	glCreateTextures(GL_TEXTURE_2D, 1, &this->m_depthPyramidTex);
	glTextureStorage2D(this->m_depthPyramidTex, this->m_depthNumLevels, GL_R32F, w, h);
	glTextureParameteri(this->m_depthPyramidTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(this->m_depthPyramidTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(this->m_depthPyramidTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(this->m_depthPyramidTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void SceneRenderer::destroyDepthPyramid() {
	if (this->m_depthPyramidTex != 0) {
		glDeleteTextures(1, &this->m_depthPyramidTex);
		this->m_depthPyramidTex = 0;
	}
}

void SceneRenderer::buildDepthPyramid() {
	// Build viewport-sized occlusion pyramid from the viewport-sized depthVizTex using MIN reduction (nearest depth).
	if (this->m_hzbProgram == nullptr || this->m_depthPyramidTex == 0 || this->m_depthVizTex == 0) return;
	this->m_hzbProgram->useProgram();
	// level 0: copy from depth texture into R32F target
	glBindImageTexture(0, this->m_depthPyramidTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
	glBindTextureUnit(1, this->m_depthVizTex);
	glBindTextureUnit(2, this->m_depthPyramidTex);
	glUniform1i(this->m_hzbSrcLevelHandle, -1);
	glUniform1i(this->m_hzbDstLevelHandle, 0);
	glUniform1i(2, 0); // reduceOp = MIN
	int w = (int)std::ceil((float)this->m_occlusionW / 8.0f);
	int h = (int)std::ceil((float)this->m_occlusionH / 8.0f);
	glDispatchCompute(w, h, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// build remaining levels from pyramid itself
	int srcW = this->m_occlusionW;
	int srcH = this->m_occlusionH;
	for (int level = 1; level < this->m_occlusionLevels; ++level) {
		int dstW = std::max(1, srcW >> 1);
		int dstH = std::max(1, srcH >> 1);
		glBindImageTexture(0, this->m_depthPyramidTex, level, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
		glUniform1i(this->m_hzbSrcLevelHandle, level - 1);
		glUniform1i(this->m_hzbDstLevelHandle, level);
		glUniform1i(2, 0); // reduceOp = MIN
		int gx = (int)std::ceil((float)dstW / 8.0f);
		int gy = (int)std::ceil((float)dstH / 8.0f);
		glDispatchCompute(gx, gy, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		srcW = dstW;
		srcH = dstH;
	}
}

void SceneRenderer::renderGeometryPass() {
	this->renderGeometryPass(true, true);
}

void SceneRenderer::renderGeometryPass(const bool recomputeVisibility, const bool buildPyramids) {
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
	// Only magic stone uses fragment normal mapping; default other draws to off.
	glUniform1i(manager->m_useNormalMapHandle, 0);
	// culling VP is provided externally via setCullingVP (player frustum)
	glm::mat4 invView = glm::inverse(this->m_viewMat);
	this->m_cullCamPos = glm::vec3(invView[3]);

	// directional light in world space
	const glm::vec3 lightDirWorld = glm::normalize(glm::vec3(0.4f, 0.5f, 0.8f));
	glUniform3fv(manager->m_lightDirWorldHandle, 1, glm::value_ptr(lightDirWorld));
	glUniform3fv(manager->m_cameraPosHandle, 1, glm::value_ptr(this->m_cullCamPos));

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
	// pass 1: occluder instances only (buildings)
	this->renderInstanceBatches(false, recomputeVisibility);

	if (buildPyramids) {
		// build depth mipmap for visualization (viewport-sized texture to avoid edge contamination)
		bool anyOcclusion = false;
		for (const auto& b : this->m_instanceBatches) {
			if (b.useOcclusion) { anyOcclusion = true; break; }
		}
		if ((this->m_depthVizEnabled || anyOcclusion) && this->m_gbufferDepthTex != 0) {
			this->ensureDepthVizTex(this->m_curViewportW, this->m_curViewportH);
			if (anyOcclusion) {
				this->ensureOcclusionPyramid(this->m_curViewportW, this->m_curViewportH);
			}
			if (this->m_depthVizEnabled) {
				this->ensureDepthVizPyramid(this->m_curViewportW, this->m_curViewportH);
			}
			if (this->m_depthVizTex != 0 && this->m_depthVizFBO != 0) {
				// Blit only the player viewport region to a viewport-sized depth texture.
				glBindFramebuffer(GL_READ_FRAMEBUFFER, this->m_gbufferFBO);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->m_depthVizFBO);
				glBlitFramebuffer(
					this->m_curViewportX, this->m_curViewportY,
					this->m_curViewportX + this->m_curViewportW, this->m_curViewportY + this->m_curViewportH,
					0, 0, this->m_curViewportW, this->m_curViewportH,
					GL_DEPTH_BUFFER_BIT, GL_NEAREST);
				glBindFramebuffer(GL_FRAMEBUFFER, this->m_gbufferFBO);
				// Visualization pyramid (MAX) only when requested.
				if (this->m_depthVizEnabled) {
					this->buildDepthVizPyramid();
				}
				// Occlusion pyramid (MIN = nearest depth) when any foliage uses occlusion.
				if (anyOcclusion) {
					this->buildDepthPyramid();
					this->m_hzbBuiltThisFrame = true;
				}
			}
		}
	}

	// (Occlusion pyramid is built above from viewport depth.)

	// pass 2: foliage instances with occlusion culling
	this->renderInstanceBatches(true, recomputeVisibility);

	// Build shadow maps once per frame, AFTER visibility is computed, so culled objects don't cast shadows.
	if (recomputeVisibility && buildPyramids && this->m_shadowEnabled && !this->m_shadowBuiltThisFrame) {
		this->buildShadowMaps();
		this->m_shadowBuiltThisFrame = true;
		// Restore G-buffer binding/viewport in case subsequent code assumes it.
		glBindFramebuffer(GL_FRAMEBUFFER, this->m_gbufferFBO);
		glViewport(this->m_curViewportX, this->m_curViewportY, this->m_curViewportW, this->m_curViewportH);
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
	glActiveTexture(GL_TEXTURE5);
	if (this->m_gbufferDisplayMode == 6 && this->m_depthVizTex != 0) {
		glBindTexture(GL_TEXTURE_2D, (this->m_depthVizPyramidTex != 0) ? this->m_depthVizPyramidTex : this->m_depthVizTex);
	} else {
		glBindTexture(GL_TEXTURE_2D, this->m_gbufferDepthTex);
	}
	glActiveTexture(GL_TEXTURE6);
	if (this->m_shadowTexArray != 0) {
		glBindTexture(GL_TEXTURE_2D_ARRAY, this->m_shadowTexArray);
	} else {
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}

	glUniform1i(this->m_displayModeHandle, this->m_gbufferDisplayMode);
	glUniform1i(this->m_displayDepthMipLevelHandle, this->m_depthDisplayLevel);
	glm::mat4 invProj = glm::inverse(this->m_projMat);
	glUniformMatrix4fv(this->m_displayInvProjHandle, 1, GL_FALSE, glm::value_ptr(invProj));
	glUniform1f(14, this->m_depthVisFar);
	glUniform1f(16, this->m_depthVisGamma);
	glUniform1i(this->m_shadowEnabledHandle, this->m_shadowEnabled ? 1 : 0);
	glUniform1i(this->m_shadowCascadeVizEnabledHandle, this->m_shadowCascadeVizEnabled ? 1 : 0);
	glUniform3f(this->m_shadowCascadeFarHandle, this->m_shadowCascadeFar[0], this->m_shadowCascadeFar[1], this->m_shadowCascadeFar[2]);
	glUniformMatrix4fv(this->m_shadowLightVPHandle, 3, GL_FALSE, glm::value_ptr(this->m_shadowLightVP[0]));
	// Always base cascade visualization ranges on player (culling) view-space depth.
	glUniformMatrix4fv(this->m_shadowCullViewMatHandle, 1, GL_FALSE, glm::value_ptr(this->m_cullView));
	float uvScaleX = 1.0f, uvScaleY = 1.0f, uvBiasX = 0.0f, uvBiasY = 0.0f;
	if (this->m_gbufferDisplayMode == 6 && this->m_depthVizTex != 0) {
		// depth viz texture is already viewport-sized: map to texel centers in [0,1]
		uvScaleX = (this->m_depthVizW > 1) ? ((float)(this->m_depthVizW - 1) / (float)this->m_depthVizW) : 0.0f;
		uvScaleY = (this->m_depthVizH > 1) ? ((float)(this->m_depthVizH - 1) / (float)this->m_depthVizH) : 0.0f;
		uvBiasX = (this->m_depthVizW > 0) ? (0.5f / (float)this->m_depthVizW) : 0.0f;
		uvBiasY = (this->m_depthVizH > 0) ? (0.5f / (float)this->m_depthVizH) : 0.0f;
	} else {
		// Map the quad UV [0,1] onto texel centers of the sampled viewport region (within full-resolution G-buffer).
		uvScaleX = (this->m_frameWidth > 0)
			? ((this->m_displaySampleViewportW > 1) ? ((float)(this->m_displaySampleViewportW - 1) / (float)this->m_frameWidth) : 0.0f)
			: 1.0f;
		uvScaleY = (this->m_frameHeight > 0)
			? ((this->m_displaySampleViewportH > 1) ? ((float)(this->m_displaySampleViewportH - 1) / (float)this->m_frameHeight) : 0.0f)
			: 1.0f;
		uvBiasX = (this->m_frameWidth > 0)
			? ((float)this->m_displaySampleViewportX + 0.5f) / (float)this->m_frameWidth
			: 0.0f;
		uvBiasY = (this->m_frameHeight > 0)
			? ((float)this->m_displaySampleViewportY + 0.5f) / (float)this->m_frameHeight
			: 0.0f;
	}
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
