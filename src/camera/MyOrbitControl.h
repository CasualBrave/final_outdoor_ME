#pragma once

#include "MyCamera.h"

namespace INANOA {

enum class MyTrackballFunction {
	ROTATE, ZOOM, PAN, UNKNOWN
};
enum class MyTrackballState {
	ROTATE, ZOOM, PAN, IDLE
};

class MyOrbitControl
{
public:
	MyOrbitControl(const int w, const int h);
	virtual ~MyOrbitControl();

public:
	void setCamera(MyCamera* camera);
	void resize(const int w, const int h);
	void update();

	void setFunction(const MyTrackballFunction func);
	void mousePress(const float x, const float y);
	void mouseMove(const float x, const float y);
	void mouseRelease(const float x, const float y);
	void mouseScroll(const float delta);

private:
	MyTrackballState m_currState;
	MyTrackballFunction m_currFunc;
	float m_frameWidth;
	float m_frameHeight; 
	
	MyCamera* m_camera;

	// rotate parameters	
	glm::vec2 m_moveCurr;
	glm::vec2 m_movePrev;
	float m_roateSpeed = 1.0;

	// pan parameters
	glm::vec2 m_panEnd;
	glm::vec2 m_panStart;
	float m_panSpeed = 10.0;

	// zoom
	float m_zoomSpeed = 1.0;

	float m_angleSign = 1.0;

	float m_currPhi;
	float m_currTheta;

private:
	glm::vec2 normalizedPosition(const glm::vec2& screenPos) const;
	glm::vec2 positionOnSphere(const glm::vec2& pt) const;	

	void rotateCamera();
	void panCamera();

public:
	static glm::mat4 myAxisAngle(const float angle, const glm::vec3& axis);

};


}


