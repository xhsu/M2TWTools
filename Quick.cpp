#include <assert.h>
#include <stdio.h>

#include <array>
#include <filesystem>
#include <experimental/generator>
#include <ranges>
#include <deque>
#include <vector>

#include <fmt/color.h>

namespace fs = std::filesystem;

using namespace std::literals;

using std::array;
using std::deque;
using std::experimental::generator;
using std::string;
using std::string_view;
using std::tuple;
using std::vector;

inline bool strieql(string_view lhs, string_view rhs) noexcept
{
	return std::ranges::equal(
		lhs, rhs,
		[](char lc, char rc) noexcept { return std::tolower(lc) == std::tolower(rc); }
	);
}

generator<string_view> Split(string_view sz, string_view delimiters = ", \n\f\v\t\r"sv) noexcept
{
	for (auto lastPos = sz.find_first_not_of(delimiters, 0), pos = sz.find_first_of(delimiters, lastPos);
		sz.npos != pos || sz.npos != lastPos;
		lastPos = sz.find_first_not_of(delimiters, pos), pos = sz.find_first_of(delimiters, lastPos)
		)
	{
		co_yield string_view{
			sz.substr(lastPos, pos - lastPos) | std::views::drop_while([](const char c) noexcept { return std::isspace(c); })
		};
	}

	co_return;
}

struct CEvent final
{
	string_view m_Folder{};
	vector<string_view> m_Files{};
	enum { None, Engine, Unit } m_Type{ None };
	vector<string_view> m_Troops{};

	string Serialize() const noexcept
	{
		string ret{};

		switch (m_Type)
		{
		case CEvent::Engine:
			ret += fmt::format("\t\t\t\tengine {}\n", fmt::join(m_Troops, ", "));
			break;
		case CEvent::Unit:
			ret += fmt::format("\t\t\t\tunit {}\n", fmt::join(m_Troops, ", "));
			break;

		case CEvent::None:
		default:
			break;
		}

		ret += "\t\t\t\tevent\n";
		ret += fmt::format("\t\t\t\t\tfolder {}\n", m_Folder);

		for (auto&& file : m_Files)
			ret += fmt::format("\t\t\t\t\t\t{}\n", file);

		ret += "\t\t\t\tend\n";

		return ret;
	}
};

struct CVocal final
{
	string_view m_Name{};
	vector<string_view> m_Arguments{};
	vector<CEvent> m_Events{};

	string Serialize() const noexcept
	{
		auto ret = fmt::format("\t\t\tvocal {} {}\n", m_Name, fmt::join(m_Arguments, " "));

		for (auto&& Event : m_Events)
			ret += Event.Serialize();

		return ret;
	}
};

struct CClass final
{
	string_view m_Name{};
	deque<CVocal> m_Vocals{};

	string Serialize() const noexcept
	{
		auto ret = fmt::format("\t\tclass {}\n", m_Name);

		for (auto&& Vocal : m_Vocals)
			ret += Vocal.Serialize();

		return ret;
	}
};

struct CAccent final
{
	string_view m_Name{};
	deque<CClass> m_Classes{};

	string Serialize() const noexcept
	{
		auto ret = fmt::format("\taccent {}\n", m_Name);

		for (auto&& Class : m_Classes)
			ret += Class.Serialize();

		return ret;
	}
};

struct CUnitVoices final
{
	explicit CUnitVoices(fs::path const& Path) noexcept
	{
		if (auto f = _wfopen(Path.c_str(), L"rb"); f != nullptr)
		{
			fseek(f, 0, SEEK_END);
			m_length = ftell(f);

			m_p = (char*)calloc(sizeof(char), m_length + 1);
			fseek(f, 0, SEEK_SET);
			fread(m_p, sizeof(char), m_length, f);

			fclose(f);

			Deserialize();
		}
	}
	~CUnitVoices() noexcept
	{
		if (m_p)
		{
			free(m_p);

			m_p = nullptr;
			m_length = 0;
		}
	}

	CUnitVoices(CUnitVoices const&) noexcept = delete;
	CUnitVoices(CUnitVoices&&) noexcept = delete;

	CUnitVoices& operator=(CUnitVoices const&) noexcept = delete;
	CUnitVoices& operator=(CUnitVoices&&) noexcept = delete;

private:
	char* m_p{};
	size_t m_length{};

	static inline constexpr auto m_fnNameOf = [](auto&& obj) noexcept { return obj.m_Name; };

public:
	deque<CAccent> m_Accents{};

	inline bool operator== (CUnitVoices const& rhs) const noexcept { return Serialize() == rhs.Serialize(); }

	void Deserialize() noexcept
	{
		CAccent* m_ThisAccent{};
		CClass* m_ThisClass{};
		CVocal* m_ThisVocal{};
		CEvent* m_ThisEvent{};

		for (auto&& Line : Split({m_p, m_length}, "\n\r"))
		{
			if (Line.starts_with("BANK:"))
				continue;

			if (Line.starts_with("accent"))
			{
				auto const Verses = Split(Line) | std::ranges::to<vector>();
				assert(Verses.size() == 2);

				m_ThisAccent = &m_Accents.emplace_back(CAccent{ .m_Name{Verses[1]} });
				m_ThisClass = nullptr;
				m_ThisVocal = nullptr;
				assert(m_ThisEvent == nullptr);
			}
			else if (Line.starts_with("class"))
			{
				auto const Verses = string_view{
					Line
					| std::views::drop(sizeof("class"))
					| std::views::drop_while([](char c) noexcept { return std::isspace(c); })
				};

				assert(!Verses.contains(' '));

				m_ThisClass = &m_ThisAccent->m_Classes.emplace_back(CClass{ .m_Name{ Verses } });
				m_ThisVocal = nullptr;
				assert(m_ThisEvent == nullptr);
			}
			else if (Line.starts_with("vocal"))
			{
				auto const Verses = Split(Line) | std::ranges::to<vector>();
				assert(Verses.size() <= 3);

				m_ThisVocal = &m_ThisClass->m_Vocals.emplace_back(CVocal{ .m_Name{Verses[1]}, .m_Arguments{Verses.begin() + 2, Verses.end()} });
			}
			else if (Line.starts_with("engine"))
			{
				assert(m_ThisEvent == nullptr);
				m_ThisEvent = &m_ThisVocal->m_Events.emplace_back(
					CEvent{
						.m_Type{ CEvent::Engine },
						.m_Troops{ Split(Line) | std::views::drop(1) | std::ranges::to<vector>() },
					}
				);
			}
			else if (Line.starts_with("unit"))
			{
				assert(m_ThisEvent == nullptr);
				m_ThisEvent = &m_ThisVocal->m_Events.emplace_back(
					CEvent{
						.m_Type{ CEvent::Unit },
						.m_Troops{ string_view{Line | std::views::drop(sizeof("unit")) | std::views::drop_while([](char c) noexcept { return std::isspace(c); })} },
					}
				);

				while (std::isspace(m_ThisEvent->m_Troops.back().back()))
					m_ThisEvent->m_Troops.back() = m_ThisEvent->m_Troops.back().substr(0, m_ThisEvent->m_Troops.back().length() - 1);
			}
			else if (Line.starts_with("event"))
			{
				assert(m_ThisEvent == nullptr || m_ThisEvent->m_Type != CEvent::None);

				if (m_ThisEvent == nullptr)
					m_ThisEvent = &m_ThisVocal->m_Events.emplace_back();
			}
			else if (Line.starts_with("folder"))
			{
				m_ThisEvent->m_Folder = string_view{
					Line
					| std::views::drop(sizeof("folder"))
					| std::views::drop_while([](char c) noexcept { return std::isspace(c); })
				};
			}
			else if (Line.starts_with("end"))
			{
				assert(m_ThisEvent != nullptr);
				m_ThisEvent = nullptr;
			}
			else if (m_ThisEvent != nullptr)
			{
				m_ThisEvent->m_Files.emplace_back(Line);
			}
			else
				fmt::print(fg(fmt::color::red), "[ERROR] Unknown script command '{}'\n", Line);
		}
	}

	string Serialize() const noexcept
	{
		string ret{ "BANK: unit_voice\n" };
		ret.reserve(m_length);

		for (auto&& Accent : m_Accents)
			ret += Accent.Serialize();

		return ret;
	}

	CAccent* At(string_view szAccent) noexcept
	{
		if (auto const it = std::ranges::find(m_Accents, szAccent, m_fnNameOf);
			it != m_Accents.end())
		{
			return &(*it);
		}

		return nullptr;
	}

	CClass* At(string_view szAccent, string_view szClass) noexcept
	{
		auto const it = std::ranges::find(m_Accents, szAccent, m_fnNameOf);
		if (it == m_Accents.end())
			return nullptr;

		auto const it2 = std::ranges::find(it->m_Classes, szClass, m_fnNameOf);
		if (it2 == it->m_Classes.end())
			return nullptr;

		return &(*it2);
	}

	CVocal* At(string_view szAccent, string_view szClass, string_view szVocal) noexcept
	{
		auto const it = std::ranges::find(m_Accents, szAccent, m_fnNameOf);
		if (it == m_Accents.end())
			return nullptr;

		auto const it2 = std::ranges::find(it->m_Classes, szClass, m_fnNameOf);
		if (it2 == it->m_Classes.end())
			return nullptr;

		auto const it3 = std::ranges::find(it2->m_Vocals, szVocal, m_fnNameOf);
		if (it3 == it2->m_Vocals.end())
			return nullptr;

		return &(*it3);
	}

	generator<tuple<CAccent*, CClass*, CVocal*>> EveryVocalOf(string_view szVocal) noexcept
	{
		for (auto&& Accent : m_Accents)
		{
			for (auto&& Class : Accent.m_Classes)
			{
				for (auto&& Vocal : Class.m_Vocals)
				{
					if (Vocal.m_Name == szVocal)
						co_yield tuple{ &Accent, &Class, &Vocal };
				}
			}
		}

		co_return;
	}

	generator<tuple<CAccent*, CClass*, CVocal*, CEvent*>> EveryUnitOf(string_view szUnit) noexcept
	{
		for (auto&& Accent : m_Accents)
		{
			for (auto&& Class : Accent.m_Classes)
			{
				for (auto&& Vocal : Class.m_Vocals)
				{
					for (auto&& Event : Vocal.m_Events)
					{
						if (Event.m_Type == CEvent::Unit && Event.m_Troops[0] == szUnit)
							co_yield tuple{ &Accent, &Class, &Vocal, &Event };
					}
				}
			}
		}
	}

	bool Save(fs::path const& Path) const noexcept
	{
		if (auto f = _wfopen(Path.c_str(), L"wb"); f != nullptr)
		{
			fmt::print(f, "{}\n", Serialize());
			fclose(f);

			return true;
		}

		return false;
	}
};

int main() noexcept
{
	CUnitVoices mymod{ R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data\export_descr_sounds_units_voice.txt)" };
	CUnitVoices brit{ R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\british_isles_data\export_descr_sounds_units_voice.txt)" };

	static constexpr array units{
		"Sami Axemen",
		"Gotland Footmen",
		"Svenner",
	};

	for (auto&& szUnit : units)
	{
		for (auto&& [pAcc, pClass, pVoc, pEv] : brit.EveryUnitOf(szUnit))
		{
			fmt::println("britmod: {}/{}/{}/unit {}", pAcc->m_Name, pClass->m_Name, pVoc->m_Name, szUnit);

			if (auto p = mymod.At(pAcc->m_Name, pClass->m_Name, pVoc->m_Name); p != nullptr)
				p->m_Events.emplace_back(*pEv);
			else
				fmt::print(fg(fmt::color::red), "\t[ERROR] Cannot find same structure under mymod.\n");
		}
	}

	mymod.Save(fs::current_path() / L"export_descr_sounds_units_voice.txt");

	return 0;
}