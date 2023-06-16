#include "String.hpp"

#include <ctype.h>
#include <string.h>
#include <wctype.h>

#include <algorithm>
#include <ranges>

using namespace std;
using namespace std::experimental;
using namespace std::literals;

static inline constexpr bool IsSpace(char const c) noexcept
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

bool wcsieql(std::wstring_view lhs, std::wstring_view rhs) noexcept
{
	return std::ranges::equal(
		lhs, rhs,
		[](wchar_t lc, wchar_t rc) noexcept -> bool { return ToWLower(lc) == ToWLower(rc); }
	);
}

bool strieql(string_view lhs, string_view rhs) noexcept
{
	return std::ranges::equal(
		lhs, rhs,
		[](char lc, char rc) noexcept -> bool { return ToUpper(lc) == ToUpper(rc); }
	);
}

generator<string_view> UTIL_Split(string_view sz, string_view delimiters, bool bLTrim) noexcept
{
	for (auto lastPos = sz.find_first_not_of(delimiters, 0), pos = sz.find_first_of(delimiters, lastPos);
		sz.npos != pos || sz.npos != lastPos;
		lastPos = sz.find_first_not_of(delimiters, pos), pos = sz.find_first_of(delimiters, lastPos)
		)
	{
		if (!bLTrim)
		{
			co_yield sz.substr(lastPos, pos - lastPos);
		}
		else
		{
			co_yield string_view{
				sz.substr(lastPos, pos - lastPos) | std::views::drop_while(IsSpace)
			};
		}
	}

	co_return;
}

CBaseParser::CBaseParser(std::filesystem::path const& Path) noexcept
{
	if (auto f = _wfopen(Path.c_str(), L"rb"); f)
	{
		fseek(f, 0, SEEK_END);
		m_length = (size_t)ftell(f);

		m_p = (char*)calloc(m_length + 1, sizeof(char));
		fseek(f, 0, SEEK_SET);
		fread(m_p, sizeof(char), m_length, f);

		fclose(f);
	}

	m_cur = m_p;
}

CBaseParser::~CBaseParser() noexcept
{
	if (m_p)
	{
		free(m_p);
		m_p = nullptr;
		m_cur = nullptr;
		m_length = 0;
	}
}

void CBaseParser::Skip(int32_t iCount /*= 1*/) noexcept
{
	SkipUntilNonspace();

	for (bool phase{ false }; !Eof() && iCount > 0;)
	{
		if (!phase)
		{
			for (; !Eof() && !IsSpace(*m_cur); ++m_cur) {}
			phase = !phase;
			--iCount;
		}
		else
		{
			for (; !Eof() && IsSpace(*m_cur); ++m_cur) {}
			phase = !phase;
		}
	}

	SkipUntilNonspace();
}

void CBaseParser::SkipUntilNonspace(void) noexcept
{
	for (; !Eof() && IsSpace(*m_cur); ++m_cur) {}
}

void CBaseParser::Rewind(int32_t iCount) noexcept
{
	RewindUntilNonspace();

	reverse_iterator it{ m_cur };
	for (bool phase{ false }; it != crend() && iCount > 0;)
	{
		if (!phase)
		{
			for (; it != crend() && !IsSpace(*it); ++it) {}
			phase = !phase;
			--iCount;
		}
		else
		{
			for (; it != crend() && IsSpace(*it); ++it) {}
			phase = !phase;
		}
	}

	m_cur = it.base();
	RewindUntilNonspace();
}

void CBaseParser::RewindUntilNonspace(void) noexcept
{
	reverse_iterator it{ m_cur };

	for (; it != crend() && (*it == '\0' || IsSpace(*it)); ++it) {}

	m_cur = it.base();
}

std::string_view CBaseParser::Parse(std::string_view delimiters /*= " \n\t\f\v\r"*/, bool bLeftTrim /*= true*/, bool bRightTrim /*= true*/) noexcept
{
	// Find a pos where non of the delimiters show up.
	for (; m_cur < cend()
		// continue skipping if delimiters presented.
		&& std::ranges::contains(delimiters, *m_cur); ++m_cur) {}

	if (bLeftTrim)
		SkipUntilNonspace();

	auto const pos1 = m_cur;

	// Find a pos after the pos1 that a delimiter shows up.
	for (; m_cur < cend()
		// continue skipping if non-delimiter shows up.
		&& !std::ranges::contains(delimiters, *m_cur); ++m_cur) {}

	if (bRightTrim)
	{
		reverse_iterator it{ m_cur };

		for (; it != crend() && IsSpace(*it); ++it) {}

		return string_view{ pos1, it.base() };
	}
	else
		// Construct by: itFirst, itLast
		return string_view{ pos1, m_cur };
}

std::string_view CBaseParser::Parse(uint32_t iCount) noexcept
{
	iCount = std::min((ptrdiff_t)iCount, cend() - m_cur);

	string_view ret{ m_cur, iCount };

	m_cur += iCount;

	return ret;
}

std::string_view CBaseParser::Peek(std::string_view delimiters, bool bLeftTrim, bool bRightTrim) const noexcept
{
	auto pos1 = m_cur;

	for (; pos1 < cend()
		&& (std::ranges::contains(delimiters, *pos1) || (bLeftTrim && IsSpace(*pos1)));
		++pos1);

	auto pos2 = pos1;

	for (; pos2 < cend()
		&& !std::ranges::contains(delimiters, *pos2);
		++pos2);

	if (bRightTrim)
	{
		reverse_iterator it{ pos2 };

		for (; it != crend() && IsSpace(*it); ++it);

		return string_view{ pos1, it.base() };
	}

	return string_view{ pos1, pos2 };
}

std::string_view CBaseParser::Peek(uint32_t iCount) const noexcept
{
	iCount = std::min((ptrdiff_t)iCount, cend() - m_cur);

	return string_view{ m_cur, iCount };
}

void CBaseParser::Seek(ptrdiff_t iOffset, int iMode /*= SEEK_CUR*/) noexcept
{
	switch (iMode)
	{
	case SEEK_SET:
		m_cur = m_p + iOffset;
		break;

	case SEEK_END:
		m_cur = cend() + iOffset;
		break;

	case SEEK_CUR:
		m_cur += iOffset;
		break;

	default:
		break;
	}
}

size_t CaseIgnoredString::operator()(std::string_view const& sz) const noexcept
{
	static std::hash<string> fnHash{};

	return fnHash(
		sz
		| std::views::transform(ToLower)
		| std::ranges::to<string>()
	);
}

size_t CaseIgnoredString::operator()(std::wstring_view const& sz) const noexcept
{
	static std::hash<wstring> fnHash{};

	return fnHash(
		sz
		| std::views::transform(ToWLower)
		| std::ranges::to<wstring>()
	);
}
