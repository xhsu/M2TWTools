#include "Shader.hpp"

#include <fmt/color.h>

#include <fstream>

// utility function for checking shader compilation/linking errors.
// ------------------------------------------------------------------------
void checkCompileErrors(GLuint shader, std::string_view type) noexcept
{
	int success{};
	char infoLog[1024]{};

	if (type != "PROGRAM")
	{
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

		if (!success)
		{
			glGetShaderInfoLog(shader, sizeof(infoLog) - 1, nullptr, infoLog);

			fmt::print(fg(fmt::color::red), "ERROR::SHADER_COMPILATION_ERROR of type: {}\n", type);
			fmt::print(fg(fmt::color::gray), "{}\n -- --------------------------------------------------- -- \n", infoLog);
		}
	}
	else
	{
		glGetProgramiv(shader, GL_LINK_STATUS, &success);

		if (!success)
		{
			glGetProgramInfoLog(shader, sizeof(infoLog) - 1, nullptr, infoLog);

			fmt::print(fg(fmt::color::red), "ERROR::PROGRAM_LINKING_ERROR of type: {}\n", type);
			fmt::print(fg(fmt::color::gray), "{}\n -- --------------------------------------------------- -- \n", infoLog);
		}
	}
}

Shader_t Shader_t::CompileFromFile(std::filesystem::path const &vertexPath, std::filesystem::path const &fragmentPath) noexcept
{
	// 1. retrieve the vertex/fragment source code from filePath
	std::string vertexCode;
	std::string fragmentCode;
	std::ifstream vShaderFile;
	std::ifstream fShaderFile;

	// ensure ifstream objects can throw exceptions:
	vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try
	{
		// open files
		vShaderFile.open(vertexPath);
		fShaderFile.open(fragmentPath);
		std::stringstream vShaderStream, fShaderStream;
		// read file's buffer contents into streams
		vShaderStream << vShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();
		// close file handlers
		vShaderFile.close();
		fShaderFile.close();
		// convert stream into string
		vertexCode = vShaderStream.str();
		fragmentCode = fShaderStream.str();
	}
	catch (std::ifstream::failure &e)
	{
		fmt::print(fg(fmt::color::red), "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: {}\n", e.what());
		return Shader_t{};
	}

	const char *vShaderCode = vertexCode.c_str();
	const char *fShaderCode = fragmentCode.c_str();

	// 2. compile shaders
	GLuint vertex{}, fragment{};

	// vertex shader
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vShaderCode, nullptr);
	glCompileShader(vertex);
	checkCompileErrors(vertex, "VERTEX");

	// fragment Shader
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fShaderCode, nullptr);
	glCompileShader(fragment);
	checkCompileErrors(fragment, "FRAGMENT");

	// shader Program
	auto const m_Id = glCreateProgram();
	glAttachShader(m_Id, vertex);
	glAttachShader(m_Id, fragment);
	glLinkProgram(m_Id);
	checkCompileErrors(m_Id, "PROGRAM");

	// delete the shaders as they're linked into our program now and no longer necessary
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	return Shader_t{ m_Id };
}

Shader_t Shader_t::CompileFromText(const char *vShaderCode, const char *fShaderCode) noexcept
{
	// 2. compile shaders

	// vertex shader
	auto const vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vShaderCode, nullptr);
	glCompileShader(vertex);
	checkCompileErrors(vertex, "VERTEX");

	// fragment Shader
	auto const fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fShaderCode, nullptr);
	glCompileShader(fragment);
	checkCompileErrors(fragment, "FRAGMENT");

	// shader Program
	auto const m_Id = glCreateProgram();
	glAttachShader(m_Id, vertex);
	glAttachShader(m_Id, fragment);
	glLinkProgram(m_Id);
	checkCompileErrors(m_Id, "PROGRAM");

	// delete the shaders as they're linked into our program now and no longer necessary
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	return Shader_t{ m_Id };
}
