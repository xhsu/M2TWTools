#pragma once

#include <experimental/generator>
#include <filesystem>
#include <string_view>

bool wcsieql(std::wstring_view lhs, std::wstring_view rhs) noexcept;

std::experimental::generator<std::string_view> UTIL_Split(std::string_view sz, std::string_view delimiters) noexcept;

class CBaseParser
{
public:
	CBaseParser() noexcept = default;
	explicit CBaseParser(std::filesystem::path const& Path) noexcept;

	virtual ~CBaseParser() noexcept;

	CBaseParser(CBaseParser const&) noexcept = delete;
	CBaseParser(CBaseParser&&) noexcept = delete;

	CBaseParser& operator= (CBaseParser const&) noexcept = delete;
	CBaseParser& operator= (CBaseParser&&) noexcept = delete;

public:
	explicit operator std::string_view() const noexcept { return std::string_view{m_p, m_length}; }

protected:
	char* m_p{};
	size_t m_length{};
	const char* m_cur{};

	void Skip(int32_t iCount = 1) noexcept;
	void SkipUntilNonspace(void) noexcept;
	void Rewind(int32_t iCount = 1) noexcept;
	void RewindUntilNonspace(void) noexcept;

	std::string_view Parse(std::string_view delimiters = " \n\t\f\v\r", bool bLeftTrim = true, bool bRightTrim = true) noexcept;
	std::string_view ReadN(uint32_t iCount) noexcept;

	bool Eof(void) const noexcept { return m_cur >= (m_p + m_length); }
	void Seek(ptrdiff_t iOffset, int iMode = SEEK_CUR) noexcept;
};
