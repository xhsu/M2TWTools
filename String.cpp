#include "String.hpp"

#include <fmt/core.h>

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <wctype.h>

#include <algorithm>
#include <codecvt>
#include <ranges>

using namespace std;
using namespace std::experimental;
using namespace std::literals;

bool strieql(string_view lhs, string_view rhs) noexcept
{
	return std::ranges::equal(
		lhs, rhs,
		[](char lc, char rc) noexcept -> bool { return ToUpper(lc) == ToUpper(rc); }
	);
}

bool wcsieql(wstring_view lhs, wstring_view rhs) noexcept
{
	return std::ranges::equal(
		lhs, rhs,
		[](wchar_t lc, wchar_t rc) noexcept -> bool { return ToWLower(lc) == ToWLower(rc); }
	);
}

bool StartsWith_I(string_view text, string_view what) noexcept
{
	if (what.length() > text.length())
		return false;

	for (auto&& [lc, rc] : std::views::zip(text, what))
	{
		auto const lci = ToLower(lc);
		auto const rci = ToLower(rc);

		if (lci != rci)
			return false;
	}

	return true;
}

bool StartsWith_I(wstring_view text, wstring_view what) noexcept
{
	if (what.length() > text.length())
		return false;

	for (auto&& [lc, rc] : std::views::zip(text, what))
	{
		auto const lci = ToWLower(lc);
		auto const rci = ToWLower(rc);

		if (lci != rci)
			return false;
	}

	return true;
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

void UTIL_Replace(string* str, string_view const from, string_view const to) noexcept
{
	size_t start_pos = 0;
	while ((start_pos = str->find(from, start_pos)) != str->npos)
	{
		str->replace(start_pos, from.length(), to);
		start_pos += to.length();	// In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

std::string ToUTF8(std::wstring_view wsz) noexcept
{
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

	try
	{
		//std::string narrow = converter.to_bytes(wide_utf16_source_string);
		//std::wstring wide = converter.from_bytes(narrow_utf8_source_string);
		return converter.to_bytes(wsz.data(), wsz.data() + wsz.length());
	}
	catch (const std::exception& e)
	{
		return "[Error] Cannot convert: "s + e.what();
	}
	catch (...)
	{
		return "[Error] Unspecified exception!";
	}
}

std::wstring ToUTF16(std::string_view sz) noexcept
{
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

	try
	{
		//std::string narrow = converter.to_bytes(wide_utf16_source_string);
		//std::wstring wide = converter.from_bytes(narrow_utf8_source_string);
		return converter.from_bytes(sz.data(), sz.data() + sz.length());
	}
	catch (const std::exception& e)
	{
		return L"[Error] Cannot convert: "s + converter.from_bytes(e.what());
	}
	catch (...)
	{
		return L"[Error] Unspecified exception!";
	}
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
	m_legit_size = m_length;
}

CBaseParser::~CBaseParser() noexcept
{
	if (m_p)
	{
		free(m_p);
		m_p = nullptr;
		m_cur = nullptr;
		m_length = 0;
		m_legit_size = 0;
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

void CBaseParser::StripComments() noexcept
{
	for (auto p = cbegin(); p < cend(); /* does nothing */)
	{
		if (*p == ';')
		{
			auto eol = p;
			for (; eol < cend() && *eol != '\r' && *eol != '\n'; ++eol) {}

			auto const len = eol - p;
			assert(len >= 0 && m_length >= (size_t)len);

			auto const tell = eol - cbegin();
			assert(tell >= 0 && m_length >= (size_t)tell);

			memmove(p, eol, m_length - tell);
			m_length -= len;
		}
		else
			++p;
	}

	// All previous pointers/iterators will be invalidated.
	Seek(0, SEEK_SET);
}

void CBaseParser::FilterComment(std::string_view* const psv) noexcept
{
	[[unlikely]]
	if (psv == nullptr)
		return;

	if (psv->front() == ';')
	{
		*psv = "";
		return;
	}

	if (auto const pos = psv->find_first_of(';'); pos != psv->npos)
	{
		*psv = psv->substr(0, pos);
		return;
	}
}

std::string_view CBaseParser::Parse(std::string_view delimiters /*= " \n\t\f\v\r"*/, bool bLeftTrim /*= true*/, bool bRightTrim /*= true*/) noexcept
{
	// Find a pos where non of the delimiters show up.
	for (; m_cur < cend()
		// continue skipping if delimiters presented.
		&& std::ranges::contains(delimiters, *m_cur); ++m_cur) {}

	if (bLeftTrim)
		SkipUntilNonspace();

	// Though there are some characters left, but all of which are illegal under current delimiters.
	if (Eof())
		return "";

	auto const pos1 = m_cur;

	// Find a pos after the pos1 that a delimiter shows up.
	for (; m_cur < cend()
		// continue skipping if non-delimiter shows up.
		&& !std::ranges::contains(delimiters, *m_cur); ++m_cur) {}

	if (bRightTrim)
	{
		reverse_iterator it{ m_cur }, lim{ pos1 };

		for (; it < lim && IsSpace(*it); ++it) {}

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

	// Though there are some characters left, but all of which are illegal under current delimiters.
	if (pos1 >= cend())
		return "";

	auto pos2 = pos1;

	for (; pos2 < cend()
		&& !std::ranges::contains(delimiters, *pos2);
		++pos2);

	if (bRightTrim)
	{
		reverse_iterator it{ pos2 }, lim{ pos1 };

		for (; it < lim && IsSpace(*it); ++it);

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

bool IBaseFile::Save(std::filesystem::path const& Path) const noexcept
{
	if (auto f = _wfopen(Path.c_str(), L"wt"); f)
	{
		fmt::print(f, "{}", Serialize());
		fclose(f);
		return true;
	}

	return false;
}

size_t CaseIgnoredHash::operator()(std::string_view const& sz) const noexcept
{
	static std::hash<string> fnHash{};

	return fnHash(
		sz
		| std::views::transform(ToLower)
		| std::ranges::to<string>()
	);
}

size_t CaseIgnoredHash::operator()(std::wstring_view const& sz) const noexcept
{
	static std::hash<wstring> fnHash{};

	return fnHash(
		sz
		| std::views::transform(ToWLower)
		| std::ranges::to<wstring>()
	);
}

bool CaseIgnoredLess::operator()(std::string_view const& lhs, std::string_view const& rhs) const noexcept
{
	for (auto&& [lc, rc] : std::views::zip(lhs, rhs))
	{
		auto const lci = ToLower(lc);
		auto const rci = ToLower(rc);

		if (lci == rci)
			continue;

		return lci < rci;
	}

	return lhs.length() < rhs.length();
}

bool CaseIgnoredLess::operator()(std::wstring_view const& lhs, std::wstring_view const& rhs) const noexcept
{
	for (auto&& [lc, rc] : std::views::zip(lhs, rhs))
	{
		auto const lci = ToWLower(lc);
		auto const rci = ToWLower(rc);

		if (lci == rci)
			continue;

		return lci < rci;
	}

	return lhs.length() < rhs.length();
}

template int32_t UTIL_StrToNum<int32_t>(std::string_view) noexcept;
template int16_t UTIL_StrToNum<int16_t>(std::string_view) noexcept;
template float UTIL_StrToNum<float>(std::string_view) noexcept;
