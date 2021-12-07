/*
* Dec 06 2021
*/

module;

#include <fstream>	// std::ofstream
#include <iomanip>	// std::quoted
#include <iostream>	// std::cout
#include <string>
#include <unordered_map>
#include <vector>

#include <cassert>
#include <cstdio>	// fopen
#include <cstring>	// isspace

export module battle_models;

using std::string;
using std::unordered_map;
using std::vector;

import UtlConcepts;
import UtlLinearAlgebra;
import UtlString;

struct Mesh_t
{
	std::string m_szPath{ "" };
	size_t m_iDistance{ 0U };
};

struct Texture_t
{
	std::string m_szTex{ "" };
	std::string m_szNorm{ "" };
	std::string m_szSprite{ "" };
};

struct Mount_t
{
	std::string m_szType{ "" };
	std::string m_szWpn1{ "" };
	std::string m_szWpn2{ "" };
};

struct Stance_t
{
	Mount_t m_Mount{};
	vector<string> m_rgszPrim{};
	vector<string> m_rgszSec{};
};

export struct BattleModel_t
{
	string m_szName{ "" };
	vector<vector<Mesh_t>> m_rgrgMeshGroup{};
	unordered_map<string, Texture_t> m_UnitTex{};
	unordered_map<string, Texture_t> m_AttachmentTex{};
	vector<Stance_t> m_rgStances{};
	int m_iMysteryNum{ 0 };
	Vector m_MysteryVector1;
	Vector m_MysteryVector2;
};

template<typename T, typename U> requires(std::is_same_v<std::basic_ostream<T, std::char_traits<T>>, U> || std::is_same_v<std::basic_ofstream<T, std::char_traits<T>>, U>)
void SpecialStringPrint(U& out, const std::basic_string<T, std::char_traits<T>, std::allocator<T>>& sz) noexcept
{
	out << sz.length() << ' ';

	if (!sz.empty())
		out << sz << ' ';

	if constexpr (std::is_same_v<std::basic_ostream<T, std::char_traits<T>>, U>)
		out << '\n';
	else if constexpr (std::is_same_v<std::basic_ofstream<T, std::char_traits<T>>, U>)
		out << std::endl;
	else
		static_assert(std::false_type::value, "Something went wrong.");
}

export template<typename T> requires(OStream<T>)
T& operator<< (T& lhs, const Mesh_t& rhs) noexcept
{
	using namespace std;

	lhs << rhs.m_szPath.size() << ' ' << rhs.m_szPath << ' ' << rhs.m_iDistance << ' ' << endl;

	return lhs;
}

export template<typename T> requires(OStream<T>)
T& operator<< (T& lhs, const Texture_t& rhs) noexcept
{
	using namespace std;

	SpecialStringPrint(lhs, rhs.m_szTex);
	SpecialStringPrint(lhs, rhs.m_szNorm);
	SpecialStringPrint(lhs, rhs.m_szSprite);

	return lhs;
}

export template<typename T> requires(OStream<T>)
T& operator<< (T& lhs, const Mount_t& rhs) noexcept
{
	using namespace std;

	SpecialStringPrint(lhs, rhs.m_szType);
	SpecialStringPrint(lhs, rhs.m_szWpn1);
	SpecialStringPrint(lhs, rhs.m_szWpn2);

	return lhs;
}

export template<typename T> requires(OStream<T>)
T& operator<< (T& lhs, const Stance_t& rhs) noexcept
{
	using namespace std;

	lhs << rhs.m_Mount;

	lhs << rhs.m_rgszPrim.size() << ' ' << endl;
	for (const auto& Eqp : rhs.m_rgszPrim)
		SpecialStringPrint(lhs, Eqp);

	lhs << rhs.m_rgszSec.size() << ' ' << endl;
	for (const auto& Eqp : rhs.m_rgszSec)
		SpecialStringPrint(lhs, Eqp);

	return lhs;
}

export template<typename T> requires(OStream<T>)
T& operator<< (T& lhs, const BattleModel_t& rhs) noexcept
{
	using namespace std;

	// Region 1: name
	lhs << rhs.m_szName.length() << ' ' << rhs.m_szName << ' ' << endl;

	// Region 2: mesh
	lhs << rhs.m_rgrgMeshGroup.size() << ' ' << endl;
	for (const auto& rgMesh : rhs.m_rgrgMeshGroup)
	{
		lhs << rgMesh.size() << ' ' << endl;

		for (const auto& Mesh : rgMesh)
			lhs << Mesh;
	}

	// Region 3: unit texture
	lhs << rhs.m_UnitTex.size() << ' ' << endl;
	for (const auto& [szCountry, Texture] : rhs.m_UnitTex)
	{
		lhs << szCountry.length() << ' ' << szCountry << ' ' << endl;
		lhs << Texture;
	}

	// Region 4: attachments texture
	lhs << rhs.m_AttachmentTex.size() << ' ' << endl;
	for (const auto& [szCountry, Texture] : rhs.m_AttachmentTex)
	{
		lhs << szCountry.length() << ' ' << szCountry << ' ' << endl;
		lhs << Texture;
	}

	// Region 5: Stances
	lhs << rhs.m_rgStances.size() << ' ' << endl;
	for (const auto& Stance : rhs.m_rgStances)
		lhs << Stance;

	// Region 6: Unknow numbers.
	lhs << rhs.m_iMysteryNum << ' '
		<< rhs.m_MysteryVector1.x << ' ' << rhs.m_MysteryVector1.y << ' ' << rhs.m_MysteryVector1.z << ' '
		<< rhs.m_MysteryVector2.x << ' ' << rhs.m_MysteryVector2.y << ' ' << rhs.m_MysteryVector2.z << ' '
		<< endl;

	return lhs;
}

export struct modeldb_file_t
{
private:
	long m_iSize = 0;
	char* m_pszBuffer = nullptr;
	char* m_pszCurPos = nullptr;
	char* m_pBufEnd = nullptr;

	void Reset(void) noexcept
	{
		m_iSize = 0;

		if (m_pszBuffer)
			free(m_pszBuffer);
		m_pszBuffer = nullptr;

		m_pszCurPos = nullptr;
		m_pBufEnd = nullptr;
	}
	void Set(const char* pszFilePath = "battle_models.modeldb") noexcept
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
	void Skip(int iCount = 1) noexcept
	{
		for (; m_pszCurPos != m_pBufEnd && iCount; ++m_pszCurPos)
		{
			if (*m_pszCurPos == ' ')
				--iCount;
		}

		SkipUntilNonspace();
	}
	void SkipUntilNonspace(void) noexcept
	{
		while (!Eof() && std::isspace(*m_pszCurPos))
		{
			++m_pszCurPos;
		}
	}
	void Rewind(int iCount = 1) noexcept
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
	void RewindUntilNonspace(void) noexcept
	{
		while (m_pszCurPos != m_pszBuffer && std::isspace(*m_pszCurPos))
		{
			--m_pszCurPos;
		}
	}
	string ParseString(void) noexcept
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
	string ReadN(int iCount) noexcept
	{
		string ret;

		for (; m_pszCurPos != m_pBufEnd && iCount; ++m_pszCurPos, --iCount)
		{
			ret.push_back(*m_pszCurPos);
		}

		return ret;
	}
	string ParseBySpace(void) noexcept
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
	bool Eof(void) const noexcept
	{
		return m_pszCurPos == m_pBufEnd;
	}
	void Seek(int iOffset, int iMode = SEEK_CUR) noexcept
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

public:
	bool m_bSpecialFirst = false;
	size_t m_iTotalModels = 0;
	vector<BattleModel_t> m_rgBattleModels{};

	modeldb_file_t(const char* pszFilePath) noexcept { Set(pszFilePath); }
	virtual ~modeldb_file_t(void) noexcept { Reset(); }

	void Initialize(void) noexcept
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
	template<bool bSpecial = false> void Parse(void) noexcept
	{
		if (Eof() || m_rgBattleModels.size() >= m_iTotalModels)
			return;

		// Mark for special if this version had been called.
		if constexpr (bSpecial)
			m_bSpecialFirst = true;

		// Pre-allocate
		m_rgBattleModels.emplace_back();
		BattleModel_t& BattleModel = m_rgBattleModels.back();

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

			auto& rgMeshGroup = BattleModel.m_rgrgMeshGroup[0];
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
			Texture_t& Tex = BattleModel.m_UnitTex[ParseString()];

			Tex.m_szTex = ParseString();
			Tex.m_szNorm = ParseString();
			Tex.m_szSprite = ParseString();
		}

		// Region 4: attachment texture
		iMaxSize = UTIL_StrToNum<size_t>(ParseBySpace());
		BattleModel.m_AttachmentTex.reserve(iMaxSize);

		for (size_t i = 0; i < iMaxSize; ++i)
		{
			Texture_t& Tex = BattleModel.m_AttachmentTex[ParseString()];

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
		BattleModel.m_iMysteryNum = UTIL_StrToNum<int>(ParseBySpace());
		BattleModel.m_MysteryVector1.x = UTIL_StrToNum<vec_t>(ParseBySpace());
		BattleModel.m_MysteryVector1.y = UTIL_StrToNum<vec_t>(ParseBySpace());
		BattleModel.m_MysteryVector1.z = UTIL_StrToNum<vec_t>(ParseBySpace());
		BattleModel.m_MysteryVector2.x = UTIL_StrToNum<vec_t>(ParseBySpace());
		BattleModel.m_MysteryVector2.y = UTIL_StrToNum<vec_t>(ParseBySpace());
		BattleModel.m_MysteryVector2.z = UTIL_StrToNum<vec_t>(ParseBySpace());

		if constexpr (bSpecial)
			Skip(2);
	}
	bool Sanity(void) const noexcept	// #UNDONE
	{
		for (const auto& BattleModel : m_rgBattleModels)
		{
			if (BattleModel.m_szName.empty())
			{
				std::cout << "Null model name found." << std::endl;
				continue;
			}

			for (const auto& [szCountry, Texture] : BattleModel.m_UnitTex)
			{
				if (!BattleModel.m_AttachmentTex.contains(szCountry))
				{
					std::cout << '[' << BattleModel.m_szName << ']' << std::quoted(szCountry) << " entry found in unit texture section but absent from attachment texture section." << std::endl;
					continue;
				}
			}
		}

		return true;
	}
	void Save(const char* pszFilePath = "battle_models.modeldb") const noexcept
	{
		std::ofstream f(pszFilePath, std::ios::binary | std::ios::out);
		if (!f)
			return;

		f << "22 serialization::archive 3 0 0 0 0 " << Count() + 1 << " 0 0 " << std::endl;
		f << "5 blank 0 0 0 1 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 " << std::endl;

		for (const auto& BattleModel : m_rgBattleModels)
		{
			f << BattleModel;
		}
	}
	size_t Count(void) const noexcept
	{
		size_t ret = 0;

		for (const auto& BattleModel : m_rgBattleModels)
		{
			if (!BattleModel.m_szName.empty())
				++ret;
		}

		return ret;
	}
};
