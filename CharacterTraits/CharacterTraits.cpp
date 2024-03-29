﻿// CharacterTraits.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <codecvt>
#include <deque>
#include <functional>
#include <iostream>	// std::cin
#include <list>
#include <ranges>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

#include <cassert>
#include <cstdio>

#include <fmt/color.h>

#include <experimental/generator>

using std::array;
using std::bit_cast;
using std::deque;
using std::function;
using std::list;
using std::string;
using std::string_view;
using std::tuple;
using std::unordered_map;
using std::variant;
using std::vector;

using std::experimental::generator;

using namespace std::literals;

import CharacterTraits;

enum class SearchMode_e : std::uint8_t
{
	NAME = (1 << 0),
	CHAR_TYPE = (1 << 1),
	EXCL_CULTURE = (1 << 2),
};

deque<Trait_t> g_Traits;
Trait_t* g_pCurTrait = nullptr;
Level_t* g_pCurLevel = nullptr;
list<string> g_rgszUnknownTokens;
unordered_map<string, string> g_rgszDic;

deque<Trigger_t> g_Triggers;
Trigger_t* g_pCurTrigger = nullptr;

bool g_bApplicationRunning = true;
string g_szCurCommand = "";
vector<function<bool(const Trait_t& Trait)>> g_rgfnFilters;
vector<string> g_rgszInfoStrings;

generator<string_view> LineByLine(const char* const p) noexcept
{
	for (const char* pCurPos = p, *pBegin = p; pCurPos != nullptr && *pCurPos != '\0'; /* Does nothing */)
	{
		if (*pCurPos == '\n')
		{
			//auto save = pCurPos;
			for (; std::isspace(*(pCurPos - 1)) && pCurPos > p; --pCurPos) {}	// R Trim

			co_yield string_view(pBegin, pCurPos);

			//pCurPos = save;
			for (pBegin = pCurPos + 1; std::isspace(*pBegin); ++pBegin) {}	// L Trim

			pCurPos = pBegin + 1;	// Starting from Next non-space.
		}
		else
			++pCurPos;
	}

	co_return;
}

generator<string_view> Split(const string_view& s, const char* delimiters = ", ") noexcept
{
	for (auto lastPos = s.find_first_not_of(delimiters, 0), pos = s.find_first_of(delimiters, lastPos);
		s.npos != pos || s.npos != lastPos;
		lastPos = s.find_first_not_of(delimiters, pos), pos = s.find_first_of(delimiters, lastPos)
		)
	{
		co_yield s.substr(lastPos, pos - lastPos);
	}

	co_return;
}

generator<vector<string_view>> ConsoleCommand(void) noexcept
{
	while (g_bApplicationRunning)
	{
		g_szCurCommand.push_back(std::cin.get());

		if (g_szCurCommand.back() == '\n')
		{
			co_yield Split(g_szCurCommand, ", \n\r\t") | std::ranges::to<vector>();

			g_szCurCommand.clear();
		}
	}
}

auto ToUpper(char c) noexcept
{
	if (c <= 'z' && c >= 'a')
		return static_cast<char>(c + ('A' - 'a'));

	return c;
}

int main(int argc, char* argv[]) noexcept
{
	// Basicallly ask you to compile this code on windows...
	static_assert(sizeof(char16_t) == sizeof(wchar_t), "Unsupported compiling environment.");

	char const* pTraitFileName = "export_descr_character_traits.txt";
	char const* pTraitTranslationFileName = "export_vnvs.txt.strings.bin";

	if (argc > 2)
		pTraitFileName = argv[1];
	if (argc > 3)
		pTraitTranslationFileName = argv[2];

	string szTraitTranslationFileOutput = pTraitTranslationFileName;
	if (auto const iPos = szTraitTranslationFileOutput.find(".strings.bin"); iPos != string::npos)
		szTraitTranslationFileOutput.erase(iPos);
	else
	{
		fmt::print("Invalid .strings.bin localization file input: {}.\n", szTraitTranslationFileOutput);
		return EXIT_FAILURE;
	}

	if (FILE* f = std::fopen(pTraitFileName, "rb"); f != nullptr)
	{
		fseek(f, 0, SEEK_END);

		auto const fsize = ftell(f);
		char* p = (char*)std::calloc(fsize + 1, sizeof(char));

		fseek(f, 0, SEEK_SET);
		fread(p, sizeof(char), fsize, f);

		for (auto&& sz : LineByLine(p))
		{
			if (sz[0] == ';')
				continue;

			if (sz.starts_with("Trait "))
			{
				g_pCurTrait = &g_Traits.emplace_back();
				g_pCurTrait->m_Name = string(sz, (size_t)6, sz.length() - 6);
			}
			else if (sz.starts_with("Characters "))
			{
				auto cs = Split(sz.substr(11));
				g_pCurTrait->m_Character = decltype(g_pCurTrait->m_Character)(cs.begin(), cs.end());
			}
			else if (sz.starts_with("ExcludeCultures "))
			{
				auto ecs = Split(sz.substr(16));
				g_pCurTrait->m_ExcludeCultures = decltype(g_pCurTrait->m_ExcludeCultures)(ecs.begin(), ecs.end());
			}
			else if (sz.starts_with("NoGoingBackLevel "))
			{
				try
				{
					g_pCurTrait->m_NoGoingBackLevel = (decltype(g_pCurTrait->m_NoGoingBackLevel))std::stoi(string(sz, size_t(17), sz.length() - 17));
				}
				catch (...)
				{
					g_pCurTrait->m_NoGoingBackLevel = -1;
				}
			}
			else if (sz.starts_with("AntiTraits "))
			{
				for (auto&& TraitStr : Split(sz.substr(11)))
					g_pCurTrait->m_AntiTraits.emplace_back(string(TraitStr));
			}
			else if (sz.starts_with("Hidden"))
			{
				g_pCurTrait->m_Hidden = true;
			}

			else if (sz.starts_with("Level "))
			{
				g_pCurLevel = &g_pCurTrait->m_Levels.emplace_back();
				g_pCurLevel->m_Name = string(sz, (size_t)6, sz.length() - 6);
				g_pCurLevel->m_Level = (decltype(g_pCurLevel->m_Level))g_pCurTrait->m_Levels.size();
			}
			else if (sz.starts_with("Description "))
			{
				g_pCurLevel->m_Description = string(sz, size_t(12), sz.length() - 12);
			}
			else if (sz.starts_with("EffectsDescription "))
			{
				g_pCurLevel->m_EffectsDescription = string(sz, size_t(19), sz.length() - 19);
			}
			else if (sz.starts_with("GainMessage "))
			{
				g_pCurLevel->m_GainMessage = string(sz, size_t(12), sz.length() - 12);
			}
			else if (sz.starts_with("LoseMessage "))
			{
				g_pCurLevel->m_LoseMessage = string(sz, size_t(12), sz.length() - 12);
			}
			else if (sz.starts_with("Epithet "))
			{
				g_pCurLevel->m_Epithet = string(sz, size_t(8), sz.length() - 8);
			}
			else if (sz.starts_with("Threshold "))
			{
				try
				{
					g_pCurLevel->m_Threshold = (decltype(g_pCurLevel->m_Threshold))std::stoi(string(sz, size_t(10), sz.length() - 10));
				}
				catch (...)
				{
					g_pCurLevel->m_Threshold = 0;
				}
			}
			else if (sz.starts_with("Effect "))
			{
				auto es = Split(sz.substr(7));
				vector<string_view> rgsz(es.begin(), es.end());

				if (rgsz.size() != 2) [[unlikely]]
					fmt::print(fg(fmt::color::dark_red), "ERROR: 2 parameters expected, but {} parameter(s) received.\n", rgsz.size());

				g_pCurLevel->m_Effects.emplace_back(string(rgsz[0]), std::stoi(string(rgsz[1])));
			}

			else if (sz.starts_with("Trigger "))
			{
				g_pCurTrigger = &g_Triggers.emplace_back();
				g_pCurTrigger->m_Name = string(sz, (size_t)8, sz.length() - 8);
			}
			else if (sz.starts_with("WhenToTest "))
			{
				g_pCurTrigger->m_WhenToTest = string(sz, (size_t)11, sz.length() - 11);
			}
			else if (sz.starts_with("Condition ") || sz.starts_with("and "))
			{
				g_pCurTrigger->m_Conditions.emplace_back(string(sz.substr(sz.find_first_of(' ') + 1)));
			}
			else if (sz.starts_with("Affects "))
			{
				auto future = Split(sz.substr(8));
				vector<string_view> rgsz(future.begin(), future.end());

				if (rgsz.size() != 4)
				{
					fmt::print(fg(fmt::color::red), "wrong arg count: {}", sz);
					continue;
				}

				Affect_t& Affect = g_pCurTrigger->m_Affects.emplace_back();
				Affect.m_Trait = string(rgsz[0]);
				std::from_chars(rgsz[1].data(), rgsz[1].data() + rgsz[1].length(), Affect.m_Level);
				std::from_chars(rgsz[3].data(), rgsz[3].data() + rgsz[3].length(), Affect.m_Chance);
			}

			else
			{
				if (sz.contains(' '))
				{
					auto const rgsz = Split(sz) | std::ranges::to<vector>();

					g_rgszUnknownTokens.emplace_back(rgsz[0]);
				}
				else
				{
					g_rgszUnknownTokens.emplace_back(sz);
				}
			}
		}

		free(p);
		fclose(f);

		fmt::print("Successfully loading {} traits.\n", g_Traits.size());
	}

	if (FILE* f = std::fopen(pTraitTranslationFileName, "rb"), *fout = std::fopen(szTraitTranslationFileOutput.c_str(), "w, ccs=UTF-16LE"); f && fout)
	{
		fseek(f, 0, SEEK_SET);

		std::int16_t iStyle1 = 0, iStyle2 = 0;
		fread(&iStyle1, sizeof(iStyle1), 1, f);
		fread(&iStyle2, sizeof(iStyle2), 1, f);

		assert(iStyle1 == 2 && iStyle2 == 2048);

		std::int32_t iCount = 0;
		fread(&iCount, sizeof(iCount), 1, f);

		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> Convert;	// Fuck C++17

		// Seems like M2TW requires this line to identify txt files.
		fwrite(u"¬\n", sizeof(char16_t), 2, fout);	// Under "wt" mode(default), Windows interpret \n as CRLF eol. And consider \r\n as LF eol.

		std::int16_t iStrLen = 0;
		char16_t* pszKey = nullptr, * pszValue = nullptr;

		for (std::int32_t i = 0; i < iCount; ++i)
		{
			fread(&iStrLen, sizeof(iStrLen), 1, f);

			pszKey = (char16_t*)realloc(pszKey, (iStrLen + 1) * sizeof(char16_t));
			pszKey[iStrLen] = u'\0';
			fread(pszKey, sizeof(char16_t), iStrLen, f);
			fwrite(u"{", sizeof(char16_t), 1, fout);
			fwrite(pszKey, sizeof(char16_t), iStrLen, fout);
			fwrite(u"}", sizeof(char16_t), 1, fout);

			iStrLen = 0;
			fread(&iStrLen, sizeof(iStrLen), 1, f);

			pszValue = (char16_t*)realloc(pszValue, (iStrLen + 1) * sizeof(char16_t));
			pszValue[iStrLen] = u'\0';
			fread(pszValue, sizeof(char16_t), iStrLen, f);

			for (auto&& c : std::ranges::subrange(pszValue, pszValue + iStrLen + 1))
				if (bit_cast<std::uint16_t>(c) == (std::uint16_t)10u)
					c = u'\n';

			fwrite(pszValue, sizeof(char16_t), iStrLen, fout);
			fwrite(u"\n", sizeof(char16_t), 1, fout);

			g_rgszDic.try_emplace(Convert.to_bytes(pszKey), Convert.to_bytes(pszValue));
		}

		free(pszKey); pszKey = nullptr;
		free(pszValue); pszValue = nullptr;

		std::fclose(f);
		fmt::print("Successfully loading {} translation entries.\n", g_rgszDic.size());

		std::fclose(fout);
		fmt::print("Successfully saved translation file as {}.\n", "export_vnvs.txt");
	}

	// Link up the anti-trait.
	for (auto&& Trait : g_Traits)
	{
		for (auto&& AntiTrait : Trait.m_AntiTraits)
		{
			if (AntiTrait.index() == 1)
				continue;

			if (auto iter = std::find_if(g_Traits.cbegin(), g_Traits.cend(), [AntiTrait](const Trait_t& elem) -> bool { return elem.m_Name == std::get<string>(AntiTrait); }); iter != g_Traits.cend())
				AntiTrait = &*iter;
			else
				fmt::print(fg(fmt::color::dark_golden_rod) | fmt::emphasis::italic, "Anti-trait \"{}\" of \"{}\" no found.\n", std::get<string>(AntiTrait), Trait.m_Name);
		}
	}

	// Link up the texts.
	static auto const fnTranslate = [](string* psz) {try { *psz = g_rgszDic.at(*psz); } catch (...) {}};
	for (auto&& Trait : g_Traits)
	{
		for (auto&& Level : Trait.m_Levels)
		{
			fnTranslate(&Level.m_Name);
			fnTranslate(&Level.m_Description);
			fnTranslate(&Level.m_EffectsDescription);
			fnTranslate(&Level.m_Epithet);
			fnTranslate(&Level.m_GainMessage);
			fnTranslate(&Level.m_LoseMessage);
		}

		fnTranslate(&Trait.m_Name);
	}

#pragma region Handle unparsed tokens
	g_rgszUnknownTokens.sort();
	g_rgszUnknownTokens.unique();

	for (const auto& Token : g_rgszUnknownTokens)
		fmt::print(fg(fmt::color::dark_golden_rod), "Unparsed token: {}\n", Token);
#pragma endregion Handle unparsed tokens

	for (auto&& cmdarg : ConsoleCommand())
	{
		if (cmdarg.empty())
			continue;
		else if (cmdarg[0] == "exit")
		{
			g_bApplicationRunning = false;
			return EXIT_SUCCESS;
		}

		else if (cmdarg[0] == "name")
		{
			static auto const fnTrName = [](const string& szName, const Trait_t& Trait) noexcept -> bool { return Trait.m_Name.contains(szName); };
			static auto const fnTrNameOr = [](const auto& rgszNames, const Trait_t& Trait) noexcept -> bool
			{
				for (auto&& Name : rgszNames)
				{
					if (Trait.m_Name.contains(Name))
						return true;
				}

				return false;
			};
			static auto const fnLvName = [](const string& szName, const Trait_t& Trait) noexcept -> bool
			{
				for (auto&& Level : Trait.m_Levels)
				{
					if (Level.m_Name.contains(szName))
						return true;
				}

				return false;
			};
			static auto const fnLvNameOr = [](const auto& rgszNames, const Trait_t& Trait) noexcept -> bool
			{

				for (auto&& Level : Trait.m_Levels)
				{
					for (auto&& Name : rgszNames)
						if (Level.m_Name.contains(Name))
							return true;
				}

				return false;
			};
			static auto const fnName = [](const string& szName, const Trait_t& Trait) noexcept -> bool { return fnTrName(szName, Trait) || fnLvName(szName, Trait); };
			static auto const fnNameOr = [](const auto& rgszNames, const Trait_t& Trait) noexcept -> bool { return fnTrNameOr(rgszNames, Trait) || fnLvNameOr(rgszNames, Trait); };

			if (cmdarg.size() < 2)
			{
				fmt::print(fg(fmt::color::red), "Command \"name\": expecting at least 1 argument, but {} received.\n", (int)cmdarg.size() - 1);
				continue;
			}
			else if (cmdarg[1] == "tr")
			{
				if (cmdarg.size() < 3)
				{
					fmt::print(fg(fmt::color::red), "Command \"name tr\": expecting at least 1 more argument, but total {} received.\n", (int)cmdarg.size() - 1);
					continue;
				}
				else if (cmdarg.size() == 3)
				{
					g_rgfnFilters.emplace_back(std::bind_front(fnTrName, string(cmdarg[2])));
					g_rgszInfoStrings.emplace_back(fmt::format("Must contains: \"{}\" in trait name", cmdarg[2]));
				}
				else
				{
					g_rgfnFilters.emplace_back(std::bind_front(fnTrNameOr, vector<string>(cmdarg.begin() + 2, cmdarg.end())));
					g_rgszInfoStrings.emplace_back(fmt::format("Must contains one of these strings in its trait name: \"{}\"", fmt::join(cmdarg | std::views::drop(2), "\", \"")));
				}
			}
			else if (cmdarg[1] == "lv")
			{
				if (cmdarg.size() < 3)
				{
					fmt::print(fg(fmt::color::red), "Command \"name lv\": expecting at least 1 more argument, but total {} received.\n", (int)cmdarg.size() - 1);
					continue;
				}
				else if (cmdarg.size() == 3)
				{
					g_rgfnFilters.emplace_back(std::bind_front(fnLvName, string(cmdarg[2])));
					g_rgszInfoStrings.emplace_back(fmt::format("Must contains: \"{}\" in any of level name", cmdarg[2]));
				}
				else
				{
					g_rgfnFilters.emplace_back(std::bind_front(fnLvNameOr, vector<string>(cmdarg.begin() + 2, cmdarg.end())));
					g_rgszInfoStrings.emplace_back(fmt::format("Must contains one of these strings in its level name: \"{}\"", fmt::join(cmdarg | std::views::drop(2), "\", \"")));
				}
			}
			else if (cmdarg.size() == 2)
			{
				g_rgfnFilters.emplace_back(std::bind_front(fnName, string(cmdarg[1])));
				g_rgszInfoStrings.emplace_back(fmt::format("Must contains: \"{}\" in either trait name or level names.", cmdarg[1]));
			}
			else if (cmdarg.size() > 2)
			{
				g_rgfnFilters.emplace_back(std::bind_front(fnNameOr, vector<string>(cmdarg.begin() + 1, cmdarg.end())));
				g_rgszInfoStrings.emplace_back(fmt::format("Must contains one of these strings in either trait name or level names: \"{}\"", fmt::join(cmdarg | std::views::drop(1), "\", \"")));
			}
			else [[unlikely]]
				std::unreachable();
		}
		else if (cmdarg[0] == "prop")
		{
			static auto const fnLesser = [](string szProperty, short iModifer, const Trait_t& Trait) noexcept -> bool
			{
				for (auto&& Level : Trait.m_Levels)
				{
					for (auto&& Effect : Level.m_Effects)
					{
						if (!_strnicmp(szProperty.c_str(), Effect.m_Type.c_str(), szProperty.length())
							&& Effect.m_Modification < iModifer)
						{
							return true;
						}
					}
				}

				return false;
			};
			static auto const fnLesserOrEqual = [](string szProperty, short iModifer, const Trait_t& Trait) noexcept -> bool
			{
				for (auto&& Level : Trait.m_Levels)
				{
					for (auto&& Effect : Level.m_Effects)
					{
						if (!_strnicmp(szProperty.c_str(), Effect.m_Type.c_str(), szProperty.length())
							&& Effect.m_Modification <= iModifer)
						{
							return true;
						}
					}
				}

				return false;
			};
			static auto const fnEqual = [](string szProperty, short iModifer, const Trait_t& Trait) noexcept -> bool
			{
				for (auto&& Level : Trait.m_Levels)
				{
					for (auto&& Effect : Level.m_Effects)
					{
						if (!_strnicmp(szProperty.c_str(), Effect.m_Type.c_str(), szProperty.length())
							&& Effect.m_Modification == iModifer)
						{
							return true;
						}
					}
				}

				return false;
			};
			static auto const fnGreaterOrEqual = [](string szProperty, short iModifer, const Trait_t& Trait) noexcept -> bool
			{
				for (auto&& Level : Trait.m_Levels)
				{
					for (auto&& Effect : Level.m_Effects)
					{
						if (!_strnicmp(szProperty.c_str(), Effect.m_Type.c_str(), szProperty.length())
							&& Effect.m_Modification >= iModifer)
						{
							return true;
						}
					}
				}

				return false;
			};
			static auto const fnGreater = [](string szProperty, short iModifer, const Trait_t& Trait) noexcept -> bool
			{
				for (auto&& Level : Trait.m_Levels)
				{
					for (auto&& Effect : Level.m_Effects)
					{
						if (!_strnicmp(szProperty.c_str(), Effect.m_Type.c_str(), szProperty.length())
							&& Effect.m_Modification > iModifer)
						{
							return true;
						}
					}
				}

				return false;
			};

			if (cmdarg.size() != 4)
			{
				fmt::print(fg(fmt::color::red), "Command \"prop\" must have exact 4 arguments.\n");
				continue;
			}

			short iCompare = 0;
			std::from_chars(cmdarg[3].data(), cmdarg[3].data() + cmdarg[3].length(), iCompare);

			if (cmdarg[2] == "<")
			{
				g_rgfnFilters.emplace_back(std::bind_front(fnLesser, string(cmdarg[1]), iCompare));
				g_rgszInfoStrings.emplace_back(fmt::format("Containing property(ies) that have effect with string \"{}\" and lesser than {}.", cmdarg[1], iCompare));
			}
			else if (cmdarg[2] == "<=")
			{
				g_rgfnFilters.emplace_back(std::bind_front(fnLesserOrEqual, string(cmdarg[1]), iCompare));
				g_rgszInfoStrings.emplace_back(fmt::format("Containing property(ies) that have effect with string \"{}\" and lesser than or equals to {}.", cmdarg[1], iCompare));
			}
			else if (cmdarg[2] == "==")
			{
				g_rgfnFilters.emplace_back(std::bind_front(fnEqual, string(cmdarg[1]), iCompare));
				g_rgszInfoStrings.emplace_back(fmt::format("Containing property(ies) that have effect with string \"{}\" and equals to {}.", cmdarg[1], iCompare));
			}
			else if (cmdarg[2] == ">=")
			{
				g_rgfnFilters.emplace_back(std::bind_front(fnGreaterOrEqual, string(cmdarg[1]), iCompare));
				g_rgszInfoStrings.emplace_back(fmt::format("Containing property(ies) that have effect with string \"{}\" and greater than or equals to {}.", cmdarg[1], iCompare));
			}
			else if (cmdarg[2] == ">")
			{
				g_rgfnFilters.emplace_back(std::bind_front(fnGreater, string(cmdarg[1]), iCompare));
				g_rgszInfoStrings.emplace_back(fmt::format("Containing property(ies) that have effect with string \"{}\" and greater than {}.", cmdarg[1], iCompare));
			}
			else
			{
				fmt::print(fg(fmt::color::red), "Operator '{}' does not supported.\n", cmdarg[2]);

				if (cmdarg[2] == "=")
					fmt::print(fg(fmt::color::red), "Use '==' as equality operator instead.\n");
				else if (cmdarg[2] == "<>" || cmdarg[2] == "!=")
					fmt::print(fg(fmt::color::red), "Operator 'not equal' does not make sense here.\n");
				else if (cmdarg[2] == "<=>")
					fmt::print(fg(fmt::color::red), "Spaceship operator? Seriously? What's wrong with you?\n");

				continue;
			}

			//g_rgfnFilters.emplace_back(fn);
			//g_rgszInfoStrings.emplace_back("Must contains only positive effect on all level(s).");
		}
		else if (cmdarg[0] == "epithet")
		{
			static auto const fnHasEpithet = [](const Trait_t& Trait) noexcept -> bool
			{
				for (auto&& Level : Trait.m_Levels)
				{
					if (!Level.m_Epithet.empty())
						return true;
				}

				return false;
			};
			static auto const fnHasNoEpithet = [](const Trait_t& Trait) noexcept -> bool
			{
				for (auto&& Level : Trait.m_Levels)
				{
					if (!Level.m_Epithet.empty())
						return false;
				}

				return true;
			};
			static auto const fnEpithetIs = [](const Trait_t &Trait, string szWhat) noexcept -> bool
			{
				for (auto &&Level : Trait.m_Levels)
				{
					if (Level.m_Epithet.contains(szWhat))
						return true;
				}

				return false;
			};

			if (cmdarg.size() == 1 || !_strnicmp(cmdarg[1].data(), "true", cmdarg[1].length()))
			{
				g_rgfnFilters.emplace_back(fnHasEpithet);
				g_rgszInfoStrings.emplace_back("Mush have epithet associated with.");
			}
			else if (!_strnicmp(cmdarg[1].data(), "false", cmdarg[1].length()))
			{
				g_rgfnFilters.emplace_back(fnHasNoEpithet);
				g_rgszInfoStrings.emplace_back("Mush have no epithet associated with.");
			}
			else if (cmdarg.size() == 2)
			{
				g_rgfnFilters.emplace_back(std::bind_back(fnEpithetIs, string(cmdarg[1])));
				g_rgszInfoStrings.emplace_back(fmt::format("Mush have an epithet containing \"{}\"", cmdarg[1]));
			}
			else
			{
				fmt::print(fg(fmt::color::red), "Command \"epithet\" have an optional argument ['true' | 'false' | 'String to search'].\n");
				continue;
			}
		}
		else if (cmdarg[0] == "eval")
		{
			static constexpr array rgszScoreNames{ "NEGATIVE"sv, "LEAN_NEGATIVE"sv, "NEUTER"sv, "LEAN_POSITIVE"sv, "POSITIVE"sv, };

			static auto const fnEvalLesser = [](Trait_t const &Trait, std::uint8_t iScore) noexcept -> bool
			{
				for (auto&& Level : Trait.m_Levels)
				{
					if (Level.Evaluate() < iScore)
						return true;
				}

				return false;
			};
			static auto const fnEvalLesserOrEqual = [](Trait_t const &Trait, std::uint8_t iScore) noexcept -> bool
			{
				for (auto&& Level : Trait.m_Levels)
				{
					if (Level.Evaluate() <= iScore)
						return true;
				}

				return false;
			};
			static auto const fnEvalEqual = [](Trait_t const &Trait, std::uint8_t iScore) noexcept -> bool
			{
				for (auto &&Level : Trait.m_Levels)
				{
					if (Level.Evaluate() == iScore)
						return true;
				}

				return false;
			};
			static auto const fnEvalGreaterOrEqual = [](Trait_t const &Trait, std::uint8_t iScore) noexcept -> bool
			{
				for (auto&& Level : Trait.m_Levels)
				{
					if (Level.Evaluate() >= iScore)
						return true;
				}

				return false;
			};
			static auto const fnEvalGreater = [](Trait_t const &Trait, std::uint8_t iScore) noexcept -> bool
			{
				for (auto&& Level : Trait.m_Levels)
				{
					if (Level.Evaluate() > iScore)
						return true;
				}

				return false;
			};

			if (cmdarg.size() != 3)
			{
				fmt::print(fg(fmt::color::red), "Command \"eval\" must have a comparision sign and a value as arguments.\n");
				continue;
			}
			else
			{
				std::uint8_t iScore{};

				for (; iScore < 5; ++iScore)
				{
					if (std::ranges::starts_with(rgszScoreNames[iScore], cmdarg[2], {}, {}, ToUpper))
						break;
				}

				if (iScore >= 5)
				{
					fmt::print(fg(fmt::color::red), "The value must be one of ['{}']\n", fmt::join(rgszScoreNames, "', '"));
					continue;
				}

				if (cmdarg[1] == "<")
				{
					g_rgfnFilters.emplace_back(std::bind_back(fnEvalLesser, iScore));
					g_rgszInfoStrings.emplace_back(fmt::format("Any level-wise evaluation result worse than {}.", rgszScoreNames[iScore]));
				}
				else if (cmdarg[1] == "<=")
				{
					g_rgfnFilters.emplace_back(std::bind_back(fnEvalLesserOrEqual, iScore));
					g_rgszInfoStrings.emplace_back(fmt::format("Any level-wise evaluation result worse than or equal to {}.", rgszScoreNames[iScore]));
				}
				else if (cmdarg[1] == "==")
				{
					g_rgfnFilters.emplace_back(std::bind_back(fnEvalEqual, iScore));
					g_rgszInfoStrings.emplace_back(fmt::format("Any level-wise evaluation result equal to {}.", rgszScoreNames[iScore]));
				}
				else if (cmdarg[1] == ">=")
				{
					g_rgfnFilters.emplace_back(std::bind_back(fnEvalGreaterOrEqual, iScore));
					g_rgszInfoStrings.emplace_back(fmt::format("Any level-wise evaluation result better than or equal to {}.", rgszScoreNames[iScore]));
				}
				else if (cmdarg[1] == ">")
				{
					g_rgfnFilters.emplace_back(std::bind_back(fnEvalGreater, iScore));
					g_rgszInfoStrings.emplace_back(fmt::format("Any level-wise evaluation result better than {}.", rgszScoreNames[iScore]));
				}
				else
				{
					fmt::print(fg(fmt::color::red), "Operator '{}' does not supported.\n", cmdarg[1]);

					if (cmdarg[1] == "=")
						fmt::print(fg(fmt::color::red), "Use '==' as equality operator instead.\n");
					else if (cmdarg[1] == "<>" || cmdarg[1] == "!=")
						fmt::print(fg(fmt::color::red), "Operator 'not equal' does not make sense here.\n");
					else if (cmdarg[1] == "<=>")
						fmt::print(fg(fmt::color::red), "Spaceship operator? Seriously? What's wrong with you?\n");

					continue;
				}
			}
		}
		else if (cmdarg[0] == "char")
		{
			static constexpr array rgszCharacterType{ "admiral"sv, "all"sv, "assassin"sv, "diplomat"sv, "family"sv, "heretic"sv, "inquisitor"sv, "merchant"sv, "priest"sv, "princess"sv, "spy"sv, "witch"sv };
			static const auto fnChType = [](Trait_t const &Trait, string szType) noexcept -> bool { return std::ranges::contains(Trait.m_Character, szType); };	// warpper for the sake of the lifetime of 'string' arg.

			if (cmdarg.size() != 2)
			{
				fmt::print(fg(fmt::color::red), "Command \"char\" must use with an argument indicating character type.\n");
				continue;
			}

			bool bFind{};
			string szSnapped{ cmdarg[1] };
			for (auto &&s : rgszCharacterType)
			{
				if (std::ranges::contains_subrange(s, szSnapped, {}, ToUpper, ToUpper))
				{
					szSnapped = (string)s;
					bFind = true;
					break;
				}
			}

			if (!bFind)
				fmt::print(fg(fmt::color::gold), "Warning: The character type you are inputting could be wrong.\n - Expecting: [{}]\n", fmt::join(rgszCharacterType, ", "));

			g_rgfnFilters.emplace_back(std::bind_back(fnChType, szSnapped));
			g_rgszInfoStrings.emplace_back(fmt::format("Must be compatible with '{}' character type", szSnapped));
		}
		else if (cmdarg[0] == "hidden")
		{
			static auto const fnHidden = [](Trait_t const &Trait, bool bTest) noexcept -> bool { return Trait.m_Hidden == bTest; };

			if (cmdarg.size() == 1 || !_strnicmp(cmdarg[1].data(), "true", cmdarg[1].length()))
			{
				g_rgfnFilters.emplace_back(std::bind_back(fnHidden, true));
				g_rgszInfoStrings.emplace_back("Mush be an hidden trait.");
			}
			else if (!_strnicmp(cmdarg[1].data(), "false", cmdarg[1].length()))
			{
				g_rgfnFilters.emplace_back(std::bind_back(fnHidden, false));
				g_rgszInfoStrings.emplace_back("Mush not be an hidden trait.");
			}
			else
			{
				fmt::print(fg(fmt::color::red), "Command \"hidden\" have only one optional argument ['true' | 'false'].\n");
				continue;
			}
		}

		else if (cmdarg[0] == "pop")
		{
			if (g_rgfnFilters.empty())
				fmt::print(fg(fmt::color::red), "Illegal usage of command \"pop\": No filter to remove from stack.\n");
			else
			{
				fmt::print("Removed filter: {}\n", fmt::styled(g_rgszInfoStrings.back(), fg(fmt::color::dark_gray) | fmt::emphasis::strikethrough | fmt::emphasis::italic));

				g_rgfnFilters.pop_back();
				g_rgszInfoStrings.pop_back();
			}
		}
		else if (cmdarg[0] == "inspect")
		{
			system("cls");

			if (g_rgszInfoStrings.empty())
			{
				fmt::print("No activated filter.\n");
			}
			else
			{
				fmt::print("Current activated filter{}:\n", g_rgszInfoStrings.size() == 1 ? "" : "s");
				for (const auto &szInfo : g_rgszInfoStrings)
					fmt::print(fg(fmt::color::slate_gray), " - {}\n", szInfo);
			}
		}
		else if (cmdarg[0] == "list")
		{
			system("cls");
			fmt::print("Listing trait(s) under filter{}:\n", g_rgszInfoStrings.size() == 1 ? "" : "s");
			for (const auto& szInfo : g_rgszInfoStrings)
				fmt::print(fg(fmt::color::slate_gray), " - {}\n", szInfo);

			static const auto fnApplyAllFilters = [](const Trait_t& Trait) -> bool
			{
				for (auto&& fn : g_rgfnFilters)
					if (!fn(Trait))
						return false;

				return true;
			};

			for (auto&& Trait : g_Traits | std::views::filter(fnApplyAllFilters))
				Trait.Print();
		}
		else if (cmdarg[0] == "new")
		{
			g_rgfnFilters.clear();
			g_rgszInfoStrings.clear();
			system("cls");
		}
		else if (cmdarg[0] == "clear")
		{
			system("cls");
		}
		else
			fmt::print(fg(fmt::color::red), "Unknown command: \"{}\".\n", cmdarg[0]);
	}

	return EXIT_SUCCESS;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
