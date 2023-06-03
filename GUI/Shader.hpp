#pragma once

#include <glad/glad.h>

#include <concepts>
#include <filesystem>

struct Shader_t final
{
	GLuint m_Id{};

	// constructor generates the shader on the fly
	// ------------------------------------------------------------------------
	static Shader_t CompileFromFile(std::filesystem::path const &vertexPath, std::filesystem::path const &fragmentPath) noexcept;
	static Shader_t CompileFromText(const char* vertex, const char* fragment) noexcept;

	// activate the shader
	// ------------------------------------------------------------------------
	inline void Use() const noexcept { glUseProgram(m_Id); }

	// utility uniform functions
	// ------------------------------------------------------------------------
	template <typename T> requires (std::is_arithmetic_v<T>)
	void GLUniform(const char* psz, T value) const noexcept
	{
		if constexpr (std::integral<T>)
			glUniform1i(glGetUniformLocation(m_Id, psz), (int)value);
		else if constexpr (std::floating_point<T>)
			glUniform1f(glGetUniformLocation(m_Id, psz), value);

		// #UPDATE_AT_CPP23 static_assert(false);
	}
};
