#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <vector>
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <GLFW/glfw3.h>

#include "Shader.h"
#include "SceneRenderer.h"
#include "MyImGuiPanel.h"

#include "ViewFrustumSceneObject.h"
#include "DynamicSceneObject.h"
#include "terrain\MyTerrain.h"
#include "MyCameraManager.h"

const int INIT_WIDTH = 1024;
const int INIT_HEIGHT = 512;

// ==============================================
// You can probably tell these come from class members,
// but let's make them global for clarity�especially for those less familiar with C++ OOP.

int displayWidth;
int displayHeight;

double cursorPos[2];

MyImGuiPanel* m_imguiPanel = nullptr;
SceneRenderer* defaultRenderer = nullptr;
ShaderProgram* defaultShaderProgram = nullptr;
ViewFrustumSceneObject* m_viewFrustumSO = nullptr;
MyTerrain* m_terrain = nullptr;
INANOA::MyCameraManager* m_myCameraManager = nullptr;
DynamicSceneObject* m_airplaneSO = nullptr;
DynamicSceneObject* m_magicStoneSO = nullptr;

bool g_useNormalMap = false;
int g_gbufferViewMode = 3; // 0:pos,1:normal,2:ambient,3:diffuse,4:specular
// ==============================================

void resize_impl(int w, int h);
DynamicSceneObject* createAirplaneSceneObject();
DynamicSceneObject* createMagicStoneSceneObject();

GLuint createTextureFromFile(const std::string& fileFullPath) {
	int width = 0, height = 0, channels = 0;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(fileFullPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	if (data == nullptr || width <= 0 || height <= 0) {
		return 0;
	}

	GLuint texHandle = 0;
	glGenTextures(1, &texHandle);
	glBindTexture(GL_TEXTURE_2D, texHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(data);
	return texHandle;
}

DynamicSceneObject* createAirplaneSceneObject()
{
	const std::string modelPath = "assets\\outdoor\\airplane.obj";
	const std::string texturePath = "assets\\outdoor\\Airplane_smooth_DefaultMaterial_BaseMap.jpg";

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(modelPath,
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GenSmoothNormals |
		aiProcess_CalcTangentSpace);

	if (scene == nullptr || scene->mNumMeshes == 0) {
		return nullptr;
	}

	const aiMesh* mesh = scene->mMeshes[0];
	const bool hasUV = mesh->HasTextureCoords(0);

	const int numVertices = static_cast<int>(mesh->mNumVertices);
	const int numIndices = static_cast<int>(mesh->mNumFaces * 3);

	DynamicSceneObject* airplane = new DynamicSceneObject(numVertices, numIndices, true, true);

	float* dataBuffer = airplane->dataBuffer();
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		const aiVector3D& v = mesh->mVertices[i];
		const aiVector3D uv = hasUV ? mesh->mTextureCoords[0][i] : aiVector3D(0.0f, 0.0f, 0.0f);

		dataBuffer[i * 11 + 0] = v.x;
		dataBuffer[i * 11 + 1] = v.y;
		dataBuffer[i * 11 + 2] = v.z;
		dataBuffer[i * 11 + 3] = mesh->mNormals[i].x;
		dataBuffer[i * 11 + 4] = mesh->mNormals[i].y;
		dataBuffer[i * 11 + 5] = mesh->mNormals[i].z;
		dataBuffer[i * 11 + 6] = mesh->mTangents ? mesh->mTangents[i].x : 0.0f;
		dataBuffer[i * 11 + 7] = mesh->mTangents ? mesh->mTangents[i].y : 0.0f;
		dataBuffer[i * 11 + 8] = mesh->mTangents ? mesh->mTangents[i].z : 0.0f;
		dataBuffer[i * 11 + 9] = uv.x;
		dataBuffer[i * 11 + 10] = uv.y;
	}

	unsigned int* indexBuffer = airplane->indexBuffer();
	for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
		const aiFace& face = mesh->mFaces[f];
		indexBuffer[f * 3 + 0] = face.mIndices[0];
		indexBuffer[f * 3 + 1] = face.mIndices[1];
		indexBuffer[f * 3 + 2] = face.mIndices[2];
	}

	airplane->updateDataBuffer(0, numVertices * 11 * sizeof(float));
	airplane->updateIndexBuffer(0, numIndices * sizeof(unsigned int));
	airplane->setPrimitive(GL_TRIANGLES);
	airplane->setPixelFunctionId(SceneManager::Instance()->m_fs_texturePass);
	airplane->setMaterial(glm::vec3(1.0f), glm::vec3(1.0f), 32.0f);

	const GLuint albedoTex = createTextureFromFile(texturePath);
	if (albedoTex != 0) {
		airplane->setAlbedoTexture(albedoTex);
	}

	return airplane;
}

DynamicSceneObject* createMagicStoneSceneObject()
{
	const std::string modelPath = "assets\\outdoor\\MagicRock\\magicRock.obj";
	const std::string texturePath = "assets\\outdoor\\MagicRock\\StylMagicRocks_AlbedoTransparency.png";

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(modelPath,
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GenSmoothNormals |
		aiProcess_CalcTangentSpace);

	if (scene == nullptr || scene->mNumMeshes == 0) {
		return nullptr;
	}

	const aiMesh* mesh = scene->mMeshes[0];
	const bool hasUV = mesh->HasTextureCoords(0);

	const int numVertices = static_cast<int>(mesh->mNumVertices);
	const int numIndices = static_cast<int>(mesh->mNumFaces * 3);

	DynamicSceneObject* stone = new DynamicSceneObject(numVertices, numIndices, true, true);

	float* dataBuffer = stone->dataBuffer();
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		const aiVector3D& v = mesh->mVertices[i];
		const aiVector3D uv = hasUV ? mesh->mTextureCoords[0][i] : aiVector3D(0.0f, 0.0f, 0.0f);

		dataBuffer[i * 11 + 0] = v.x;
		dataBuffer[i * 11 + 1] = v.y;
		dataBuffer[i * 11 + 2] = v.z;
		dataBuffer[i * 11 + 3] = mesh->mNormals[i].x;
		dataBuffer[i * 11 + 4] = mesh->mNormals[i].y;
		dataBuffer[i * 11 + 5] = mesh->mNormals[i].z;
		dataBuffer[i * 11 + 6] = mesh->mTangents ? mesh->mTangents[i].x : 0.0f;
		dataBuffer[i * 11 + 7] = mesh->mTangents ? mesh->mTangents[i].y : 0.0f;
		dataBuffer[i * 11 + 8] = mesh->mTangents ? mesh->mTangents[i].z : 0.0f;
		dataBuffer[i * 11 + 9] = uv.x;
		dataBuffer[i * 11 + 10] = uv.y;
	}

	unsigned int* indexBuffer = stone->indexBuffer();
	for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
		const aiFace& face = mesh->mFaces[f];
		indexBuffer[f * 3 + 0] = face.mIndices[0];
		indexBuffer[f * 3 + 1] = face.mIndices[1];
		indexBuffer[f * 3 + 2] = face.mIndices[2];
	}

	stone->updateDataBuffer(0, numVertices * 11 * sizeof(float));
	stone->updateIndexBuffer(0, numIndices * sizeof(unsigned int));
	stone->setPrimitive(GL_TRIANGLES);
	stone->setPixelFunctionId(SceneManager::Instance()->m_fs_texturePass);
	stone->setMaterial(glm::vec3(1.0f), glm::vec3(1.0f), 32.0f);

	const GLuint albedoTex = createTextureFromFile(texturePath);
	if (albedoTex != 0) {
		stone->setAlbedoTexture(albedoTex);
	}
	const GLuint normalTex = createTextureFromFile("assets\\outdoor\\MagicRock\\StylMagicRocks_NormalOpenGL.png");
	if (normalTex != 0) {
		stone->setNormalTexture(normalTex);
	}

	return stone;
}


bool on_init(int displayWidth, int displayHeight)
{
	// initialize shader program
	// vertex shader
	Shader* vsShader = new Shader(GL_VERTEX_SHADER);
	vsShader->createShaderFromFile("shaders\\oglVertexShader.glsl");
	std::cout << vsShader->shaderInfoLog() << "\n";

	// fragment shader
	Shader* fsShader = new Shader(GL_FRAGMENT_SHADER);
	fsShader->createShaderFromFile("shaders\\oglFragmentShader.glsl");
	std::cout << fsShader->shaderInfoLog() << "\n";

	// shader program
	ShaderProgram* shaderProgram = new ShaderProgram();
	shaderProgram->init();
	shaderProgram->attachShader(vsShader);
	shaderProgram->attachShader(fsShader);
	shaderProgram->checkStatus();
	if (shaderProgram->status() != ShaderProgramStatus::READY) { return false; }
	shaderProgram->linkProgram();

	vsShader->releaseShader();
	fsShader->releaseShader();

	delete vsShader;
	delete fsShader;

	defaultShaderProgram = shaderProgram;
	// =================================================================
	// init renderer
	defaultRenderer = new SceneRenderer();
	if (!defaultRenderer->initialize(displayWidth, displayHeight, shaderProgram)) { return false; }

	// =================================================================
	// initialize camera
	m_myCameraManager = new INANOA::MyCameraManager();
	m_myCameraManager->init(displayWidth, displayHeight);

	// initialize view frustum
	m_viewFrustumSO = new ViewFrustumSceneObject(2, SceneManager::Instance()->m_fs_pixelProcessIdHandle, SceneManager::Instance()->m_fs_pureColor);
	defaultRenderer->appendDynamicSceneObject(m_viewFrustumSO->sceneObject());

	// initialize airplane
	m_airplaneSO = createAirplaneSceneObject();
	if (m_airplaneSO != nullptr) {
		defaultRenderer->appendDynamicSceneObject(m_airplaneSO);
	}

	// initialize magic stone
	m_magicStoneSO = createMagicStoneSceneObject();
	if (m_magicStoneSO != nullptr) {
		defaultRenderer->appendDynamicSceneObject(m_magicStoneSO);
	}

	// initialize terrain
	m_terrain = new MyTerrain();
	m_terrain->init(-1);
	defaultRenderer->appendTerrainSceneObject(m_terrain->sceneObject());
	// =================================================================	

	resize_impl(displayWidth, displayHeight);
	m_imguiPanel = new MyImGuiPanel();

	return true;
}

void on_destroy()
{
	delete defaultRenderer;
	delete defaultShaderProgram;
	delete m_myCameraManager;
	delete m_viewFrustumSO;
	delete m_airplaneSO;
	delete m_magicStoneSO;
	delete m_terrain;
	delete m_imguiPanel;
}

void viewFrustumMultiClipCorner(const std::vector<float>& depths, const glm::mat4& viewMat, const glm::mat4& projMat, float* clipCorner)
{
	const int NUM_CLIP = depths.size();

	// Calculate Inverse
	glm::mat4 viewProjInv = glm::inverse(projMat * viewMat);

	// Calculate Clip Plane Corners
	int clipOffset = 0;
	for (const float depth : depths)
	{
		// Get Depth in NDC, the depth in viewSpace is negative
		glm::vec4 v = glm::vec4(0, 0, -1 * depth, 1);
		glm::vec4 vInNDC = projMat * v;
		if (fabs(vInNDC.w) > 0.00001)
		{
			vInNDC.z = vInNDC.z / vInNDC.w;
		}
		// Get 4 corner of clip plane
		float cornerXY[] = {
			-1, 1,
			-1, -1,
			1, -1,
			1, 1
		};
		for (int j = 0; j < 4; j++)
		{
			glm::vec4 wcc = {
				cornerXY[j * 2 + 0], cornerXY[j * 2 + 1], vInNDC.z, 1
			};
			wcc = viewProjInv * wcc;
			wcc = wcc / wcc.w;

			clipCorner[clipOffset * 12 + j * 3 + 0] = wcc[0];
			clipCorner[clipOffset * 12 + j * 3 + 1] = wcc[1];
			clipCorner[clipOffset * 12 + j * 3 + 2] = wcc[2];
		}
		clipOffset = clipOffset + 1;
	}
}

void updateWhenPlayerProjectionChanged(const float nearDepth, const float farDepth)
{
	// get view frustum corner
	const int NUM_CASCADE = 2;
	const float HY = 0.0;

	float dOffset = (farDepth - nearDepth) / NUM_CASCADE;
	float* corners = new float[(NUM_CASCADE + 1) * 12];
	std::vector<float> depths(NUM_CASCADE + 1);
	for (int i = 0; i < NUM_CASCADE; i++)
	{
		depths[i] = nearDepth + dOffset * i;
	}
	depths[NUM_CASCADE] = farDepth;
	// get viewspace corners
	glm::mat4 tView = glm::lookAt(glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	// calculate corners of view frustum cascade
	viewFrustumMultiClipCorner(depths, tView, m_myCameraManager->playerProjectionMatrix(), corners);

	// update view frustum scene object
	for (int i = 0; i < NUM_CASCADE + 1; i++)
	{
		float* layerBuffer = m_viewFrustumSO->cascadeDataBuffer(i);
		for (int j = 0; j < 12; j++)
		{
			layerBuffer[j] = corners[i * 12 + j];
		}
	}
	m_viewFrustumSO->updateDataBuffer();

	delete[] corners;
}

inline void resize_impl(int w, int h)
{
	m_myCameraManager->resize(w, h);
	defaultRenderer->resize(w, h);
	updateWhenPlayerProjectionChanged(0.1, m_myCameraManager->playerCameraFar());
}

void on_resize(GLFWwindow* window, int w, int h)
{
	displayWidth = w;
	displayHeight = h;
	resize_impl(w, h);
}

inline void on_display()
{
	// update cameras and airplane

	// god camera
	m_myCameraManager->updateGodCamera();
	// player camera
	m_myCameraManager->updatePlayerCamera();
	const glm::vec3 PLAYER_CAMERA_POSITION = m_myCameraManager->playerViewOrig();
	m_myCameraManager->adjustPlayerCameraHeight(m_terrain->terrainData()->height(PLAYER_CAMERA_POSITION.x, PLAYER_CAMERA_POSITION.z));
	// airplane
	m_myCameraManager->updateAirplane();
	const glm::vec3 AIRPLANE_POSTION = m_myCameraManager->airplanePosition();
	m_myCameraManager->adjustAirplaneHeight(m_terrain->terrainData()->height(AIRPLANE_POSTION.x, AIRPLANE_POSTION.z));

	// prepare parameters
	const glm::mat4 playerVM = m_myCameraManager->playerViewMatrix();
	const glm::mat4 playerProjMat = m_myCameraManager->playerProjectionMatrix();
	const glm::vec3 playerViewOrg = m_myCameraManager->playerViewOrig();

	const glm::mat4 godVM = m_myCameraManager->godViewMatrix();
	const glm::mat4 godProjMat = m_myCameraManager->godProjectionMatrix();

	const glm::mat4 airplaneModelMat = m_myCameraManager->airplaneModelMatrix();
	const glm::mat4 scaledAirplaneModelMat = airplaneModelMat * glm::scale(glm::vec3(1.0f));
	const glm::mat4 magicStoneModelMat = glm::translate(glm::vec3(25.92f, 18.27f, 11.75f));

	if (m_airplaneSO != nullptr) {
		m_airplaneSO->setModelMat(scaledAirplaneModelMat);
	}
	if (m_magicStoneSO != nullptr) {
		m_magicStoneSO->setModelMat(magicStoneModelMat);
	}

	// (x, y, w, h)
	const glm::ivec4 playerViewport = m_myCameraManager->playerViewport();

	// (x, y, w, h)
	const glm::ivec4 godViewport = m_myCameraManager->godViewport();

	// ====================================================================================
	// update player camera view frustum
	m_viewFrustumSO->updateState(playerVM, playerViewOrg);

	// update geography
	m_terrain->updateState(playerVM, playerViewOrg, playerProjMat, nullptr);
	// =============================================

	// =============================================
	// start rendering
	// start new frame
	defaultRenderer->setViewport(0, 0, displayWidth, displayHeight);
	defaultRenderer->startNewFrame();

	// rendering with player view		
	defaultRenderer->setViewport(playerViewport[0], playerViewport[1], playerViewport[2], playerViewport[3]);
	defaultRenderer->setView(playerVM);
	defaultRenderer->setProjection(playerProjMat);
	defaultRenderer->renderPass(g_gbufferViewMode);

	// rendering with god view
	defaultRenderer->setViewport(godViewport[0], godViewport[1], godViewport[2], godViewport[3]);
	defaultRenderer->setView(godVM);
	defaultRenderer->setProjection(godProjMat);
	defaultRenderer->renderPass(g_gbufferViewMode);
	// ===============================
}

inline void on_gui()
{
	// Show statistics window

	ImGui::Begin("Information");
	m_imguiPanel->update();

	// teleport controls
	if (ImGui::Button("Teleport 1")) {
		m_myCameraManager->teleport(0);
	}
	ImGui::SameLine();
	if (ImGui::Button("Teleport 2")) {
		m_myCameraManager->teleport(1);
	}
	ImGui::SameLine();
	if (ImGui::Button("Teleport 3")) {
		m_myCameraManager->teleport(2);
	}

	if (ImGui::Checkbox("Enable Magic Normal Map", &g_useNormalMap)) {
    	DynamicSceneObject::setGlobalNormalMapToggle(g_useNormalMap);
	}

	static const char* gbufferLabels[] = {
		"World Position", "World Normal", "Ambient", "Diffuse", "Specular"
	};
	ImGui::Text("G-Buffer View");
	ImGui::Combo("Mode", &g_gbufferViewMode, gbufferLabels, IM_ARRAYSIZE(gbufferLabels));

	ImGui::End();
}


void on_mouse_button(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		m_myCameraManager->mousePress(RenderWidgetMouseButton::M_LEFT, cursorPos[0], cursorPos[1]);
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		m_myCameraManager->mouseRelease(RenderWidgetMouseButton::M_LEFT, cursorPos[0], cursorPos[1]);
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		m_myCameraManager->mousePress(RenderWidgetMouseButton::M_RIGHT, cursorPos[0], cursorPos[1]);
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
	{
		m_myCameraManager->mouseRelease(RenderWidgetMouseButton::M_RIGHT, cursorPos[0], cursorPos[1]);
	}
}

void on_cursor_pos(GLFWwindow* window, double x, double y)
{
	cursorPos[0] = x;
	cursorPos[1] = y;

	m_myCameraManager->mouseMove(x, y);
}

void on_key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	auto setKeyStatus = [](const RenderWidgetKeyCode code, const int action)
		{
			if (action == GLFW_PRESS)
			{
				m_myCameraManager->keyPress(code);
			}
			else if (action == GLFW_RELEASE)
			{
				m_myCameraManager->keyRelease(code);
			}
		};

	// =======================================
	if (key == GLFW_KEY_W) { setKeyStatus(RenderWidgetKeyCode::KEY_W, action); }
	else if (key == GLFW_KEY_A) { setKeyStatus(RenderWidgetKeyCode::KEY_A, action); }
	else if (key == GLFW_KEY_S) { setKeyStatus(RenderWidgetKeyCode::KEY_S, action); }
	else if (key == GLFW_KEY_D) { setKeyStatus(RenderWidgetKeyCode::KEY_D, action); }
	else if (key == GLFW_KEY_T) { setKeyStatus(RenderWidgetKeyCode::KEY_T, action); }
	else if (key == GLFW_KEY_Z) { setKeyStatus(RenderWidgetKeyCode::KEY_Z, action); }
	else if (key == GLFW_KEY_X) { setKeyStatus(RenderWidgetKeyCode::KEY_X, action); }
}

void on_scroll(GLFWwindow* window, double xoffset, double yoffset) {}

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char**)
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	const char* glsl_version = "#version 460";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);


	// Create window with graphics context
	float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
	const int windowWidth = (int)(INIT_WIDTH * main_scale);
	const int windowHeight = (int)(INIT_HEIGHT * main_scale);
	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Final_Outdoor_Template", nullptr, nullptr);
	if (window == nullptr)
		return 1;
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Failed to initialize GLAD\n";
		return -1;
	}
	// Uncomment if you want to disable vsync
	// glfwSwapInterval(0);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup scaling
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

	// Register callbacks (before ImGui_ImplGlfw_InitForOpenGL)
	glfwSetKeyCallback(window, on_key);
	glfwSetScrollCallback(window, on_scroll);
	glfwSetMouseButtonCallback(window, on_mouse_button);
	glfwSetCursorPosCallback(window, on_cursor_pos);
	glfwSetFramebufferSizeCallback(window, on_resize);

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Init program
	if (on_init(windowWidth, windowHeight) == false)
	{
		glfwTerminate();
		return 0;
	}

	// FPS calculation
	double previousTimeForFPS = glfwGetTime();
	int frameCount = 0;

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		// FPS calculation
		const double currentTime = glfwGetTime();
		frameCount = frameCount + 1;
		const double deltaTime = currentTime - previousTimeForFPS;

		if (deltaTime >= 1.0)
		{
			// Kind of an ImGui anti-pattern to use it this way
			m_imguiPanel->setAvgFPS(frameCount * 1.0);
			m_imguiPanel->setAvgFrameTime(deltaTime * 1000.0 / frameCount);

			// Reset
			frameCount = 0;
			previousTimeForFPS = currentTime;
		}

		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		glfwPollEvents();
		if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
		{
			ImGui_ImplGlfw_Sleep(10);
			continue;
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		on_gui();
		// Rendering
		on_display();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

	// Cleanup
	on_destroy();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
