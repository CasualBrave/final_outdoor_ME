#pragma once

#include <glm\gtx\transform.hpp>
#include <glm\mat4x4.hpp>

namespace INANOA {

class MyCamera
{
public:
	MyCamera();
	MyCamera(const glm::vec3& viewOrg, const glm::vec3& lookCenter, const glm::vec3& upVector, const float distance);
	virtual ~MyCamera();

public:
	void reset(const glm::vec3& viewOrg, const glm::vec3& lookCenter, const glm::vec3& upVector, const float distance);
	void setViewOrg(const glm::vec3& org);
	void setLookCenter(const glm::vec3& center);
	void setUpVector(const glm::vec3& upVec);
	void setDistance(const float dis);
	void distanceOffset(const float offset);
	void update();

	void setViewOrigBasedOnLookCenter(const glm::vec3& tVec);
	void translateLookCenterAndViewOrg(const glm::vec3& t);

	void forward(const glm::vec3& forwardMagnitude, const bool disableYDimension);
	void rotateLookCenterAccordingToViewOrg(const float rad);

public:
	static glm::vec3 rotateLookCenterAccordingToViewOrg(const glm::vec3& center, const glm::vec3& eye, const glm::mat4& viewMat, const float rad);

public:
	glm::mat4 viewMatrix() const;
	glm::vec3 viewOrig() const;
	glm::vec3 lookCenter() const;
	glm::vec3 upVector() const;

private:
	glm::vec3 m_viewOrg;
	glm::vec3 m_lookCenter;
	glm::vec3 m_upVector;
		
	glm::mat4 m_viewMat;

	float m_distance;

	const float MIN_DISTANCE;

};


}


