// CharacterTraits.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <deque>
#include <list>
#include <string>
#include <variant>
#include <vector>
#include <iostream>
#include <ranges>
#include <functional>
#include <unordered_set>
#include <charconv>

#include <cassert>
#include <cstdio>

#include <fmt/color.h>

#include <experimental/generator>

using std::deque;
using std::list;
using std::pair;
using std::string;
using std::string_view;
using std::variant;
using std::vector;
using std::function;
using std::unordered_set;

using std::experimental::generator;

using namespace std::string_literals;
using namespace std::string_view_literals;

import UtlWinConsole;

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
			auto SplitResult = Split(g_szCurCommand, ", \n\r\t");
			co_yield vector<string_view>(SplitResult.begin(), SplitResult.end());	// std::ranges::to #UPDATE_AT_CPP23

			g_szCurCommand.clear();
		}
	}
}

template<typename T, typename U>
bool Contains(const T& Range, const U& Search) noexcept
{
	if constexpr (requires(typename T::value_type t, U u) { t.contains(u); })
	{
		for (auto&& elem : Range)
		{
			if (elem.contains(Search))
				return true;
		}

		return false;
	}
	else if constexpr (requires(typename T::value_type t, typename U::value_type u) { t.contains(u); })
	{
		for (auto&& elem : Range)
		{
			for (auto&& elem2 : Search)
			{
				if (elem.contains(elem2))
					return true;
			}
		}

		return false;
	}
}

int main(int argc, char* argv[]) noexcept
{
	if (FILE* f = std::fopen("export_descr_character_traits.txt", "rb"); f != nullptr)
	{
		fseek(f, 0, SEEK_END);

		auto fsize = ftell(f);
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

			else
			{
				if (sz.contains(' '))
				{
					auto us = Split(sz);
					vector<string_view> rgsz(us.begin(), us.end());

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
	}

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

	//for (const auto& Trait : g_Traits)
	//	Trait.Print();

	//list<string> efx;
	//for (const auto& Trait : g_Traits)
	//{
	//	for (const auto& Level : Trait.m_Levels)
	//		for (const auto& Effect : Level.m_Effects)
	//			if (std::find(efx.cbegin(), efx.cend(), Effect.first) == efx.cend())
	//				efx.emplace_back(Effect.first);
	//}
	//efx.sort();
	//for (auto&& Effect : efx)
	//	fmt::print("{}\n", Effect);

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
			static auto const fnTrName = [](const string& szName, const Trait_t& Trait) -> bool { return Trait.m_Name.contains(szName.c_str()); };
			static auto const fnTrNameOr = [](const auto& rgszNames, const Trait_t& Trait) -> bool
			{
				for (auto&& Name : rgszNames)
				{
					if (Trait.m_Name.contains(Name))
						return true;
				}

				return false;
			};
			static auto const fnLvName = [](const string& szName, const Trait_t& Trait) -> bool
			{
				for (auto&& Level : Trait.m_Levels)
				{
					if (Level.m_Name.contains(szName))
						return true;
				}

				return false;
			};
			static auto const fnLvNameOr = [](const auto& rgszNames, const Trait_t& Trait) -> bool
			{

				for (auto&& Level : Trait.m_Levels)
				{
					for (auto&& Name : rgszNames)
						if (Level.m_Name.contains(Name))
							return true;
				}

				return false;
			};
			static auto const fnName = [](const string& szName, const Trait_t& Trait) -> bool { return fnTrName(szName, Trait) || fnLvName(szName, Trait); };
			static auto const fnNameOr = [](const auto& rgszNames, const Trait_t& Trait) -> bool { return fnTrNameOr(rgszNames, Trait) || fnLvNameOr(rgszNames, Trait); };

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
					g_rgfnFilters.push_back(std::bind_front(fnTrName, string(cmdarg[2])));	// #UPDATE_AT_CPP23	std::bind_back feels more natural.
					g_rgszInfoStrings.emplace_back(fmt::format("Must contains: \"{}\" in trait name", cmdarg[2]));
				}
				else
				{
					g_rgfnFilters.push_back(std::bind_front(fnTrNameOr, vector<string>(cmdarg.begin() + 2, cmdarg.end())));	// #UPDATE_AT_CPP23	std::bind_back feels more natural.
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
					g_rgfnFilters.push_back(std::bind_front(fnLvName, string(cmdarg[2])));	// #UPDATE_AT_CPP23	std::bind_back feels more natural.
					g_rgszInfoStrings.emplace_back(fmt::format("Must contains: \"{}\" in any of level name", cmdarg[2]));
				}
				else
				{
					g_rgfnFilters.push_back(std::bind_front(fnLvNameOr, vector<string>(cmdarg.begin() + 2, cmdarg.end())));	// #UPDATE_AT_CPP23	std::bind_back feels more natural.
					g_rgszInfoStrings.emplace_back(fmt::format("Must contains one of these strings in its level name: \"{}\"", fmt::join(cmdarg | std::views::drop(2), "\", \"")));
				}
			}
			else if (cmdarg.size() == 2)
			{
				g_rgfnFilters.push_back(std::bind_front(fnName, string(cmdarg[1])));	// #UPDATE_AT_CPP23	std::bind_back feels more natural.
				g_rgszInfoStrings.emplace_back(fmt::format("Must contains: \"{}\" in either trait name or level names.", cmdarg[1]));
			}
			else if (cmdarg.size() > 2)
			{
				g_rgfnFilters.push_back(std::bind_front(fnNameOr, vector<string>(cmdarg.begin() + 1, cmdarg.end())));	// #UPDATE_AT_CPP23	std::bind_back feels more natural.
				g_rgszInfoStrings.emplace_back(fmt::format("Must contains one of these strings in either trait name or level names: \"{}\"", fmt::join(cmdarg | std::views::drop(1), "\", \"")));
			}
			else [[unlikely]]
				std::unreachable();
		}
		else if (cmdarg[0] == "prop")
		{
			static auto const fnLesser = [](string szProperty, short iModifer, const Trait_t& Trait) -> bool
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
			static auto const fnLesserOrEqual = [](string szProperty, short iModifer, const Trait_t& Trait) -> bool
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
			static auto const fnEqual = [](string szProperty, short iModifer, const Trait_t& Trait) -> bool
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
			static auto const fnGreaterOrEqual = [](string szProperty, short iModifer, const Trait_t& Trait) -> bool
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
			static auto const fnGreater = [](string szProperty, short iModifer, const Trait_t& Trait) -> bool
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
				g_rgfnFilters.push_back(std::bind_front(fnLesser, string(cmdarg[1]), iCompare));
				g_rgszInfoStrings.emplace_back(fmt::format("Containing property(ies) that have effect with string \"{}\" and lesser than {}.", cmdarg[1], iCompare));
			}
			else if (cmdarg[2] == "<=")
			{
				g_rgfnFilters.push_back(std::bind_front(fnLesserOrEqual, string(cmdarg[1]), iCompare));
				g_rgszInfoStrings.emplace_back(fmt::format("Containing property(ies) that have effect with string \"{}\" and lesser than or equals to {}.", cmdarg[1], iCompare));
			}
			else if (cmdarg[2] == "==")
			{
				g_rgfnFilters.push_back(std::bind_front(fnEqual, string(cmdarg[1]), iCompare));
				g_rgszInfoStrings.emplace_back(fmt::format("Containing property(ies) that have effect with string \"{}\" and equals to {}.", cmdarg[1], iCompare));
			}
			else if (cmdarg[2] == ">=")
			{
				g_rgfnFilters.push_back(std::bind_front(fnGreaterOrEqual, string(cmdarg[1]), iCompare));
				g_rgszInfoStrings.emplace_back(fmt::format("Containing property(ies) that have effect with string \"{}\" and greater than or equals to {}.", cmdarg[1], iCompare));
			}
			else if (cmdarg[2] == ">")
			{
				g_rgfnFilters.push_back(std::bind_front(fnGreater, string(cmdarg[1]), iCompare));
				g_rgszInfoStrings.emplace_back(fmt::format("Containing property(ies) that have effect with string \"{}\" and greater than {}.", cmdarg[1], iCompare));
			}
			else
			{
				fmt::print(fg(fmt::color::red), "Operator '{}' does not supported.\n", cmdarg[2]);

				if (cmdarg[2] == "=")
					fmt::print(fg(fmt::color::red), "Use '==' as equality operator instead.\n");
				else if (cmdarg[2] == "<>" || cmdarg[2] == "!=")
					fmt::print(fg(fmt::color::red), "Operator 'not equal' does not make sense here.\n");

				continue;
			}

			//g_rgfnFilters.push_back(fn);
			//g_rgszInfoStrings.emplace_back("Must contains only positive effect on all level(s).");
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
		else if (cmdarg[0] == "list")
		{
			clear_console();
			fmt::print("Listing trait(s) under filter(s):\n");
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
			clear_console();
		}
		else if (cmdarg[0] == "clear")
		{
			clear_console();
		}
		else
			fmt::print(fg(fmt::color::red), "Unknown command: \"{}\".\n", cmdarg[0]);
	}

#pragma region Handle unparsed tokens
	g_rgszUnknownTokens.sort();
	g_rgszUnknownTokens.unique();

	for (const auto& Token : g_rgszUnknownTokens)
		fmt::print(fg(fmt::color::dark_golden_rod), "Unparsed token: {}\n", Token);
#pragma endregion Handle unparsed tokens

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
