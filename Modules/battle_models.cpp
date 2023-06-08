/*
* Module:	Dec 06 2021
* Fix:		Jun 08 2023
*/

#include "battle_models.hpp"

#include <charconv>
#include <ranges>

#include <fmt/color.h>

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

void BattleModels::CFile::Reset(void) noexcept
{
	m_iSize = 0;

	if (m_pszBuffer)
		free(m_pszBuffer);

	m_pszBuffer = nullptr;

	m_pszCurPos = nullptr;
	m_pBufEnd = nullptr;
}

void BattleModels::CFile::Set(const char* pszFilePath) noexcept
{
	Reset();

	FILE* f = fopen(pszFilePath, "rb");
	if (!f)
		return;

	fseek(f, 0, SEEK_END);
	m_iSize = ftell(f);

	m_pszBuffer = (char*)calloc(m_iSize + 1, sizeof(char));
	fseek(f, 0, SEEK_SET);
	fread(m_pszBuffer, sizeof(char), m_iSize, f);

	fclose(f);

	m_pszCurPos = m_pszBuffer;
	m_pBufEnd = m_pszBuffer + m_iSize;	// Landed at the last '\0'
}

inline void BattleModels::CFile::Skip(int iCount) noexcept
{
	for (; m_pszCurPos != m_pBufEnd && iCount; ++m_pszCurPos)
	{
		if (*m_pszCurPos == ' ')
			--iCount;
	}

	SkipUntilNonspace();
}

inline void BattleModels::CFile::SkipUntilNonspace(void) noexcept
{
	while (!Eof() && std::isspace(*m_pszCurPos))
	{
		++m_pszCurPos;
	}
}

inline void BattleModels::CFile::Rewind(int iCount) noexcept
{
	++iCount;

	while (m_pszCurPos != m_pszBuffer && iCount)
	{
		--m_pszCurPos;

		if (*m_pszCurPos == ' ')
			--iCount;
	}

	SkipUntilNonspace();
}

inline void BattleModels::CFile::RewindUntilNonspace(void) noexcept
{
	while (m_pszCurPos != m_pszBuffer && std::isspace(*m_pszCurPos))
	{
		--m_pszCurPos;
	}
}

inline string BattleModels::CFile::ParseString(void) noexcept
{
	int iSize = UTIL_StrToNum<int>(ParseBySpace());
	if (iSize <= 0)
		return "";

	string ret;
	ret.reserve(iSize + 1);

	for (; m_pszCurPos != m_pBufEnd && iSize; ++m_pszCurPos, --iSize)
	{
		ret.push_back(*m_pszCurPos);
	}

	SkipUntilNonspace();
	return ret;
}

inline string BattleModels::CFile::ReadN(int iCount) noexcept
{
	string ret;

	for (; m_pszCurPos != m_pBufEnd && iCount; ++m_pszCurPos, --iCount)
	{
		ret.push_back(*m_pszCurPos);
	}

	return ret;
}

inline string BattleModels::CFile::ParseBySpace(void) noexcept
{
	string ret;

	for (char* p = m_pszCurPos; m_pszCurPos != m_pBufEnd; ++m_pszCurPos)
	{
		if (*m_pszCurPos == ' ')
			break;

		ret.push_back(*m_pszCurPos);
	}

	SkipUntilNonspace();
	return ret;
}

inline bool BattleModels::CFile::Eof(void) const noexcept
{
	return m_pszCurPos == m_pBufEnd;
}

inline void BattleModels::CFile::Seek(int iOffset, int iMode) noexcept
{
	switch (iMode)
	{
	case SEEK_SET:
		m_pszCurPos = m_pszBuffer;
		goto LAB_HANDLE_OFS;

	case SEEK_END:
		m_pszCurPos = m_pBufEnd;
		[[fallthrough]];

	case SEEK_CUR:
	LAB_HANDLE_OFS:;
		if (iOffset > 0)
			Skip(iOffset);
		else if (iOffset < 0)
			Rewind(-iOffset);

	default:
		break;
	}
}

void BattleModels::CFile::Initialize(void) noexcept
{
	Seek(0, SEEK_SET);
	Skip(7);

	m_iTotalModels = UTIL_StrToNum<size_t>(ParseBySpace());
	m_rgBattleModels.clear();
	m_rgBattleModels.reserve(m_iTotalModels);

	Skip(2);
	if (ParseString() == "blank")
		Skip(39);
	else
	{
		Rewind(2);	// Going back so that ParseString() reads therefore we can properly get the name.
		Parse<true>();
	}

	for (size_t i = 0; i < m_iTotalModels; ++i)
		Parse();
}

bool BattleModels::CFile::Sanity(void) const noexcept	// #UNDONE
{
	for (const auto& BattleModel : m_rgBattleModels)
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

	for (const auto& BattleModel : m_rgBattleModels)
	{
		fmt::print(f, "{}", Serialize(BattleModel));
	}
}

size_t BattleModels::CFile::Count(void) const noexcept
{
	size_t ret = 0;

	for (const auto& BattleModel : m_rgBattleModels)
	{
		if (!BattleModel.m_szName.empty())
			++ret;
	}

	return ret;
}

template <bool bSpecial>
inline void BattleModels::CFile::Parse(void) noexcept
{
	if (Eof() || m_rgBattleModels.size() >= m_iTotalModels)
		return;

	// Mark for special if this version had been called.
	if constexpr (bSpecial)
		m_bSpecialFirst = true;

	// Pre-allocate
	m_rgBattleModels.emplace_back();
	CBattleModel& BattleModel = m_rgBattleModels.back();

	// Region 1: name
	BattleModel.m_szName = ParseString();

	// Region 2: mesh
	if constexpr (!bSpecial)
	{
		BattleModel.m_rgrgMeshGroup.resize(UTIL_StrToNum<size_t>(ParseBySpace()));

		for (auto& rgMeshGroup : BattleModel.m_rgrgMeshGroup)
		{
			rgMeshGroup.resize(UTIL_StrToNum<size_t>(ParseBySpace()));

			for (auto& Mesh : rgMeshGroup)
			{
				Mesh.m_szPath.reserve(UTIL_StrToNum<size_t>(ParseBySpace()));
				Mesh.m_szPath = ParseBySpace();
				Mesh.m_iDistance = UTIL_StrToNum<size_t>(ParseBySpace());
			}
		}
	}
	else
	{
		BattleModel.m_rgrgMeshGroup.resize(1);	// For the "mount_pony", always be one.
		Skip(3);

		auto& rgMeshGroup = BattleModel.m_rgrgMeshGroup.front();
		rgMeshGroup.resize(UTIL_StrToNum<size_t>(ParseBySpace()));
		Skip(2);

		for (auto& Mesh : rgMeshGroup)
		{
			Mesh.m_szPath.reserve(UTIL_StrToNum<size_t>(ParseBySpace()));
			Mesh.m_szPath = ParseBySpace();
			Mesh.m_iDistance = UTIL_StrToNum<size_t>(ParseBySpace());
		}
	}

	// Region 3: unit texture
	if constexpr (bSpecial)	// It's like "0 0 28 0 0 " for "mount_pony"
		Skip(2);

	size_t iMaxSize = UTIL_StrToNum<size_t>(ParseBySpace());
	BattleModel.m_UnitTex.reserve(iMaxSize);

	if constexpr (bSpecial)
		Skip(2);

	for (size_t i = 0; i < iMaxSize; ++i)
	{
		//CTexture& Tex = BattleModel.m_UnitTex[ParseString()];
		auto& [szCountry, Tex] = BattleModel.m_UnitTex.emplace_back();

		szCountry = ParseString();
		Tex.m_szTex = ParseString();
		Tex.m_szNorm = ParseString();
		Tex.m_szSprite = ParseString();
	}

	// Region 4: attachment texture
	iMaxSize = UTIL_StrToNum<size_t>(ParseBySpace());
	BattleModel.m_AttachmentTex.reserve(iMaxSize);

	for (size_t i = 0; i < iMaxSize; ++i)
	{
		//CTexture& Tex = BattleModel.m_AttachmentTex[ParseString()];
		auto& [szCountry, Tex] = BattleModel.m_AttachmentTex.emplace_back();

		szCountry = ParseString();
		Tex.m_szTex = ParseString();
		Tex.m_szNorm = ParseString();
		Tex.m_szSprite = ParseString();
	}

	// Region 5: Stances
	if constexpr (bSpecial)
		Skip(2);

	BattleModel.m_rgStances.resize(UTIL_StrToNum<size_t>(ParseBySpace()));

	if constexpr (bSpecial)
		Skip(2);

	for (auto& Stance : BattleModel.m_rgStances)
	{
		Stance.m_Mount.m_szType = ParseString();
		Stance.m_Mount.m_szWpn1 = ParseString();
		Stance.m_Mount.m_szWpn2 = ParseString();

		Stance.m_rgszPrim.resize(UTIL_StrToNum<size_t>(ParseBySpace()));
		for (auto& Eqp : Stance.m_rgszPrim)
			Eqp = ParseString();

		Stance.m_rgszSec.resize(UTIL_StrToNum<size_t>(ParseBySpace()));
		for (auto& Eqp : Stance.m_rgszSec)
			Eqp = ParseString();
	}

	if constexpr (bSpecial)
		Skip(2);

	// Region 6: Unknow numbers.
	BattleModel.m_iMysteryNum = UTIL_StrToNum<int32_t>(ParseBySpace());
	BattleModel.m_MysteryVector1[0] = UTIL_StrToNum<float>(ParseBySpace());
	BattleModel.m_MysteryVector1[1] = UTIL_StrToNum<float>(ParseBySpace());
	BattleModel.m_MysteryVector1[2] = UTIL_StrToNum<float>(ParseBySpace());
	BattleModel.m_MysteryVector2[0] = UTIL_StrToNum<float>(ParseBySpace());
	BattleModel.m_MysteryVector2[1] = UTIL_StrToNum<float>(ParseBySpace());
	BattleModel.m_MysteryVector2[2] = UTIL_StrToNum<float>(ParseBySpace());

	if constexpr (bSpecial)
		Skip(2);
}
