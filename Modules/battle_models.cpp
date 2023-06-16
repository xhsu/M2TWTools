/*
* Module:	Dec 06 2021
* Fix:		Jun 08 2023
*/

#include "battle_models.hpp"

#include <charconv>
#include <ranges>

#include <fmt/color.h>

namespace fs = std::filesystem;

using namespace BattleModels;

using std::string;
using std::string_view;

template <typename T>
static inline auto UTIL_StrToNum(string_view sz) noexcept
{
	if constexpr (std::is_enum_v<T>)
	{
		if (std::underlying_type_t<T> ret{}; std::from_chars(sz.data(), sz.data() + sz.size(), ret).ec == std::errc{})
			return static_cast<T>(ret);
	}
	else
	{
		if (T ret{}; std::from_chars(sz.data(), sz.data() + sz.size(), ret).ec == std::errc{})
			return ret;
	}

	return T{};
}

static inline string Serialize(string_view sz) noexcept
{
	if (sz.empty())
		return "0";
	else
		return fmt::format("{} {}", sz.length(), sz);	// no space here.
}

static inline string Serialize(CMesh const& Mesh) noexcept
{
	return fmt::format(
		"{} {} \n",
		Serialize(Mesh.m_szPath),
		Mesh.m_iDistance
	);
}

static inline string Serialize(CTexture const& Texture) noexcept
{
	return fmt::format(
		"{} \n{} \n{} \n",
		Serialize(Texture.m_szTex),
		Serialize(Texture.m_szNorm),
		Serialize(Texture.m_szSprite)
	);
}

static inline string Serialize(CMount const& Mount) noexcept
{
	return fmt::format(
		"{} \n{} \n{} \n",
		Serialize(Mount.m_szType),
		Serialize(Mount.m_szWpn1),
		Serialize(Mount.m_szWpn2)
	);
}

static inline string Serialize(CStance const& Stance) noexcept
{
	auto ret = Serialize(Stance.m_Mount);

	ret += fmt::format("{} \n", Stance.m_rgszPrim.size());
	for (auto&& sz : Stance.m_rgszPrim)
		ret += fmt::format("{} \n", Serialize(sz));

	ret += fmt::format("{} \n", Stance.m_rgszSec.size());
	for (auto&& sz : Stance.m_rgszSec)
		ret += fmt::format("{} \n", Serialize(sz));

	return ret;
}

static inline string Serialize(CBattleModel const& Model) noexcept
{
	if (Model.m_szName == "")
		return "";

	string ret{};

	// Region 1: name
	ret += fmt::format("{} \n", Serialize(Model.m_szName));

	// Region 2: mesh
	ret += fmt::format("{} \n", Model.m_rgrgMeshGroup.size());
	for (auto&& rgMesh : Model.m_rgrgMeshGroup)
	{
		ret += fmt::format("{} \n", rgMesh.size());

		for (auto&& Mesh : rgMesh)
			ret += Serialize(Mesh);
	}

	// Region 3: unit texture
	ret += fmt::format("{} \n", Model.m_UnitTex.size());
	for (auto&& [szCountry, Texture] : Model.m_UnitTex)
	{
		ret += fmt::format("{} \n", Serialize(szCountry));
		ret += Serialize(Texture);
	}

	// Region 4: attachments texture
	ret += fmt::format("{} \n", Model.m_AttachmentTex.size());
	for (auto&& [szCountry, Texture] : Model.m_AttachmentTex)
	{
		ret += fmt::format("{} \n", Serialize(szCountry));
		ret += Serialize(Texture);
	}

	// Region 5: Stances
	ret += fmt::format("{} \n", Model.m_rgStances.size());
	for (auto&& Stance : Model.m_rgStances)
		ret += Serialize(Stance);

	// Region 6: Unknow numbers.
	ret += fmt::format(
		"{} {} {} \n",
		Model.m_iMysteryNum,
		fmt::join(Model.m_MysteryVector1, " "),
		fmt::join(Model.m_MysteryVector2, " ")
	);

	return ret;
}

template <bool bSpecial>
inline void BattleModels::CFile::ParseBlock(void) noexcept
{
	if (Eof())
		return;

	// Mark for special if this version had been called.
	if constexpr (bSpecial)
		m_bSpecialFirst = true;

	// Pre-allocate
	auto const szName = ParseString();
	CBattleModel& BattleModel = m_rgBattleModels[szName];

	// Region 1: name
	BattleModel.m_szName = szName;

	// Region 2: mesh
	if constexpr (!bSpecial)
	{
		BattleModel.m_rgrgMeshGroup.resize(ParseNumber());

		for (auto& rgMeshGroup : BattleModel.m_rgrgMeshGroup)
		{
			rgMeshGroup.resize(ParseNumber());

			for (auto& Mesh : rgMeshGroup)
			{
				Mesh.m_szPath = ParseString();
				Mesh.m_iDistance = ParseNumber();
			}
		}
	}
	else
	{
		BattleModel.m_rgrgMeshGroup.resize(1);	// For the "mount_pony", always be one.
		Skip(3);

		auto& rgMeshGroup = BattleModel.m_rgrgMeshGroup.front();
		rgMeshGroup.resize(ParseNumber());
		Skip(2);

		for (auto& Mesh : rgMeshGroup)
		{
			Mesh.m_szPath = ParseString();
			Mesh.m_iDistance = ParseNumber();
		}
	}

	// Region 3: unit texture
	if constexpr (bSpecial)	// It's like "0 0 28 0 0 " for "mount_pony"
		Skip(2);

	auto iMaxSize = ParseNumber();
	/*BattleModel.m_UnitTex.reserve(iMaxSize);*/

	if constexpr (bSpecial)
		Skip(2);

	for (auto i = 0; i < iMaxSize; ++i)
	{
		CTexture& Tex = BattleModel.m_UnitTex[ParseString()];

		Tex.m_szTex = ParseString();
		Tex.m_szNorm = ParseString();
		Tex.m_szSprite = ParseString();
	}

	// Region 4: attachment texture
	iMaxSize = ParseNumber();
	/*BattleModel.m_AttachmentTex.reserve(iMaxSize);*/

	for (auto i = 0; i < iMaxSize; ++i)
	{
		CTexture& Tex = BattleModel.m_AttachmentTex[ParseString()];

		Tex.m_szTex = ParseString();
		Tex.m_szNorm = ParseString();
		Tex.m_szSprite = ParseString();
	}

	// Region 5: Stances
	if constexpr (bSpecial)
		Skip(2);

	BattleModel.m_rgStances.resize(ParseNumber());

	if constexpr (bSpecial)
		Skip(2);

	for (auto& Stance : BattleModel.m_rgStances)
	{
		Stance.m_Mount.m_szType = ParseString();
		Stance.m_Mount.m_szWpn1 = ParseString();
		Stance.m_Mount.m_szWpn2 = ParseString();

		Stance.m_rgszPrim.resize(ParseNumber());
		for (auto& Eqp : Stance.m_rgszPrim)
			Eqp = ParseString();

		Stance.m_rgszSec.resize(ParseNumber());
		for (auto& Eqp : Stance.m_rgszSec)
			Eqp = ParseString();
	}

	if constexpr (bSpecial)
		Skip(2);

	// Region 6: Unknow numbers.
	BattleModel.m_iMysteryNum = ParseNumber();
	BattleModel.m_MysteryVector1[0] = ParseNumber<float>();
	BattleModel.m_MysteryVector1[1] = ParseNumber<float>();
	BattleModel.m_MysteryVector1[2] = ParseNumber<float>();
	BattleModel.m_MysteryVector2[0] = ParseNumber<float>();
	BattleModel.m_MysteryVector2[1] = ParseNumber<float>();
	BattleModel.m_MysteryVector2[2] = ParseNumber<float>();

	if constexpr (bSpecial)
		Skip(2);
}

template <typename T>
T BattleModels::CFile::ParseNumber(void) noexcept
{
	return UTIL_StrToNum<T>(Parse());
}

string_view BattleModels::CFile::ParseString(void) noexcept
{
	auto const iSize = ParseNumber();

	if (iSize <= 0)
		return "";

	SkipUntilNonspace();
	return Parse(iSize);
}

void BattleModels::CFile::Initialize(void) noexcept
{
	Seek(0, SEEK_SET);
	Skip(7);

	m_iTotalModels = ParseNumber();
	m_rgBattleModels.clear();
	/*m_rgBattleModels.reserve(m_iTotalModels);*/

	Skip(2);
	if (ParseString() == "blank")
	{
		Skip(39);
		--m_iTotalModels;	// blank count as 1.
	}
	else
	{
		Rewind(2);	// Going back so that ParseString() reads therefore we can properly get the name.
		ParseBlock<true>();
	}

	for (size_t i = 0; i < m_iTotalModels; ++i)
		ParseBlock();
}

bool BattleModels::CFile::Sanity(void) const noexcept	// #UNDONE
{
	for (const auto& BattleModel : m_rgBattleModels | std::views::values)
	{
		if (BattleModel.m_szName.empty())
		{
			fmt::print(fg(fmt::color::red), "Null model name found.\n");
			continue;
		}

		for (const auto& [szCountry, Texture] : BattleModel.m_UnitTex)
		{
			if (!std::ranges::contains(BattleModel.m_AttachmentTex | std::views::keys, szCountry))
//			if (!BattleModel.m_AttachmentTex.contains(szCountry))
			{
				fmt::print(
					fg(fmt::color::red),
					R"([{0}] "{1}" entry found in unit texture section but absent from attachment texture section.\n)",
					BattleModel.m_szName,
					szCountry
				);
				continue;
			}
		}
	}

	return true;
}

void BattleModels::CFile::Save(const char* pszFilePath) const noexcept
{
	auto f = fopen(pszFilePath, "wb");
	if (!f)
		return;

	fmt::println(f, "22 serialization::archive 3 0 0 0 0 {} 0 0 ", Count() + 1);
	fmt::println(f, "5 blank 0 0 0 1 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ");

	for (const auto& BattleModel : m_rgBattleModels | std::views::values)
		fmt::print(f, "{}", Serialize(BattleModel));
}

size_t BattleModels::CFile::Count(void) const noexcept
{
	size_t ret = 0;

	for (const auto& BattleModel : m_rgBattleModels | std::views::values)
	{
		if (!BattleModel.m_szName.empty())
			++ret;
	}

	return ret;
}

set<fs::path> BattleModels::CTexture::ListOfFiles(fs::path const& DataPath) const noexcept
{
	set<fs::path> ret{};

	if (auto TexPath = DataPath / m_szTex; fs::exists(TexPath))
		ret.emplace(std::move(TexPath));
	if (auto NormPath = DataPath / m_szNorm; fs::exists(NormPath))
		ret.emplace(std::move(NormPath));
	if (auto SpritePath = DataPath / m_szSprite; fs::exists(SpritePath) && !fs::is_directory(SpritePath))
	{
		//ret.insert_range(
		//	fs::recursive_directory_iterator(SpritePath.parent_path())
		//	| std::views::transform([](auto&& Entry) noexcept -> fs::path { return Entry; })
		//	| std::views::filter([&](fs::path const& Path) noexcept { return Path.has_stem() && Path.stem().native().starts_with(SpritePath.stem().native()); })
		//);

		auto const szStem = SpritePath.parent_path().native() + L"\\" + SpritePath.stem().native();
		for (int i = 0; i < 10; ++i)
			if (fs::path Dependency{ szStem + L"_00" + std::to_wstring(i) + L".texture" }; fs::exists(Dependency))
				ret.emplace(std::move(Dependency));

		ret.emplace(std::move(SpritePath));
	}

	return ret;
}

set<string_view> BattleModels::CBattleModel::ListOfFiles() const noexcept
{
	set<string_view> ret{};

	for (auto&& Meshes : m_rgrgMeshGroup)
		ret.insert_range(Meshes | std::views::transform([](CMesh const& Mesh) noexcept -> string_view { return Mesh.m_szPath; }));

	for (auto&& [tex, norm, spr] : m_UnitTex | std::views::values)
	{
		ret.emplace(tex);
		ret.emplace(norm);
		ret.emplace(spr);
	}

	for (auto&& [tex, norm, spr] : m_AttachmentTex | std::views::values)
	{
		ret.emplace(tex);
		ret.emplace(norm);
		ret.emplace(spr);
	}

	ret.erase("");

	return ret;
}

set<fs::path> BattleModels::CBattleModel::ListOfFiles(fs::path const& DataPath) const noexcept
{
	set<fs::path> ret{};

	for (auto&& Meshes : m_rgrgMeshGroup)
		ret.insert_range(Meshes | std::views::transform([&](CMesh const& Mesh) noexcept { return DataPath / Mesh.m_szPath; }));

	for (auto&& files : m_UnitTex
		| std::views::values
		| std::views::transform([&](CTexture const& tex) noexcept { return tex.ListOfFiles(DataPath); })
		)
	{
		ret.insert_range(std::forward<set<fs::path>>(files));
	}

	// copy above.

	for (auto&& files : m_AttachmentTex
		| std::views::values
		| std::views::transform([&](CTexture const& tex) noexcept { return tex.ListOfFiles(DataPath); })
		)
	{
		ret.insert_range(std::forward<set<fs::path>>(files));
	}

	return ret;
}
