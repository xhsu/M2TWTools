module;

#include <array>
#include <filesystem>
#include <format>
#include <iostream>
#include <list>
#include <string>

#include <cassert>
#include <cstdio>

export module idx;

import UtlKeyValues;
import UtlString;
import UtlWinConsole;

using std::array;
using std::list;
using std::string;

using namespace std::string_literals;
using namespace std::string_view_literals;

using uint16 = unsigned __int16;
using uint32 = unsigned __int32;

namespace fs = std::filesystem;

export enum PackType_e : uint16
{
	ANIM_PACK,
	EVT_PACK,
	SKEL_PACK,
	SND_PACK,

	UNKNOWN_PACK,
};

export struct fileinfo_t
{
	fileinfo_t() noexcept { if (!m_pBuf) [[unlikely]] m_pBuf = malloc(1); }
	virtual ~fileinfo_t() noexcept { }

	string m_szPath;
	uint32 m_iOffset { 0 };
	uint32 m_iSize { 0 };

	virtual void Read(FILE* pIdxFile) noexcept = 0;
	virtual void Read(ValveKeyValues* pKvs) noexcept {}	// #FIXME
	virtual string Query(void) const noexcept = 0;
	virtual void Extract(FILE* pDatFile) const noexcept
	{
		fs::path hPath = m_szPath;

		if (hPath.has_parent_path() && !fs::exists(hPath.parent_path()))
			fs::create_directories(hPath.parent_path());

		FILE* pfOut = fopen(m_szPath.c_str(), "wb");
		m_pBuf = realloc(m_pBuf, m_iSize);

		if (!pfOut) [[unlikely]]
		{
			cout_r() << "Unable to write to file: " << m_szPath << '\n';
			return;
		}
		else if (!m_pBuf) [[unlikely]]
		{
			cout_r() << "Memory Error: Fail to allocate memory.\n" << gray_text << "[info] Requesting block size: " << m_iSize << '\n';
			return;
		}

		fseek(pDatFile, m_iOffset, SEEK_SET);
		fread(m_pBuf, 1, m_iSize, pDatFile);
		fwrite(m_pBuf, 1, m_iSize, pfOut);

		fclose(pfOut);
	}
	//virtual void WriteIdx(FILE* pIdxFile) noexcept = 0;
	virtual void WriteDat(FILE* pDatFile) noexcept
	{
		FILE* pf = fopen(m_szPath.c_str(), "rb");
		if (!pf) [[unlikely]]
		{
			cout_r() << "Unable to open file: " << m_szPath << '\n';
			return;
		}

		m_iOffset = ftell(pDatFile);

		fseek(pf, 0, SEEK_END);
		m_iSize = ftell(pf);

		m_pBuf = realloc(m_pBuf, m_iSize);
		fseek(pf, 0, SEEK_SET);
		fread(m_pBuf, 1, m_iSize, pf);
		fwrite(m_pBuf, 1, m_iSize, pDatFile);

		fclose(pf);
	}
	virtual void Write(FILE* pIdxFile, FILE* pDatFile) noexcept {};
	virtual void Write(ValveKeyValues* pField) const noexcept {};	// #FIXME should be pure virtual function

protected:
	static inline void* m_pBuf = nullptr;
};

export struct AnimFile_t : public fileinfo_t
{
	float m_flScale { 1.0f };
	uint16 m_iFrameCounts { 0 };
	uint16 m_iBoneCounts { 0 };
	__int8 m_iType { 0 };

	void Read(FILE* pIdxFile) noexcept override
	{
		uint32 iPathSize = 0;
		fread(&iPathSize, sizeof(uint32), 1, pIdxFile);

		fread(&m_iOffset, sizeof(uint32), 1, pIdxFile);
		fread(&m_iSize, sizeof(uint32), 1, pIdxFile);
		fread(&m_flScale, sizeof(float), 1, pIdxFile);
		fread(&m_iFrameCounts, sizeof(uint16), 1, pIdxFile);
		fread(&m_iBoneCounts, sizeof(uint16), 1, pIdxFile);
		fread(&m_iType, sizeof(__int8), 1, pIdxFile);

		m_szPath.resize(iPathSize - 8);
		fread(m_szPath.data(), sizeof(char), iPathSize - 9, pIdxFile);
	}

	void Read(ValveKeyValues* pKvs) noexcept
	{
		m_szPath = pKvs->GetName();

		m_flScale = pKvs->GetValue<float>("Scale");
		m_iFrameCounts = pKvs->GetValue<uint16>("FrameCounts");
		m_iBoneCounts = pKvs->GetValue<uint16>("BoneCounts");
		m_iType = pKvs->GetValue<__int8>("Type");
	}

	string Query(void) const noexcept
	{
		auto iPos = m_szPath.find_last_of("/\\");

		return std::format(
			"[{}] Size: {}, Ofs: {}, Scale: {}, Frames: {}, Bones: {}, Type: {:d}\n",
			iPos == string::npos ? m_szPath : m_szPath.substr(iPos + 1),
			m_iSize,
			m_iOffset,
			m_flScale,
			m_iFrameCounts,
			m_iBoneCounts,
			m_iType
		);
	}

	void Write(FILE* pIdxFile, FILE* pDatFile) noexcept override
	{
		WriteDat(pDatFile);

		m_iFrameCounts = ((short*)m_pBuf)[0];
		m_iBoneCounts = ((short*)m_pBuf)[1];
		m_iType = ((__int8*)m_pBuf)[4];

		// Not used anymore. This should be handled by engine.
		/*if (m_flScale != 1.0f && m_iType == 1)
		{
			float* pStart = (float*)(size_t(m_pBuf) + (m_iFrameCounts * m_iBoneCounts * 16) + 5);
			int iLoop = m_iFrameCounts * 3 * 2 + (4 * ((m_iFrameCounts - 1) / 2)) + 8;

			for (int i = 0; i < iLoop; ++i)
				pStart[i] *= m_flScale;
		}*/

		const uint32 len_plus_balabala = m_szPath.length() + 9;

		fwrite(&len_plus_balabala, sizeof(uint32), 1, pIdxFile);
		fwrite(&m_iOffset, sizeof(uint32), 1, pIdxFile);
		fwrite(&m_iSize, sizeof(uint32), 1, pIdxFile);
		fwrite(&m_flScale, sizeof(float), 1, pIdxFile);
		fwrite(&m_iFrameCounts, sizeof(uint16), 1, pIdxFile);
		fwrite(&m_iBoneCounts, sizeof(uint16), 1, pIdxFile);
		fwrite(&m_iType, sizeof(__int8), 1, pIdxFile);
		fwrite(m_szPath.data(), sizeof(char), m_szPath.length(), pIdxFile);
	}

	void Write(ValveKeyValues* pField) const noexcept override
	{
		auto p = pField->CreateEntry(m_szPath.c_str());

		p->SetValue("Scale", m_flScale);
		p->SetValue("FrameCounts", m_iFrameCounts);
		p->SetValue("BoneCounts", m_iBoneCounts);
		p->SetValue<short>("Type", m_iType);
	}
};

export struct SkelFile_t : public fileinfo_t
{
	void Read(FILE* pIdxFile) noexcept override
	{
		uint32 iPathLength = 0;
		fread(&iPathLength, sizeof(uint32), 1, pIdxFile);

		fread(&m_iOffset, sizeof(uint32), 1, pIdxFile);
		fread(&m_iSize, sizeof(uint32), 1, pIdxFile);

		m_szPath.resize(iPathLength + 1);
		fread(m_szPath.data(), sizeof(char), iPathLength, pIdxFile);
	}

	string Query(void) const noexcept override
	{
		auto iPos = m_szPath.find_last_of("/\\");

		return std::format(
			"[{}] Size: {}, Ofs: {}\n",
			iPos == string::npos ? m_szPath : m_szPath.substr(iPos + 1),
			m_iSize,
			m_iOffset
		);
	}

	void Write(FILE* pIdxFile, FILE* pDatFile) noexcept override
	{
		WriteDat(pDatFile);

		const uint32 iPathLength = m_szPath.length();
		fwrite(&iPathLength, sizeof(uint32), 1, pIdxFile);

		fwrite(&m_iOffset, sizeof(uint32), 1, pIdxFile);
		fwrite(&m_iSize, sizeof(uint32), 1, pIdxFile);
		fwrite(m_szPath.data(), sizeof(char), iPathLength, pIdxFile);
	}
};

export struct SoundFile_t : public fileinfo_t
{
	static inline constexpr uint32 STRDELIM = 3452816640u;	// The last 8 bit of this number is 0000 0000. Is this some sort of '\0' replacement?

	array<int, 4> m_rgiUnknown { 0, 0, 0, 0 };

	void Read(FILE* pIdxFile) noexcept override
	{
		fread(&m_iOffset, sizeof(uint32), 1, pIdxFile);
		fread(&m_iSize, sizeof(uint32), 1, pIdxFile);
		fread(m_rgiUnknown.data(), sizeof(int), 4, pIdxFile);

		// We have no length info this time.
		m_szPath.clear();
		do
		{
			m_szPath.push_back(fgetc(pIdxFile));
		}
		while (m_szPath.back() != '\0');

		fseek(pIdxFile, 3, SEEK_CUR);
	}

	string Query(void) const noexcept override
	{
		auto iPos = m_szPath.find_last_of("/\\");

		return std::format(
			"[{}] Size: {}, Ofs: {}, rgUnknown: [{}, {}, {}, {}]\n",
			iPos == string::npos ? m_szPath : m_szPath.substr(iPos + 1),
			m_iSize,
			m_iOffset,
			m_rgiUnknown[0], m_rgiUnknown[1], m_rgiUnknown[2], m_rgiUnknown[3]
		);
	}

	void Write(FILE* pIdxFile, FILE* pDatFile) noexcept override
	{
		WriteDat(pDatFile);

		if (stristr(m_szPath.c_str(), ".mp3"))
		{
			m_rgiUnknown = { 0, 0, 0, 13 };
		}
		else
		{
			int* iptr = (int*)m_pBuf;
			while (*iptr != 544501094 /* fmt */ && iptr < (int*)(size_t(m_pBuf) + m_iSize - 16))
				iptr++;

			m_rgiUnknown[0] = *(iptr + 3);
			m_rgiUnknown[1] = 16;
			m_rgiUnknown[2] = *(((short*)(iptr)) + 5);
			m_rgiUnknown[3] = *(((short*)(iptr)) + 4) == 1 ? 1 : 2; /*compression?*/
		}

		fwrite(&m_iOffset, sizeof(uint32), 1, pIdxFile);
		fwrite(&m_iSize, sizeof(uint32), 1, pIdxFile);
		fwrite(m_rgiUnknown.data(), sizeof(int), 4, pIdxFile);
		fwrite(m_szPath.data(), sizeof(char), m_szPath.length(), pIdxFile);	// Excluding '\0'
		fwrite(&STRDELIM, sizeof(uint32), 1, pIdxFile);
	}
};

export struct pack_t
{
	// Open without loading
	pack_t(void) noexcept {}
	// Open a exisiting file
	explicit pack_t(const char* pszStem) noexcept : m_szNameStem(pszStem)
	{
		m_fpIdx = fopen((pszStem + ".idx"s).c_str(), "rb");
		m_fpDat = fopen((pszStem + ".dat"s).c_str(), "rb");

#ifdef _DEBUG
		assert(m_fpDat && m_fpIdx);
#else
		if (!m_fpIdx)
			cout_r() << '\"' << pszStem << ".idx\" no found.\n";
		if (!m_fpDat)
			cout_r() << '\"' << pszStem << ".dat\" no found.\n";
#endif
		char szIdentifier[9] = { '\0' };
		fread(szIdentifier, sizeof(char), _countof(szIdentifier) - 1, m_fpIdx);

		for (size_t i = 0; i < m_rgszIdentifierStr.size(); ++i)
		{
			if (!_strnicmp(szIdentifier, m_rgszIdentifierStr[i], 8))
			{
				m_Type = static_cast<PackType_e>(i);
				break;
			}
		}

		if (m_Type == UNKNOWN_PACK)
			cout_w() << std::format("Unknown pack type: string '{}' found as identifier.\n", szIdentifier);

		fseek(m_fpIdx, m_Type == SND_PACK ? 12 : 16, SEEK_SET);
		fread(&m_iFileCounts, sizeof(uint32), 1, m_fpIdx);

		if (m_Type == SND_PACK)
			fseek(m_fpIdx, 24, SEEK_SET);

		switch (m_Type)
		{
		case ANIM_PACK:
			for (size_t i = 0; i < m_iFileCounts; ++i)
			{
				m_rgpFileInfo.push_back(new AnimFile_t);
				m_rgpFileInfo.back()->Read(m_fpIdx);
			}

			break;

		case EVT_PACK:
			cout_gold() << "[info] " << m_rgszIdentifierStr[EVT_PACK] << " is no longer supported. You can have it generated automatically by game engine.\n" << silver_text << "[info] http://wiki.twcenter.net/index.php?title=Sound_text_files" << std::endl;
			break;

		case SKEL_PACK:
			for (size_t i = 0; i < m_iFileCounts; ++i)
			{
				m_rgpFileInfo.push_back(new SkelFile_t);
				m_rgpFileInfo.back()->Read(m_fpIdx);
			}

			break;

		case SND_PACK:
			for (size_t i = 0; i < m_iFileCounts; ++i)
			{
				m_rgpFileInfo.push_back(new SoundFile_t);
				m_rgpFileInfo.back()->Read(m_fpIdx);
			}

			break;

		case UNKNOWN_PACK:
		default:
			break;
		}
	}
	virtual ~pack_t(void) noexcept
	{
		CloseFile();

		for (auto& p : m_rgpFileInfo)
			delete p;

		m_rgpFileInfo.clear();
	}

	void CloseFile(void) noexcept
	{
		if (m_fpDat)
			fclose(m_fpDat);
		if (m_fpIdx)
			fclose(m_fpIdx);
	}

	void Query(void) const noexcept
	{
		cout_w() << m_szNameStem << '[' << m_rgszIdentifierStr[m_Type] << "]: " << m_iFileCounts << '\n';

		for (const auto& p : m_rgpFileInfo)
		{
			cout_gray() << p->Query();
		}
	}

	void Extract(void) const noexcept
	{
		size_t iMax = m_rgpFileInfo.size(), iCur = 0;

		for (const auto& p : m_rgpFileInfo)
		{
			cout_gray() << std::format("[{}/{}] Extracting '{}'...\n", ++iCur, iMax, p->m_szPath);

			p->Extract(m_fpDat);
		}
	}

	void Pack(void) noexcept
	{
		if (m_szNameStem.empty()) [[unlikely]]
		{
			cout_gold() << "[Error] Stem name not set yet.\n";
			return;
		}

		m_fpIdx = fopen((m_szNameStem + ".idx"s).c_str(), "wb");
		m_fpDat = fopen((m_szNameStem + ".dat"s).c_str(), "wb");

		size_t iMax = m_rgpFileInfo.size(), iCur = 0;

		for (auto& p : m_rgpFileInfo)
		{
			cout_gray() << std::format("[{}/{}] Packing '{}'...\n", ++iCur, iMax, p->m_szPath);

			p->Write(m_fpIdx, m_fpDat);
		}
	}

	static inline constexpr array<const char*, 4> m_rgszIdentifierStr =
	{
		"ANIM.PACK",
		"EVT.PACK",
		"SKEL.PACK",
		"SND.PACK",
	};

	PackType_e m_Type { UNKNOWN_PACK };
	string m_szNameStem { "" };
	FILE* m_fpIdx { nullptr };
	FILE* m_fpDat { nullptr };
	uint32 m_iFileCounts { 0 };
	list<fileinfo_t*> m_rgpFileInfo {};
};
