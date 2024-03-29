// StringBin.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdint.h>
#include <stdio.h>

#include <bit>
#include <codecvt>
#include <filesystem>
#include <ranges>
#include <span>
#include <string_view>

#include <fmt/color.h>

#include "StringBin.hpp"

namespace fs = std::filesystem;

using std::bit_cast;
using std::span;
using std::wstring_view;

void OutputStringBin(wstring_view wszFile) noexcept
{
	auto const InputPath = fs::path{ wszFile };
	auto const OutputPath = InputPath.parent_path() / InputPath.stem().stem();	// remove .string and .bin

	if (FILE *f = _wfopen(InputPath.c_str(), L"rb"), *fout = _wfopen(OutputPath.c_str(), L"w, ccs=UTF-16LE"); f && fout)
	{
		uint16_t iCounter{};

		fwrite(u"¬\n", sizeof(char16_t), 2, fout);	// Under "wt" mode(default), Windows interpret \n as CRLF eol. And consider \r\n as LF eol.

		for (auto &&[pszKey, iKeyLength, pszValue, iValueLength] : StringsBin::Deserialize(f))
		{
			[[unlikely]]
			if (!pszKey || !pszValue)
			{
				fmt::print(fg(fmt::color::red), "Unknown format on '{}'\nExpected signature: [2, 2048] but [{}, {}] detected.\n", InputPath.u8string(), iKeyLength, iValueLength);
				return;
			}

			fwrite(u"{", sizeof(char16_t), 1, fout);
			fwrite(pszKey, sizeof(char16_t), iKeyLength, fout);
			fwrite(u"}", sizeof(char16_t), 1, fout);
			fwrite(pszValue, sizeof(char16_t), iValueLength, fout);
			fwrite(u"\n", sizeof(char16_t), 1, fout);

			++iCounter;
		}

		fmt::print("Output file saved as '{}'\n{} translation key exported.\n", OutputPath.u8string(), iCounter);

		fclose(f);
		fclose(fout);
	}
	else
	{
		if (!f)
			fmt::print(fg(fmt::color::red), "Cannot open source file '{}'\n", InputPath.u8string());
		if (!fout)
			fmt::print(fg(fmt::color::red), "Cannot open output file '{}'\n", OutputPath.u8string());
	}
}

int32_t wmain(int32_t argc, wchar_t *argv[]) noexcept
{
	if (argc < 2)
	{
		fmt::print("Drag file onto me!\n");
		system("pause");
		return EXIT_SUCCESS;
	}

	for (auto &&wszFile :
		span{ argv, static_cast<uint32_t>(argc) }
		| std::views::drop(1)
		| std::views::transform([](auto &&ws) noexcept -> wstring_view { return (wstring_view)ws; })
		)
	{
		if (wszFile.ends_with(L".strings.bin"))
		{
			OutputStringBin(wszFile);
			fmt::print("\n");
		}
		else if (wszFile.ends_with(L".txt"))
		{
			auto const InputPath = fs::path{ wszFile };
			auto const OutputPath = fs::path{ InputPath.wstring() + L".strings.bin" };
		}
		else
			fmt::print(fg(fmt::color::red), "File must either be an .strings.bin or an .txt\n");
	}

	system("pause");
	return EXIT_SUCCESS;
}
