#include "MyImGuiPanel.h"
#include <imgui.h>

MyImGuiPanel::MyImGuiPanel()
{
	this->m_avgFPS = 0.0;
	this->m_avgFrameTime = 0.0;
}


MyImGuiPanel::~MyImGuiPanel()
{
}

void MyImGuiPanel::update() {
	// performance information
	const std::string FPS_STR = "FPS: " + std::to_string(this->m_avgFPS);
	ImGui::TextColored(ImVec4(0, 220, 0, 255), FPS_STR.c_str());
	const std::string FT_STR = "Frame: " + std::to_string(this->m_avgFrameTime);
	ImGui::TextColored(ImVec4(0, 220, 0, 255), FT_STR.c_str());
}

void MyImGuiPanel::setAvgFPS(const double avgFPS) {
	this->m_avgFPS = avgFPS;
}
void MyImGuiPanel::setAvgFrameTime(const double avgFrameTime) {
	this->m_avgFrameTime = avgFrameTime;
}