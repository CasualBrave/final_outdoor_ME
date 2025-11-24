#define GLM_ENABLE_EXPERIMENTAL
#include "MyOrbitControl.h"
#include <glm\gtx\quaternion.hpp>

#include <iostream>

namespace INANOA {




MyOrbitControl::MyOrbitControl(const int w, const int h)
{
	this->m_currState = MyTrackballState::IDLE;
	this->m_currFunc = MyTrackballFunction::UNKNOWN;
	this->m_camera = nullptr;

	this->resize(w, h);

	this->m_currPhi = 0.0;
	this->m_currTheta = 0.0;

	this->m_zoomSpeed = 5.0;
	this->m_panSpeed = 20.0;

}


MyOrbitControl::~MyOrbitControl()
{
}

void MyOrbitControl::setCamera(MyCamera* camera) {
	glm::vec3 centerToEyeVec = glm::normalize(camera->viewOrig() - camera->lookCenter());

	if (glm::abs(centerToEyeVec.x) < 0.0001) {
		this->m_currTheta = glm::pi<float>() * 0.5f * glm::sign(centerToEyeVec.z);
	}
	else {
		this->m_currTheta = glm::atan(centerToEyeVec.z / centerToEyeVec.x);
	}
	this->m_currPhi = glm::acos(centerToEyeVec.y);

	this->m_camera = camera;
}


void MyOrbitControl::rotateCamera() {
	const glm::vec3 MOVE_VEC(
		this->m_moveCurr.x - this->m_movePrev.x,
		this->m_moveCurr.y - this->m_movePrev.y,
		0.0
	);

	// calculate rotation magnitude according to the movement of the mouse
	const float EPS = 0.000001;
	float angle = glm::length(MOVE_VEC);
	if(angle < EPS){
		// no rotation applied 
		return;
	}

	const float PHI = MOVE_VEC.y;
	const float THETA = MOVE_VEC.x;

	this->m_currPhi = this->m_currPhi + PHI;
	this->m_currTheta = this->m_currTheta + THETA;

	glm::vec3 centerToEyeVec(
		glm::cos(this->m_currTheta) * glm::sin(this->m_currPhi),
		glm::cos(this->m_currPhi),
		glm::sin(this->m_currTheta) * glm::sin(this->m_currPhi)
	);

	this->m_camera->setViewOrigBasedOnLookCenter(centerToEyeVec);
	// also update up vector
	this->m_camera->setUpVector(glm::vec3(0.0, glm::cos(this->m_currPhi - glm::pi<float>() * 0.5), 0.0));
	
	glm::mat4 rotMat = glm::mat4(1.0);
	
	// update mouse position
	this->m_movePrev = this->m_moveCurr;
}
void MyOrbitControl::panCamera() {
	glm::vec2 magnitude = this->m_panEnd - this->m_panStart;
	magnitude = magnitude * glm::vec2(-1.0,  1.0) * this->m_panSpeed;

	this->m_camera->forward(glm::vec3(magnitude, 0.0), false);
	/*
	glm::mat4 vmt = glm::transpose(this->m_camera->viewMatrix()); 
	glm::vec4 forward = vmt * glm::vec4(magnitude, 0, 0);

	this->m_camera->translateLookCenterAndViewOrg(forward);	
	*/
	this->m_panStart = this->m_panEnd;
}


glm::vec2 MyOrbitControl::normalizedPosition(const glm::vec2& screenPos) const {
	return glm::vec2(screenPos.x / this->m_frameWidth, screenPos.y / this->m_frameHeight);
}
glm::vec2 MyOrbitControl::positionOnSphere(const glm::vec2& pt) const {
	return glm::vec2(
		(pt.x - this->m_frameWidth * 0.5) / (this->m_frameWidth * 0.5),
		(this->m_frameHeight + 2.0 * (0.0 - pt.y)) / this->m_frameWidth
	);
}
glm::mat4 MyOrbitControl::myAxisAngle(const float angle, const glm::vec3& axis) {
	float w = glm::cos(angle / 2);
	float v = glm::sin(angle / 2);
	glm::vec3 qv = axis * v;

	glm::quat quaternion(w, qv);
	glm::mat4 quatTransform = glm::mat4_cast(quaternion);

	return quatTransform;
}
void MyOrbitControl::resize(const int w, const int h) {
	this->m_frameWidth = w * 1.0;
	this->m_frameHeight = h * 1.0;
}
void MyOrbitControl::update() {
	if (this->m_currState == MyTrackballState::IDLE) {
		return;
	}
	else if (this->m_currState == MyTrackballState::ROTATE) {
		this->rotateCamera();
	}
	else if (this->m_currState == MyTrackballState::PAN) {
		this->panCamera();
	}
}

void MyOrbitControl::setFunction(const MyTrackballFunction func) {
	this->m_currFunc = func;
}
void MyOrbitControl::mousePress(const float x, const float y) {
	if (this->m_currState != MyTrackballState::IDLE) {
		return;
	}
	if (this->m_currFunc == MyTrackballFunction::ROTATE) {
		// initialize
		this->m_moveCurr = this->positionOnSphere(glm::vec2(x, y));
		this->m_movePrev = this->m_moveCurr;
		this->m_currState = MyTrackballState::ROTATE;
	}
	else if (this->m_currFunc == MyTrackballFunction::PAN) {
		// initialize
		this->m_panStart = this->normalizedPosition(glm::vec2(x, y));
		this->m_panEnd = this->m_panStart;
		this->m_currState = MyTrackballState::PAN;
	}
}
void MyOrbitControl::mouseMove(const float x, const float y) {
	if (this->m_currState == MyTrackballState::IDLE) {
		return;
	}
	if (this->m_currState == MyTrackballState::ROTATE) {
		// initialize
		this->m_movePrev = this->m_moveCurr;
		this->m_moveCurr = this->positionOnSphere(glm::vec2(x, y));
	}
	else if (this->m_currState == MyTrackballState::PAN) {
		this->m_panEnd = this->normalizedPosition(glm::vec2(x, y));
	}
}
void MyOrbitControl::mouseRelease(const float x, const float y) {
	this->m_currState = MyTrackballState::IDLE;
}
void MyOrbitControl::mouseScroll(const float delta) {
	if (delta > 0.0) {
		this->m_camera->distanceOffset(0.1 * this->m_zoomSpeed);
	}
	else {
		this->m_camera->distanceOffset(-0.1 * this->m_zoomSpeed);
	}
}



}


