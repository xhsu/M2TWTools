// FileMerger.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <fmt/color.h>
#include <fmt/ranges.h>

#include <experimental/generator>
#include <ranges>
#include <set>
#include <vector>

using std::experimental::generator;
using std::set;
using std::string;
using std::string_view;
using std::vector;

static inline generator<string_view> Split(string_view sz, string_view delimiters = ", \n\f\v\t\r") noexcept
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

static auto Conclude(const char* file) noexcept -> set<string, std::less<string_view>>
{
	if (auto f = fopen(file, "rb"); f)
	{
		fseek(f, 0, SEEK_END);
		auto const len = ftell(f);

		auto const p = (char*)calloc(sizeof(char), len + 1);
		fseek(f, 0, SEEK_SET);
		fread(p, sizeof(char), len, f);

		fclose(f);

		auto const rgsz =
			Split({ p, (size_t)len }, "\r\n")
			| std::views::transform([](string_view const& sz) noexcept { return (string)sz; })
			| std::ranges::to<set<string, std::less<string_view>>>();

		free(p);

		return rgsz;
	}

	return {};
}

void Save(const char* file, std::ranges::input_range auto&& words) noexcept
{
	if (auto f = fopen(file, "wb"); f)
	{
		for (auto&& line : words)
			fmt::print(f, "{}\n", line);

		fclose(f);
	}
}

int main(int, char* []) noexcept
{
	auto van_sk = Conclude(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\data\Animations\New folder\list.txt)");
	auto crus_sk = Conclude(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\crusades\data\animations\New folder\list.txt)");

	vector<string> diff{};
	std::ranges::set_symmetric_difference(van_sk, crus_sk, std::back_inserter(diff));

	fmt::print("{}\n", fmt::join(diff, "\n"));

	set<string, std::less<>> un{};
	std::ranges::set_union(van_sk, crus_sk, std::inserter(un, un.begin()));
	Save(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data\animations\SK_PACK\list.txt)", un);


	fmt::print("\n\n");


	auto van_am = Conclude(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\data\Animations\anim_list.txt)");
	auto crus_am = Conclude(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\crusades\data\animations\anim_list.txt)");

	diff.clear();
	std::ranges::set_symmetric_difference(van_am, crus_am, std::back_inserter(diff));

	fmt::print("{}\n", fmt::join(diff, "\n"));

	un.clear();
	std::ranges::set_union(van_am, crus_am, std::inserter(un, un.begin()));
	Save(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data\animations\anim_list.txt)", un);

	return 0;
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
