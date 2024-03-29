#include <fmt/core.h>

#include <ranges>
#include <experimental/generator>

#include "Requirements.hpp"

using namespace std::literals;

using std::array;
using std::experimental::generator;
using std::string_view;

inline constexpr char RequirementTestString[] =
R"(requires factions { england, france, spain, } and building_present barracks or building_present_min_level core_building stone_wall and event_counter found_america 1 and hidden_resource america and resource iron_mine or not region_religion pagan 50)";

inline constexpr char RequirementSimpleString[] =
R"(requires factions { all, })";

template <typename... Tys>
struct LambdaSet : Tys...
{
	using Tys::operator()...;
};

static inline generator<string_view> Split(const string_view& s, const char* delimiters = ", \n\f\v\t\r") noexcept
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

namespace Requirements
{
	inline constexpr LambdaSet Serialize
	{
		[](CFactions const& RequirementObject) noexcept -> string
		{
			return fmt::format("{}{} {{ {}, }}",
				RequirementObject.m_Inverted ? "not " : "",
				RequirementObject.Function,
				fmt::join(RequirementObject.m_Arguments, ", ")
			);
		},

		[](auto const& RequirementObject) noexcept -> string
		{
			return fmt::format("{}{} {}",
				RequirementObject.m_Inverted ? "not " : "",
				RequirementObject.Function,
				fmt::join(RequirementObject.m_Arguments, " ")
			);
		},
	};

	CCell Deserialize(vector<string_view> const& Verses) noexcept
	{
		bool const bInv{ Verses[0] == "not" };
		size_t const iPos = bInv ? 1 : 0;

		if (Verses[iPos] == Keywords[0])
			return CBuildingPresent{ .m_Inverted{bInv}, .m_Arguments{ (string)Verses[iPos + 1] } };
		else if (Verses[iPos] == Keywords[1])
			return CBuildingMinLevel{ .m_Inverted{bInv}, .m_Arguments{ (string)Verses[iPos + 1], (string)Verses[iPos + 2] } };
		else if (Verses[iPos] == Keywords[2])
			return CEventCounter{ .m_Inverted{bInv}, .m_Arguments{ (string)Verses[iPos + 1], (string)Verses[iPos + 2] } };
		else if (Verses[iPos] == Keywords[3])
		{
			auto pos1 = std::ranges::find(Verses, "{") + 1;
			auto pos2 = std::ranges::find(Verses, "}");

			[[unlikely]]
			if (auto const endpos = Verses.end(); pos1 == endpos || pos2 == endpos || (pos2 - pos1) < 1)
				std::terminate();

			return CFactions{ .m_Inverted{bInv}, .m_Arguments{ pos1, pos2 } };
		}
		else if (Verses[iPos] == Keywords[4])
			return CHiddenResources{ .m_Inverted{bInv}, .m_Arguments{ (string)Verses[iPos + 1] } };
		else if (Verses[iPos] == Keywords[5])
			return CRegionReligion{ .m_Inverted{bInv}, .m_Arguments{ (string)Verses[iPos + 1], (string)Verses[iPos + 2] } };
		else if (Verses[iPos] == Keywords[6])
			return CResources{ .m_Inverted{bInv}, .m_Arguments{ (string)Verses[iPos + 1] } };
		else [[unlikely]]
			std::terminate();

			std::unreachable();
	}

	CObject Compile(string_view sz) noexcept
	{
		auto rgszWords = Split(sz) | std::ranges::to<vector>();
		std::erase(rgszWords, "requires");

		vector<vector<string_view>> Ands{};
		size_t iBeginPos{}, iCurPos{};

		for (; iCurPos < rgszWords.size(); ++iCurPos)
		{
			if (rgszWords[iCurPos] == "and")
			{
				Ands.emplace_back(rgszWords.begin() + iBeginPos, rgszWords.begin() + iCurPos);

				iBeginPos = iCurPos + 1;
			}
		}

		Ands.emplace_back(rgszWords.begin() + iBeginPos, rgszWords.end());

		vector<vector<CCell>> Compiled{};

		for (auto&& Verses : Ands)
		{
			auto& Ors = Compiled.emplace_back();

			iBeginPos = 0, iCurPos = 0;
			for (; iCurPos < Verses.size(); ++iCurPos)
			{
				if (Verses[iCurPos] == "or")
				{
					Ors.emplace_back(Deserialize({ Verses.begin() + iBeginPos, Verses.begin() + iCurPos }));
					iBeginPos = iCurPos + 1;
				}
			}

			Ors.emplace_back(Deserialize({ Verses.begin() + iBeginPos, Verses.end() }));
		}

		//for (auto&& Ors : Compiled)
		//{
		//	fmt::print("\tOR = [\n");
		//	for (auto&& Requirement : Ors)
		//	{
		//		fmt::print("\t\t{}\n", std::visit(Serialize, Requirement));
		//	}
		//	fmt::print("\t]\n");
		//}

		return Compiled;
	}

	string Decompile(CObject const& Tree) noexcept
	{
		static constexpr auto fnOperatorOrTransform = [](vector<CCell> const& Ors) noexcept -> string
			{
				return fmt::format("{}",
					fmt::join(Ors | std::views::transform([](CCell const& obj) noexcept -> string { return std::visit(Serialize, obj); }), " or ")
				);
			};

		return fmt::format("requires {}",
			fmt::join(Tree | std::views::transform(fnOperatorOrTransform), " and ")
		);
	}

	void UnitTest(void) noexcept
	{
		auto ret = Compile(RequirementTestString);
		auto dec = Decompile(ret);

		fmt::print("Decompiled:\n{}\n", dec);
		fmt::print("Original:\n{}\n", RequirementTestString);
		fmt::print("\ncmp: {}\n", dec == RequirementTestString);

		fmt::print("\n=====================================================================\n");

		ret = Compile(RequirementSimpleString);
		dec = Decompile(ret);

		fmt::print("Decompiled:\n{}\n", dec);
		fmt::print("Original:\n{}\n", RequirementSimpleString);
		fmt::print("\ncmp: {}\n", dec == RequirementSimpleString);
	}
}

