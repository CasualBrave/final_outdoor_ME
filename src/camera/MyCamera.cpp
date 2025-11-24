#define GLM_ENABLE_EXPERIMENTAL
#include "MyCamera.h"
#include <glm\gtx\quaternion.hpp>

namespace INANOA {

MyCamera::MyCamera(const glm::vec3& viewOrg, const glm::vec3& lookCenter, const glm::vec3& upVector, const float distance) : MIN_DISTANCE(0.1){
	this->reset(viewOrg, lookCenter, upVector, distance);	
}
MyCamera::MyCamera() : MIN_DISTANCE(0.1) {
	this->reset(glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0), -1.0);
}

void MyCamera::reset(const glm::vec3& viewOrg, const glm::vec3& lookCenter, const glm::vec3& upVector, const float distance) {
	this->m_distance = distance;
	if (distance < 0.0) {
		this->m_distance = glm::length(viewOrg - lookCenter);
	}

	this->setViewOrg(viewOrg);
	this->setLookCenter(lookCenter);
	this->setUpVector(upVector);

	this->update();
}

MyCamera::~MyCamera()
{
}

void MyCamera::setViewOrg(const glm::vec3& org) {
	this->m_viewOrg = org;
}
void MyCamera::setLookCenter(const glm::vec3& center) {
	this->m_lookCenter = center;
}
void MyCamera::setUpVector(const glm::vec3& upVec) {
	this->m_upVector = upVec;
}
void MyCamera::setDistance(const float dis) {
	this->m_distance = dis >= MIN_DISTANCE? dis: MIN_DISTANCE;
}
void MyCamera::distanceOffset(const float offset) {
	this->setDistance(this->m_distance + offset);
}
void MyCamera::update() {
	glm::vec3 centerToEyeVec = glm::normalize(this->m_viewOrg - this->m_lookCenter);
	this->setViewOrigBasedOnLookCenter(this->m_distance * centerToEyeVec);

	this->m_viewMat = glm::lookAt(this->m_viewOrg, this->m_lookCenter, this->m_upVector);
}

void MyCamera::setViewOrigBasedOnLookCenter(const glm::vec3& tVec) {
	this->m_viewOrg = this->m_lookCenter + tVec;
}
void MyCamera::translateLookCenterAndViewOrg(const glm::vec3& t) {
	this->m_lookCenter = this->m_lookCenter + t;
	this->m_viewOrg = this->m_viewOrg + t;
}
void MyCamera::forward(const glm::vec3& forwardMagnitude, const bool disableYDimension) {
	glm::mat4 vmt = glm::transpose(this->viewMatrix());
	glm::vec4 forward = vmt * glm::vec4(forwardMagnitude, 0);

	if (disableYDimension) {
		forward.y = 0.0;
	}

	this->translateLookCenterAndViewOrg(forward);
}
void MyCamera::rotateLookCenterAccordingToViewOrg(const float rad) {
	this->setLookCenter(
		MyCamera::rotateLookCenterAccordingToViewOrg(this->m_lookCenter, this->m_viewOrg, this->m_viewMat, rad)
	);
}

glm::mat4 MyCamera::viewMatrix() const {
	return this->m_viewMat;
}
glm::vec3 MyCamera::viewOrig() const {
	return this->m_viewOrg; 
}
glm::vec3 MyCamera::lookCenter() const {
	return this->m_lookCenter; 
}
glm::vec3 MyCamera::upVector() const {
	return this->m_upVector;
}

glm::vec3 MyCamera::rotateLookCenterAccordingToViewOrg(const glm::vec3& center, const glm::vec3& eye, const glm::mat4& viewMat, const float rad) {
	glm::mat4 vmt = glm::transpose(viewMat);
	glm::vec3 yAxis(vmt[1].x, vmt[1].y, vmt[1].z);
	glm::quat q = glm::angleAxis(rad, yAxis);
	glm::mat4 rotMat = glm::toMat4(q);

	glm::vec3 p = center - eye;
	glm::vec4 resP = rotMat * glm::vec4(p.x, p.y, p.z, 1.0);

	return glm::vec3(resP.x + eye.x, resP.y + eye.y, resP.z + eye.z);
}


}


