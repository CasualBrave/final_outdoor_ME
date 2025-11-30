#define GLM_ENABLE_EXPERIMENTAL
#include "MyCameraManager.h"
#include <glm\gtc\matrix_transform.hpp>

namespace INANOA {
MyCameraManager::MyCameraManager() : MIN_PLAYER_CAMERA_TERRAIN_OFFSET(5.0), MIN_AIRPLANE_TERRAIN_OFFSET(3.0)
{
}


MyCameraManager::~MyCameraManager()
{
}

void MyCameraManager::init(const int w, const int h){
	// initialize camera and camera control
	this->m_godCameraControl = new INANOA::MyOrbitControl(w, h);
	this->m_godMyCamera = new INANOA::MyCamera(glm::vec3(50.0, 120.0, 100.0), glm::vec3(50.0, 0.0, 60.0), glm::vec3(0.0, 1.0, 0.0), -1.0);
	this->m_godCameraControl->setCamera(this->m_godMyCamera);
	this->m_playerCameraForwardSpeed = 0.4;
	// initialize player camera
	this->m_playerMyCamera = new INANOA::MyCamera(glm::vec3(50.0, 50.0, 120.0), glm::vec3(50.0, 47.0, 110.0), glm::vec3(0.0, 1.0, 0.0), -1.0);

	this->resize(w, h);
}
void MyCameraManager::resize(const int w, const int h){
	// half for god view, half for player view
	const float PLAYER_PROJ_FAR = 700.0;
	this->setupViewports(w, h);

	this->m_godProjMat = glm::perspective(glm::radians(80.0f), this->m_godViewport[2] * 1.0f / h, 0.1f, 1000.0f);
	this->m_playerProjMat = glm::perspective(glm::radians(45.0f), this->m_playerViewport[2] * 1.0f / h, 0.1f, PLAYER_PROJ_FAR);

	// trackball use full screen
	this->m_godCameraControl->resize(w, h);
}

void MyCameraManager::updateGodCamera() {
	this->m_godCameraControl->update();
	this->m_godMyCamera->update();
}


void MyCameraManager::mousePress(const RenderWidgetMouseButton button, const int x, const int y) {
	if (button == RenderWidgetMouseButton::M_LEFT) {
		this->m_godCameraControl->setFunction(INANOA::MyTrackballFunction::ROTATE);
		this->m_godCameraControl->mousePress(x, y);
	}
	else if (button == RenderWidgetMouseButton::M_RIGHT) {
		this->m_godCameraControl->setFunction(INANOA::MyTrackballFunction::PAN);
		this->m_godCameraControl->mousePress(x, y);
	}
}
void MyCameraManager::mouseRelease(const RenderWidgetMouseButton button, const int x, const int y) {
	if (button == RenderWidgetMouseButton::M_LEFT) {
		this->m_godCameraControl->mouseRelease(x, y);
	}
	else if (button == RenderWidgetMouseButton::M_RIGHT) {
		this->m_godCameraControl->mouseRelease(x, y);
	}
}
void MyCameraManager::mouseMove(const int x, const int y) {
	this->m_godCameraControl->mouseMove(x, y);
}
void MyCameraManager::mouseScroll(const double xOffset, const double yOffset) {
	this->m_godCameraControl->mouseScroll(yOffset);
}
void MyCameraManager::keyPress(const RenderWidgetKeyCode key) {
	if (key == RenderWidgetKeyCode::KEY_W) {
		this->m_WPressedFlag = true;
	}
	else if (key == RenderWidgetKeyCode::KEY_S) {
		this->m_SPressedFlag = true;
	}
	else if (key == RenderWidgetKeyCode::KEY_A) {
		this->m_APressedFlag = true;
	}
	else if (key == RenderWidgetKeyCode::KEY_D) {
		this->m_DPressedFlag = true;
	}
	else if (key == RenderWidgetKeyCode::KEY_Z) {
		this->m_playerCameraHeightOffset = 0.1;
	}
	else if (key == RenderWidgetKeyCode::KEY_X) {
		this->m_playerCameraHeightOffset = -0.1;
	}	
}
void MyCameraManager::keyRelease(const RenderWidgetKeyCode key) {
	if (key == RenderWidgetKeyCode::KEY_W) {
		this->m_WPressedFlag = false;
	}
	else if (key == RenderWidgetKeyCode::KEY_S) {
		this->m_SPressedFlag = false;
	}
	else if (key == RenderWidgetKeyCode::KEY_A) {
		this->m_APressedFlag = false;
	}
	else if (key == RenderWidgetKeyCode::KEY_D) {
		this->m_DPressedFlag = false;
	}

	if (key == RenderWidgetKeyCode::KEY_Z || key == RenderWidgetKeyCode::KEY_X) {
		this->m_playerCameraHeightOffset = 0.0;
	}
}

void MyCameraManager::updatePlayerCamera() {

	// update player camera
	if (this->m_WPressedFlag) {
		glm::vec3 before = this->m_playerMyCamera->lookCenter();
		this->m_playerMyCamera->forward(glm::vec3(0.0, 0.0, -1.0) * this->m_playerCameraForwardSpeed, true);
		glm::vec3 after = this->m_playerMyCamera->lookCenter();

		this->m_godMyCamera->translateLookCenterAndViewOrg(after - before);
	}
	else if (this->m_SPressedFlag) {
		glm::vec3 before = this->m_playerMyCamera->lookCenter();
		this->m_playerMyCamera->forward(glm::vec3(0.0, 0.0, 1.0) * this->m_playerCameraForwardSpeed, true);
		glm::vec3 after = this->m_playerMyCamera->lookCenter();

		this->m_godMyCamera->translateLookCenterAndViewOrg(after - before);
	}
	else if (this->m_APressedFlag) {
		this->m_playerMyCamera->rotateLookCenterAccordingToViewOrg(0.01);
	}
	else if (this->m_DPressedFlag) {
		this->m_playerMyCamera->rotateLookCenterAccordingToViewOrg(-0.01);
	}

	this->m_playerMyCamera->translateLookCenterAndViewOrg(glm::vec3(0.0, this->m_playerCameraHeightOffset, 0.0));
	this->m_playerMyCamera->update();	
}
void MyCameraManager::updateAirplane() {
	const glm::mat4 playerVM = this->m_playerMyCamera->viewMatrix();
	const glm::vec3 playerViewOrg = this->m_playerMyCamera->viewOrig();

	glm::vec3 vTangent = glm::vec3(-1.0 * playerVM[0].z, -0.2, -1.0 * playerVM[2].z);
	vTangent = glm::normalize(vTangent);
	glm::vec3 airplaneCenter = playerViewOrg + vTangent * 40.0f;	

	// update airplane orientation
	glm::mat4 rm = this->m_playerMyCamera->viewMatrix();
	rm = glm::transpose(rm);
	glm::vec3 bitangent = glm::normalize(glm::cross(glm::vec3(0.0, 1.0, 0.0), vTangent));
	glm::vec3 normal = glm::normalize(glm::cross(vTangent, bitangent));
	glm::mat4 rotMat(1.0);
	rotMat[0] = glm::vec4(bitangent, 0.0);
	rotMat[1] = glm::vec4(normal, 0.0);
	rotMat[2] = glm::vec4(vTangent, 0.0);
	rotMat[3] = glm::vec4(airplaneCenter.x, airplaneCenter.y, airplaneCenter.z, 1.0);

	this->m_airplaneModelMat = rotMat;
}

void MyCameraManager::adjustAirplaneHeight(const float terrainHeight) {
	float airplaneHeight = this->m_airplaneModelMat[3].y;

	if (airplaneHeight - this->MIN_PLAYER_CAMERA_TERRAIN_OFFSET < terrainHeight) {
		airplaneHeight = terrainHeight + MIN_PLAYER_CAMERA_TERRAIN_OFFSET;
	}

	this->m_airplaneModelMat[3].y = airplaneHeight;
}
void MyCameraManager::adjustPlayerCameraHeight(const float terrainHeight) {
	// adjust with terrain height
	const glm::vec3 playerViewOrg_A = this->m_playerMyCamera->viewOrig();
	const float ELEVATION_THRESHOLD = terrainHeight + this->MIN_AIRPLANE_TERRAIN_OFFSET;

	if (playerViewOrg_A.y < ELEVATION_THRESHOLD) {
		this->m_playerMyCamera->translateLookCenterAndViewOrg(glm::vec3(0.0, (ELEVATION_THRESHOLD - playerViewOrg_A.y), 0.0));
	}

	this->m_playerMyCamera->update();
}
void MyCameraManager::setupViewports(const int w, const int h) {
    int halfW = w / 2;

    // god view on the LEFT
    this->m_godViewport = glm::ivec4(0, 0, halfW, h);

    // player view on the RIGHT
    this->m_playerViewport = glm::ivec4(halfW, 0, w - halfW, h);
}
void MyCameraManager::teleport(const int idx) {
	auto tele = [](INANOA::MyCamera* godCamera, INANOA::MyCamera* playerCamera, const glm::vec3& viewOrg, const glm::vec3& lookCenter) {
		const glm::vec3 moveVec = lookCenter - playerCamera->lookCenter();
		playerCamera->reset(viewOrg, lookCenter, glm::vec3(0.0, 1.0, 0.0), -1.0);
		godCamera->translateLookCenterAndViewOrg(moveVec);
	};

	if (idx == 0) {
		tele(this->m_godMyCamera, this->m_playerMyCamera, glm::vec3(50.0, 50.0, 120.0), glm::vec3(50.0, 47.0, 110.0));
	}
	else if (idx == 1) {
		tele(this->m_godMyCamera, this->m_playerMyCamera, glm::vec3(30.2523, 49.9996, 498.473), glm::vec3(26.8622, 47.2784, 488.98));
	}
	else if (idx == 2) {
		tele(this->m_godMyCamera, this->m_playerMyCamera, glm::vec3(119.602, 49.9999, 940.079), glm::vec3(115.248, 47.2317, 931.003));
	}
	
}

// ===============================
glm::mat4 MyCameraManager::playerViewMatrix() const { return this->m_playerMyCamera->viewMatrix(); }
glm::mat4 MyCameraManager::playerProjectionMatrix() const { return this->m_playerProjMat; }
glm::vec3 MyCameraManager::playerViewOrig() const { return this->m_playerMyCamera->viewOrig(); }
glm::vec3 MyCameraManager::playerCameraLookCenter() const { return this->m_playerMyCamera->lookCenter(); }

glm::mat4 MyCameraManager::godViewMatrix() const { return this->m_godMyCamera->viewMatrix(); }
glm::mat4 MyCameraManager::godProjectionMatrix() const { return this->m_godProjMat; }

glm::mat4 MyCameraManager::airplaneModelMatrix() const { return this->m_airplaneModelMat; }
glm::vec3 MyCameraManager::airplanePosition() const { return glm::vec3(this->m_airplaneModelMat[3]); }

glm::ivec4 MyCameraManager::playerViewport() const { return this->m_playerViewport; }
glm::ivec4 MyCameraManager::godViewport() const { return this->m_godViewport; }

float MyCameraManager::playerCameraNear() const { return 0.1; }
float MyCameraManager::playerCameraFar() const { return 400.0; }

}

