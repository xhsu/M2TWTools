/* Copyright (C) 2004, 2005        Vercingetorix <vercingetorix11@gmail.com>
 * Copyright (C) 2011, 2012, 2013, 2014  The Europa Barbarorum Team <webmaster@europabarbarorum.com>
 * 
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either version 2 
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/

#ifndef IDX_H
#define IDX_H

/*magic numbers*/
constexpr char SSND[] = "SND.PACK";
constexpr char SANM[] = "ANIM.PACK";
constexpr char SSKL[] = "SKEL.PACK";
constexpr char SEVT[] = "EVT.PACK";
constexpr unsigned int STRDELIM = 3452816640u; 

#define MP3TAG 13


// C
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

// C++
#include <array>
#include <filesystem>
#include <list>
#include <string>

using std::array;
using std::list;
using std::string;
using std::filesystem::path;

struct idxSubFile
{
	idxSubFile() noexcept {}
	virtual ~idxSubFile() noexcept { Clean(); }

	virtual bool Read(FILE*) noexcept = 0;
	virtual bool Write(FILE*) noexcept = 0;
	virtual bool GatherInfo() noexcept = 0;
	virtual bool PrepareDataOut() noexcept { return true; }
	virtual void ParseFilename(const char*) noexcept;
	virtual void PrintFilename() noexcept;
	virtual bool WriteFile(FILE*, FILE*, const char*) noexcept;

	void Clean() noexcept;
	bool Extract(FILE*) noexcept;
	bool ExtractFile(FILE*, FILE*) noexcept;
	void SetFilename(const char*) noexcept;

	size_t beginFileOffset{ 0 };	//where the file starts
	_off_t fileSize{ -1 };	//how big it is...	 
	string filename{ "" };
	char* buffer{ nullptr };
};

struct idxFile
{
	idxFile() noexcept {};
	virtual ~idxFile() noexcept { if (subFiles) delete subFiles; }

	bool Write(const char* pszFile, const list<string>& rgszFiles) noexcept;
	bool Extract(const char*, bool) noexcept;

private:
	virtual void WriteHeader() noexcept = 0;
	virtual bool ReadHeader() noexcept = 0;
	virtual void Init() noexcept = 0;
	virtual void Allocate() noexcept = 0;

	bool OpenFiles(const string&, const char*) noexcept;
	void CloseFiles() noexcept;

public:
	const char* filetype{ nullptr };
	unsigned int fileVersion{ 0 };
	size_t numFiles{ 0 };
	idxSubFile* subFiles{ nullptr };
	FILE* fp{ nullptr }, * fp2{ nullptr };
};

struct sidxSubFile : public idxSubFile
{
	bool Read(FILE*) noexcept override;
	bool Write(FILE*) noexcept override;
	bool GatherInfo() noexcept override;

private:
	array<int, 4> control{ 0, 0, 0, 0 };	 //contains info about sound file,
};
   
struct sidxFile : public idxFile
{
	void Init() noexcept override;

private:
	void WriteHeader() noexcept override;
	bool ReadHeader() noexcept override;
	void Allocate() noexcept override;

};

struct sidxFile2 : public sidxFile
{
	void Init() noexcept override;
};

struct aidxSubFile : public idxSubFile
{
	aidxSubFile(bool allowScaling) noexcept : enableScaling(allowScaling) {};

	bool Read(FILE*) noexcept override;
	bool Write(FILE*) noexcept override;
	bool GatherInfo() noexcept override;
	bool PrepareDataOut() noexcept override;
	void PrintFilename() noexcept override;
	bool Scale() noexcept;

private:
	bool enableScaling { false };

public:
	size_t	entrySize { 0 };
	float m_flScale { 1.0f }; //deprecated??
	short numFrames { 0 };
	short numBones { 0 };
	char  type { '\0' };

};
   
struct aidxFile : public idxFile
{
	void Init() noexcept override;

private:
	void WriteHeader() noexcept override;
	bool ReadHeader() noexcept override;
	void Allocate() noexcept override;

};

struct aidxFile2 : public aidxFile
{
	void Init() noexcept override;

private:
	void Allocate() noexcept override;
};

struct kidxSubFile : public idxSubFile
{
	bool Read(FILE*) noexcept override;
	bool Write(FILE*) noexcept override;
	bool GatherInfo() noexcept override;
	void ParseFilename(const char*) noexcept override;

	size_t len{ 0 };
};
   
struct kidxFile : public idxFile
{
	void Init() noexcept override;

private:
	void WriteHeader() noexcept override;
	bool ReadHeader() noexcept override;
	void Allocate() noexcept override;

};

struct kidxFile2 : public kidxFile
{
	void Init() noexcept override;
};

struct eidxFile : public idxFile
{
	void Init() noexcept override;

private:
	void WriteHeader() noexcept override;
	bool ReadHeader() noexcept override;
	void Allocate() noexcept override;
};

struct eidxFile2 : public eidxFile
{
	void Init() noexcept override;
};

struct eidxSubFile : public idxSubFile
{
	bool Read(FILE*) noexcept override;
	bool Write(FILE*) noexcept override;
	bool GatherInfo() noexcept override { return buffer != nullptr; };
	void ParseFilename(const char*) noexcept override;

private:
	int num { 0 }, frameId { 0 };
	bool tainted { false };
};
#endif
