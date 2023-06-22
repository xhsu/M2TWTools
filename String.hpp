#pragma once

#include <experimental/generator>
#include <filesystem>
#include <string_view>

template <typename T>
concept RangeOfStr = std::ranges::input_range<T> && std::convertible_to<std::ranges::range_value_t<T>, ::std::string_view>;

inline constexpr bool IsSpace(char const c) noexcept
{
	if (c & 0b10000000)
		return false;

	return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

inline constexpr char ToLower(char const c) noexcept
{
	if ('A' <= c && c <= 'Z')
		return static_cast<char>(c - 'A' + 'a');

	return c;
}

inline constexpr char ToUpper(char const c) noexcept
{
	if ('a' <= c && c <= 'z')
		return static_cast<char>(c - 'a' + 'A');

	return c;
}

inline constexpr wchar_t ToWLower(wchar_t const c) noexcept
{
	if (L'A' <= c && c <= L'Z')
		return static_cast<wchar_t>(c - L'A' + L'a');

	return c;
}

inline constexpr wchar_t ToWUpper(wchar_t const c) noexcept
{
	if (L'a' <= c && c <= L'z')
		return static_cast<wchar_t>(c - L'a' + L'A');

	return c;
}

extern bool strieql(std::string_view lhs, std::string_view rhs) noexcept;
extern bool wcsieql(std::wstring_view lhs, std::wstring_view rhs) noexcept;
extern bool StartsWith_I(std::string_view text, std::string_view what) noexcept;
extern bool StartsWith_I(std::wstring_view text, std::wstring_view what) noexcept;

extern std::experimental::generator<std::string_view> UTIL_Split(std::string_view sz, std::string_view delimiters = ", \n\f\v\t\r", bool bLTrim = true) noexcept;

extern std::string ToUTF8(std::wstring_view wsz) noexcept;
extern std::wstring ToUTF16(std::string_view sz) noexcept;

class CBaseParser
{
	friend extern int main(int, char* []) noexcept;

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

	inline auto cbegin() const noexcept { return m_p; }
	inline auto cend() const noexcept { return m_p + m_length; }
	inline auto crbegin() const noexcept { return std::reverse_iterator{ cend() }; }
	inline auto crend() const noexcept { return std::reverse_iterator{ cbegin() }; }

protected:
	char* m_p{};
	size_t m_length{};
	const char* m_cur{};

	void Skip(int32_t iCount = 1) noexcept;
	void SkipUntilNonspace(void) noexcept;
	void Rewind(int32_t iCount = 1) noexcept;
	void RewindUntilNonspace(void) noexcept;

	std::string_view Parse(std::string_view delimiters = " \n\t\f\v\r", bool bLeftTrim = true, bool bRightTrim = true) noexcept;
	std::string_view Parse(uint32_t iCount) noexcept;
	std::string_view Peek(std::string_view delimiters = " \n\t\f\v\r", bool bLeftTrim = true, bool bRightTrim = true) const noexcept;
	std::string_view Peek(uint32_t iCount) const noexcept;
	inline auto Now() const noexcept { return *m_cur; }

	bool Eof(void) const noexcept { return m_cur > cend() && cbegin() <= m_cur; }
	void Seek(ptrdiff_t iOffset, int iMode = SEEK_CUR) noexcept;
	auto Tell() const noexcept { return m_cur - m_p; }
};

struct CaseIgnoredHash final
{
	// #UPDATE_AT_CPP23 static operator()

	// Hash
	size_t operator() (std::string_view const& sz) const noexcept;
	size_t operator() (std::wstring_view const& sz) const noexcept;

	// Equal
	__forceinline bool operator() (std::string_view const& lhs, std::string_view const& rhs) const noexcept { return strieql(lhs, rhs); }
	__forceinline bool operator() (std::wstring_view const& lhs, std::wstring_view const& rhs) const noexcept { return wcsieql(lhs, rhs); }
};

struct CaseIgnoredLess final
{
	// #UPDATE_AT_CPP23 static operator()

	// Less
	bool operator() (std::string_view const& lhs, std::string_view const& rhs) const noexcept;
	bool operator() (std::wstring_view const& lhs, std::wstring_view const& rhs) const noexcept;
};
