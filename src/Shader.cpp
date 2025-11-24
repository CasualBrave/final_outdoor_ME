#include "Shader.h"


// version: 221030
Shader::Shader(const GLenum shaderType) : m_shaderType(shaderType), m_shaderId(0) {
	this->m_shaderStatus = ShaderStatus::NULL_SHADER_CODE;
}
Shader::~Shader() {}
void Shader::appendShaderCode(const std::string& code) {
	this->m_shaderCode = code;
	this->m_shaderStatus = ShaderStatus::NULL_SHADER;
}
bool Shader::compileShader() {
	if (this->status() != ShaderStatus::NULL_SHADER) {
		return false;
	}

	this->m_shaderId = glCreateShader(this->m_shaderType);
	const char* shaderCode = this->m_shaderCode.c_str();
	glShaderSource(this->m_shaderId, 1, &shaderCode, nullptr);
	glCompileShader(this->m_shaderId);

	// check compilation errors
	int params[2];
	glGetShaderiv(this->m_shaderId, GL_COMPILE_STATUS, params);

	// compile failed
	if (params[0] != GL_TRUE) {
		char compileError[1024];
		glGetShaderInfoLog(this->m_shaderId, 1024, nullptr, compileError);
		this->m_shaderInfoLog = compileError;
		return false;
	}
	// compile success	
	this->m_shaderInfoLog = "ready";
	this->m_shaderStatus = ShaderStatus::READY;

	return true;
}
void Shader::releaseShader() {
	if (this->status() != ShaderStatus::READY) {
		return;
	}
	this->m_shaderStatus = ShaderStatus::NULL_SHADER;
	glDeleteShader(this->m_shaderId);	
}
bool Shader::createShaderFromFile(const std::string& fileFullpath) {
	// read shader code from file
	std::ifstream inputStream;
	inputStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	std::string shaderCode;
	try {
		inputStream.open(fileFullpath);
		std::stringstream shaderCodeStream;
		shaderCodeStream << inputStream.rdbuf();
		inputStream.close();

		shaderCode = shaderCodeStream.str();
	}
	catch (std::ifstream::failure& e) {
		this->m_shaderInfoLog = e.what();
		this->m_shaderStatus = ShaderStatus::NULL_SHADER_CODE;
		return false;
	}

	// append shader code
	this->appendShaderCode(shaderCode);

	// compile shader
	return this->compileShader();
}
std::string Shader::shaderInfoLog() const { return this->m_shaderInfoLog; }
ShaderStatus Shader::status() const { return this->m_shaderStatus; }
GLuint Shader::shaderId() const { return this->m_shaderId; }
GLenum Shader::shaderType() const { return this->m_shaderType; }

// ========================================================
ShaderProgram::ShaderProgram() : m_programId(0) {
	this->m_shaderProgramStatus = ShaderProgramStatus::NULL_VERTEX_SHADER_FRAGMENT_SHADER;
	this->m_vsReady = false;
	this->m_fsReady = false;
}
ShaderProgram::~ShaderProgram() {}
bool ShaderProgram::init() {
	this->m_programId = glCreateProgram();

	// error occurs
	if (this->m_programId <= 0) {
		return false;
	}

	this->m_shaderProgramStatus = ShaderProgramStatus::PROGRAM_ID_READY;
	return true;
}
bool ShaderProgram::attachShader(const Shader* shader) {
	if (this->m_shaderProgramStatus != ShaderProgramStatus::PROGRAM_ID_READY) {
		return false;
	}
	if (shader->status() != ShaderStatus::READY) {
		return false;
	}
	glAttachShader(this->m_programId, shader->shaderId());

	if (shader->shaderType() == GL_VERTEX_SHADER) {
		this->m_vsReady = true;
	}
	else if (shader->shaderType() == GL_FRAGMENT_SHADER) {
		this->m_fsReady = true;
	}
	else if (shader->shaderType() == GL_COMPUTE_SHADER) {
		this->m_csReady = true;
	}
	return true;
}
ShaderProgramStatus ShaderProgram::checkStatus() {
	// compute shader program
	if (this->m_csReady == true) {
		this->m_shaderProgramStatus = ShaderProgramStatus::READY;
	}
	// render program
	else if (this->m_vsReady == false && this->m_fsReady == false) {
		this->m_shaderProgramStatus = ShaderProgramStatus::NULL_VERTEX_SHADER_FRAGMENT_SHADER;
	}
	else if (this->m_vsReady == false) {
		this->m_shaderProgramStatus = ShaderProgramStatus::NULL_VERTEX_SHADER;
	}
	else if (this->m_fsReady == false) {
		this->m_shaderProgramStatus = ShaderProgramStatus::NULL_FRAGMENT_SHADER;
	}
	else {
		this->m_shaderProgramStatus = ShaderProgramStatus::READY;
	}

	return this->m_shaderProgramStatus;
}
void ShaderProgram::linkProgram() {
	if (this->m_shaderProgramStatus != ShaderProgramStatus::READY) {
		return;
	}

	glLinkProgram(this->m_programId);
}
void ShaderProgram::useProgram() {
	if (this->m_shaderProgramStatus != ShaderProgramStatus::READY) {
		return;
	}

	glUseProgram(this->m_programId);
}

GLuint ShaderProgram::programId() const { return this->m_programId; }
ShaderProgramStatus ShaderProgram::status() const { return this->m_shaderProgramStatus; }