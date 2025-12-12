#pragma once

#include <string>

class MyImGuiPanel
{
public:
	MyImGuiPanel();
	virtual ~MyImGuiPanel();

public:
	void update();
	void setAvgFPS(const double avgFPS);
	void setAvgFrameTime(const double avgFrameTime);
	void setDepthMipLevel(const int level) { m_depthMipLevel = level; }
	int depthMipLevel() const { return m_depthMipLevel; }

private:
	double m_avgFPS;
	double m_avgFrameTime;
	int m_depthMipLevel = 0;
};

