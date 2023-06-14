/*
* June 14 2023
*/

import std.compat;

using namespace ::std;
using namespace ::std::literals;

namespace fs = ::std::filesystem;

inline void Delete(fs::path const& file) noexcept
{
	auto const sz{ fs::relative(file, fs::current_path()).u8string() };
	std::error_code ec{};
	fs::remove(file, ec);

	if (!ec)
		cout << std::format("FILE deleted: {}\n", sz);
	else
		cout << std::format("FILE '{}' deletion error '{}': {}\n", sz, ec.value(), ec.message());
}

int32_t main(int32_t, char*[]) noexcept
{
	auto const szSoundPath = fs::current_path() / L"data" / L"sounds";

	const array rgszSoundFilesToDelete
	{
		szSoundPath / L"events.dat",
		szSoundPath / L"events.idx",
		szSoundPath / L"Music.dat",
		szSoundPath / L"Music.idx",
		szSoundPath / L"SFX.dat",
		szSoundPath / L"SFX.idx",
		szSoundPath / L"Voice.dat",
		szSoundPath / L"Voice.idx",
	};

	for (auto&& file : rgszSoundFilesToDelete)
		Delete(file);

	for (auto&& file : fs::directory_iterator(fs::current_path() / L"data" / L"text")
		| std::views::transform([](auto&& obj) noexcept -> fs::path { return obj; })
		| std::views::filter([](fs::path const& Path) noexcept { return Path.native().ends_with(L".strings.bin"); })
		| std::views::filter([](fs::path const& Path) noexcept { auto const sz = Path.filename().native(); return !sz.starts_with(L"battle.txt") && !sz.starts_with(L"shared.txt") && !sz.starts_with(L"strat.txt") && !sz.starts_with(L"tooltips.txt"); })
		)
	{
		Delete(file);
	}

	Delete(fs::current_path() / L"data/world/maps/base/map.rwm");

	return 0;
}
