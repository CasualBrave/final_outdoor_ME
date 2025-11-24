#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <cstdio>
#include <iostream>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>

#include "Shader.h"
#include "SceneRenderer.h"
#include "MyImGuiPanel.h"

#include "ViewFrustumSceneObject.h"
#include "terrain\MyTerrain.h"
#include "MyCameraManager.h"

const int INIT_WIDTH = 1024;
const int INIT_HEIGHT = 512;

// ==============================================
// You can probably tell these come from class members,
// but let's make them global for clarity—especially for those less familiar with C++ OOP.

int displayWidth;
int displayHeight;

double cursorPos[2];

MyImGuiPanel* m_imguiPanel = nullptr;
SceneRenderer* defaultRenderer = nullptr;
ShaderProgram* defaultShaderProgram = nullptr;
ViewFrustumSceneObject* m_viewFrustumSO = nullptr;
MyTerrain* m_terrain = nullptr;
INANOA::MyCameraManager* m_myCameraManager = nullptr;
// ==============================================

void resize_impl(int w, int h);

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
	defaultRenderer->renderPass();

	// rendering with god view
	defaultRenderer->setViewport(godViewport[0], godViewport[1], godViewport[2], godViewport[3]);
	defaultRenderer->setView(godVM);
	defaultRenderer->setProjection(godProjMat);
	defaultRenderer->renderPass();
	// ===============================
}

inline void on_gui()
{
	// Show statistics window

	ImGui::Begin("Information");
	m_imguiPanel->update();
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
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
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