#pragma once

#include "String.hpp"

namespace StringsBin
{
	namespace fs = std::filesystem;

	using std::experimental::generator;
	using std::tuple;

	generator<tuple<char16_t const*, size_t, char16_t const*, size_t>> Deserialize(FILE* f) noexcept;
	void Serialize(fs::path const& out, std::ranges::input_range auto&& rgszTranslations) noexcept
	{
		static_assert(
			requires
			{
				{ std::get<0>(*std::begin(rgszTranslations)) } -> std::convertible_to<std::string_view>;
				{ std::get<1>(*std::begin(rgszTranslations)) } -> std::convertible_to<std::string_view>;
			},
			"Requires the range to be tuple-binded into two string_view!"
		);

		if (auto f = _wfopen(out.c_str(), L"w, ccs=UTF-16LE"); f)
		{
			fwrite(u"¬\n", sizeof(char16_t), 2, f);	// Under "wt" mode(default), Windows interpret \n as CRLF eol. And consider \r\n as LF eol.

			for (auto&& [szKey, szValue] : rgszTranslations)
			{
				auto const wcsKey = ToUTF16(szKey);
				auto const wcsValue = ToUTF16(szValue);

				fwrite(u"{", sizeof(char16_t), 1, f);
				fwrite(wcsKey.c_str(), sizeof(char16_t), wcsKey.length(), f);
				fwrite(u"}", sizeof(char16_t), 1, f);
				fwrite(wcsValue.c_str(), sizeof(char16_t), wcsValue.length(), f);
				fwrite(u"\n", sizeof(char16_t), 1, f);
			}

			fclose(f);
		}
	}
}
