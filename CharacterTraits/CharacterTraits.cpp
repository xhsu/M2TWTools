// CharacterTraits.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <deque>
#include <list>
#include <string>
#include <variant>
#include <vector>

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

using std::experimental::generator;

enum class Character_e
{
	ERR,

	family,

};

enum class Effect_e
{
	ERR,

	Command,

};

enum class Culture_e
{
	NONE,

	middle_eastern,
};

struct Level_t
{
	string m_Name{};
	string m_Description{};
	string m_EffectsDescription{};
	string m_GainMessage{};
	string m_LoseMessage{};
	string m_Epithet{};
	short m_Threshold = 0;
	short m_Level = 0;
	vector<pair<string, short>> m_Effects{};
};

struct Trait_t
{
	string m_Name{};
	vector<string> m_Character{};
	vector<string> m_ExcludeCultures{};
	short m_NoGoingBackLevel = -1;
	bool m_Hidden : 1 { false };
	vector<variant<string, const Trait_t*>> m_AntiTraits{};
	deque<Level_t> m_Levels{};	// std::vector would move and thus invalidate our pointer.

	inline void Print(void) const noexcept
	{
		static constexpr auto SPACES_COUNT = 17;
		static constexpr auto SPACES_COUNT2 = 20;

		fmt::print("{}\n", m_Name);

		if (!m_Character.empty())
		{
			string ListStr{};

			for (const auto& s : m_Character)
				ListStr += s + ", ";

			ListStr.pop_back();
			ListStr.pop_back();

			fmt::print("\t{0: <{2}}{1}\n", "Characters", ListStr, SPACES_COUNT);
		}

		if (!m_ExcludeCultures.empty())
		{
			string ListStr{};

			for (const auto& s : m_ExcludeCultures)
				ListStr += s + ", ";

			ListStr.pop_back();
			ListStr.pop_back();

			fmt::print("\t{0: <{2}}{1}\n", "ExcludeCultures", ListStr, SPACES_COUNT);
		}

		if (m_NoGoingBackLevel >= 0)
		{
			fmt::print("\t{0: <{2}}{1}\n", "NoGoingBackLevel", m_NoGoingBackLevel, SPACES_COUNT);
		}

		if (m_Hidden)
		{
			fmt::print("\tHidden\n");
		}

		if (!m_AntiTraits.empty())
		{
			string ListStr{};

			for (const auto& AntiTrait : m_AntiTraits)
			{
				try
				{
					ListStr += std::get<string>(AntiTrait) + ", ";
				}
				catch (...)
				{
					try
					{
						ListStr += std::get<const Trait_t*>(AntiTrait)->m_Name + ", ";
					}
					catch (...)
					{
						ListStr = "ERROR";
						fmt::print(fg(fmt::color::dark_red), "ERROR: YOU SHOULD NEVER SEE THIS!");
					}
				}
			}

			ListStr.pop_back();
			ListStr.pop_back();

			fmt::print("\t{0: <{2}}{1}\n", "AntiTraits", ListStr, SPACES_COUNT);
		}

		for (const auto& Level : m_Levels)
		{
			fmt::print("\n\tLevel {}\n", Level.m_Name);

			if (!Level.m_Description.empty())
			{
				fmt::print("\t\t{0: <{2}}{1}\n", "Description", Level.m_Description, SPACES_COUNT2);
			}

			if (!Level.m_EffectsDescription.empty())
			{
				fmt::print("\t\t{0: <{2}}{1}\n", "EffectsDescription", Level.m_EffectsDescription, SPACES_COUNT2);
			}

			if (!Level.m_GainMessage.empty())
			{
				fmt::print("\t\t{0: <{2}}{1}\n", "GainMessage", Level.m_GainMessage, SPACES_COUNT2);
			}

			if (!Level.m_LoseMessage.empty())
			{
				fmt::print("\t\t{0: <{2}}{1}\n", "LoseMessage", Level.m_LoseMessage, SPACES_COUNT2);
			}

			if (!Level.m_Epithet.empty())
			{
				fmt::print("\t\t{0: <{2}}{1}\n", "Epithet", Level.m_Epithet, SPACES_COUNT2);
			}

			if (Level.m_Threshold)
			{
				fmt::print("\t\t{0: <{2}}{1}\n", "Threshold", Level.m_Threshold, SPACES_COUNT2);
			}

			for (const auto& Effect : Level.m_Effects)
			{
				fmt::print("\t\t{0: <{1}}{2} {3}\n", "Effect", SPACES_COUNT2, Effect.first, Effect.second);
			}
		}

		fmt::print("\n;------------------------------------------\n");
	}
};

deque<Trait_t> g_Traits;
Trait_t* g_pCurTrait = nullptr;
Level_t* g_pCurLevel = nullptr;
list<string> g_rgszUnknownTokens;

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

				g_pCurLevel->m_Effects.emplace_back(rgsz[0], std::stoi(string(rgsz[1])));
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

	for (const auto& Trait : g_Traits)
		Trait.Print();

	// Print unparsed tokens.
	g_rgszUnknownTokens.sort();
	g_rgszUnknownTokens.unique();

	for (const auto& Token : g_rgszUnknownTokens)
		fmt::print(fg(fmt::color::dark_golden_rod), "Unparsed token: {}\n", Token);

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
