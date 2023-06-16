#pragma once

#include "String.hpp"

#include <map>	// #UPDATE_AT_CPP23 flat_map

namespace UnitsVoice
{
	namespace fs = std::filesystem;

	using std::experimental::generator;
	using std::string;
	using std::string_view;
	using std::tuple;
	using std::vector;
	using std::map;

	struct CEvent final
	{
		string_view m_Folder{};
		vector<string_view> m_Files{};
		enum { None, Engine, Unit } m_Type{ None };
		vector<string_view> m_Troops{};

		string Serialize() const noexcept;
	};

	struct CVocal final
	{
		string_view m_Name{};
		vector<string_view> m_Arguments{};
		vector<CEvent> m_Events{};

		string Serialize() const noexcept;
	};

	struct CClass final
	{
		string_view m_Name{};
		map<string_view, CVocal> m_Vocals{};

		string Serialize() const noexcept;
	};

	struct CAccent final
	{
		string_view m_Name{};
		map<string_view, CClass> m_Classes{};

		string Serialize() const noexcept;
	};

	struct CUnitVoices final
	{
		explicit CUnitVoices(fs::path const& Path) noexcept;
		~CUnitVoices() noexcept;

		CUnitVoices(CUnitVoices const&) noexcept = delete;
		CUnitVoices(CUnitVoices&&) noexcept = delete;

		CUnitVoices& operator=(CUnitVoices const&) noexcept = delete;
		CUnitVoices& operator=(CUnitVoices&&) noexcept = delete;

	private:
		char* m_p{};
		size_t m_length{};

		static inline constexpr auto m_fnNameOf = [](auto&& obj) noexcept { return obj.m_Name; };

	public:
		map<string_view, CAccent> m_Accents{};

		inline bool operator== (CUnitVoices const& rhs) const noexcept { return Serialize() == rhs.Serialize(); }

		void Deserialize() noexcept;
		string Serialize() const noexcept;
		bool Save(fs::path const& Path) const noexcept;

		CAccent* At(string_view szAccent) noexcept;
		CClass* At(string_view szAccent, string_view szClass) noexcept;
		CVocal* At(string_view szAccent, string_view szClass, string_view szVocal) noexcept;

		// #UPDATE_AT_CPP23 multiple argument for operator[] - compiler feature.
		CVocal& operator[] (string_view sz) noexcept = delete;

		generator<tuple<CAccent*, CClass*, CVocal*>> EveryVocalOf(string_view szVocal) noexcept;
		generator<tuple<CAccent*, CClass*, CVocal*, CEvent*>> EveryUnitOf(string_view szUnit) noexcept;
	};
}
