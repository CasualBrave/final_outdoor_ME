#pragma once

#include <fstream>

class MyPoissonSample
{
public:
	MyPoissonSample(){}
	virtual ~MyPoissonSample() {
		delete[] this->m_positions;
	}

public:
	int m_numSample = 0;
	float* m_positions = nullptr;
	float* m_radians = nullptr;

public:
	static MyPoissonSample* fromFile(const std::string& fileFullpath) {
		// load poisson samples
		int numPoissonSample = -1;
		std::ifstream ppInput(fileFullpath, std::ios::binary);
		ppInput.read((char*)(&numPoissonSample), sizeof(int));
		float* poissonSamples = new float[numPoissonSample * 3];
		ppInput.read((char*)(poissonSamples), sizeof(float) * 3 * numPoissonSample);
		float* angles = new float[numPoissonSample * 3];
		ppInput.read((char*)(angles), sizeof(float) * 3 * numPoissonSample);
		ppInput.close();

		MyPoissonSample* mps = new MyPoissonSample();
		mps->m_numSample = numPoissonSample;
		mps->m_positions = poissonSamples;
		mps->m_radians = angles;

		return mps;
	}

	void setPosition(const int idx, const float x, const float y, const float z) {
		this->m_positions[idx * 3 + 0] = x;
		this->m_positions[idx * 3 + 1] = y;
		this->m_positions[idx * 3 + 2] = z;
	}
	void setRadians(const int idx, const float x, const float y, const float z) {
		this->m_radians[idx * 3 + 0] = x;
		this->m_radians[idx * 3 + 1] = y;
		this->m_radians[idx * 3 + 2] = z;
	}
	void exportBinaryFile(std::ostream& outputStream) {
		outputStream.write((char*)(&(this->m_numSample)), sizeof(int));
		outputStream.write((char*)(this->m_positions), sizeof(float) * this->m_numSample * 3);
		outputStream.write((char*)(this->m_radians), sizeof(float) * this->m_numSample * 3);
	}
};

