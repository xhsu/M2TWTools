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

bool wcsieql(std::wstring_view lhs, std::wstring_view rhs) noexcept
{
	return std::ranges::equal(
		lhs, rhs,
		[](wchar_t lc, wchar_t rc) noexcept -> bool { return towlower(lc) == towlower(rc); }
	);
}

generator<string_view> UTIL_Split(string_view sz, string_view delimiters) noexcept
{
	for (auto lastPos = sz.find_first_not_of(delimiters, 0), pos = sz.find_first_of(delimiters, lastPos);
		sz.npos != pos || sz.npos != lastPos;
		lastPos = sz.find_first_not_of(delimiters, pos), pos = sz.find_first_of(delimiters, lastPos)
		)
	{
		co_yield sz.substr(lastPos, pos - lastPos);
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
	for (; iCount > 0 && m_cur < (m_p + m_length); --iCount)
		++m_cur;
}

void CBaseParser::SkipUntilNonspace(void) noexcept
{
	for (; *m_cur != '\0' && m_cur < (m_p + m_length) && IsSpace(*m_cur); ++m_cur) {}
}

void CBaseParser::Rewind(int32_t iCount /*= 1*/) noexcept
{
	for (; iCount > 0 && m_cur > m_p; --iCount)
		--m_cur;
}

void CBaseParser::RewindUntilNonspace(void) noexcept
{
	for (; *m_cur != '\0' && m_cur > m_p && IsSpace(*m_cur); --m_cur) {}
}

std::string_view CBaseParser::Parse(std::string_view delimiters /*= " \n\t\f\v\r"*/, bool bLeftTrim /*= true*/, bool bRightTrim /*= true*/) noexcept
{
	// Find a pos where non of the delimiters show up.
	for (; m_cur < (m_p + m_length)
		&& !std::ranges::contains(delimiters, *m_cur); ++m_cur) {}

	if (bLeftTrim)
		SkipUntilNonspace();

	auto const pos1 = m_cur;

	// Find a pos after the pos1 that a delimiter shows up.
	for (; m_cur < (m_p + m_length)
		&& std::ranges::contains(delimiters, *m_cur); ++m_cur) {}

	auto pos2 = m_cur;
	for (; pos1 <= pos2 && IsSpace(*pos2); --pos2) {}

	// Construct by: itFirst, itLast
	return string_view{ pos1, pos2 };
}

std::string_view CBaseParser::ReadN(uint32_t iCount) noexcept
{
	iCount = std::min((ptrdiff_t)iCount, m_p + m_length - m_cur);

	string_view const ret{ m_cur, iCount };

	m_cur += iCount;

	return ret;
}

void CBaseParser::Seek(ptrdiff_t iOffset, int iMode /*= SEEK_CUR*/) noexcept
{
	switch (iMode)
	{
	case SEEK_SET:
		m_cur = m_p + iOffset;
		break;

	case SEEK_END:
		m_cur = m_p + m_length + iOffset;
		break;

	case SEEK_CUR:
		m_cur += iOffset;
		break;

	default:
		break;
	}
}
