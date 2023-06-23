#include "export_descr_sounds.hpp"

#include <fmt/color.h>
#include <fmt/std.h>
#include <fmt/ranges.h>

#include <functional>
#include <ranges>

using namespace std::literals;
using namespace Voice;

string Voice::CSimpleFolder::Serialize(int iIndent) const noexcept
{
	auto const Indent = fmt::format("{}", fmt::join(std::views::repeat('\t') | std::views::take(iIndent), ""));
	auto ret = fmt::format("{}folder {}\n", Indent, m_Folder);

	for (auto&& file : m_Files)
		ret += fmt::format("{}\t{}\n", Indent, file);

	return ret;
}

string Voice::CSimpleEvent::Serialize(int iIndent) const noexcept
{
	auto const Indent = fmt::format("{}", fmt::join(std::views::repeat('\t') | std::views::take(iIndent), ""));

	return
		fmt::format(
			"{0}event {1}\n{2}{0}end\n",
			Indent,
			fmt::join(m_Arguments, " "),
			fmt::join(m_Folders | std::views::transform(std::bind_back(&CSimpleFolder::Serialize, iIndent + 1)), "")
		);
}

CSimpleEvent Voice::CSimpleEvent::Deserialize(CBaseParser* File, string_view Line) noexcept
{
	// Assmue the first command is 'event'

	auto const Verses = UTIL_Split(Line) | std::views::drop(1) | std::ranges::to<vector>();	// drop 'event'

	CSimpleEvent ret{ .m_Arguments{UTIL_Split(Line) | std::views::drop(1) /* drop 'event' */ | std::ranges::to<vector>()}};

	CSimpleFolder* m_ThisFolder{};
	for (Line = File->Parse("\r\n"); !File->Eof() && Line != "end"; Line = File->Parse("\r\n"))
	{
		if (Line.starts_with("folder "))
			m_ThisFolder = &ret.m_Folders.emplace_back(CSimpleFolder{ .m_Folder{Line | std::views::drop("folder "sv.length())} });
		else
			m_ThisFolder->m_Files.emplace_back(Line);
	}

	return ret;
}
