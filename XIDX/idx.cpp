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

#include <cctype>
#include <cmath>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>

#include "idx.h" 

#define IO_CHECK(a,b) if(io_check((a), (b))) { return false; }
#define IO_BUF_CHECK(a,b, buf, fp) if(io_buf_check((a), (b), buf, fp)) { buf = nullptr; return false; }

#define DAT_SUFFIX ".dat"
#define IDX_SUFFIX ".idx"

import UtlFileSystem;
import UtlString;
import UtlWinConsole;

namespace fs = std::filesystem;

using std::string;

void append_suffix(const char* filename, const char* suffix, int len, char* r) noexcept
{
	strncpy(r, filename, len);
	r[len] = '.';
	strcpy(&r[len + 1], suffix);
}

bool io_check(unsigned int cmpA, unsigned int cmpB) noexcept
{
	if (cmpA != cmpB)
	{
		fputs("I/O Error\n", stderr);
		return true;
	}

	return false;
}

bool io_buf_check(unsigned int cmpA, unsigned int cmpB, void* buf, FILE* fp) noexcept
{
	if (io_check(cmpA, cmpB))
	{
		free(buf);
		if (fp != nullptr)
			fclose(fp);

		return true;
	}
	return false;
}

void replace_suffix(const char* filename, const char* suffix, int len,char* r) noexcept
{
	return append_suffix(filename, suffix, len - strlen(suffix) - 1, r);
}

int basename_length(const char* name, int len) noexcept
{
	if (auto pstr = strrchr(name, '/'); pstr)
		return len - (pstr - name) - 1;
	else
		return len;
}

/*extracts the data path from a filename*/
auto getdpath(auto psz, bool bFixupSlashes, bool bFindDataPath) noexcept -> decltype(psz)
{
	if (bFixupSlashes)
		strrep(psz, '\\', '/');

	if (bFindDataPath)
	{
		for (int i = strlen(psz); i >= 0; i--)
			if (!strnicmp_c<"data/">(&psz[i]))
				return &psz[i];

		return nullptr;
	}
	else
		return psz;
}

/*class defs*/
void idxSubFile::SetFilename(const char* str) noexcept
{
	filename = str;
}

void idxSubFile::Clean() noexcept
{
	if (buffer != nullptr)
		free(buffer);

	buffer = nullptr;
}

void idxSubFile::ParseFilename(const char* fname) noexcept
{
	// make the path relative to data\\

	filename = fname;

	for (auto& c : filename)
		if (c == '\\')
			c = '/';

	auto iPos = filename.find("data/");
	if (iPos == filename.npos && filename.find_first_of('/') != filename.npos)
		cout_r() << "warning: file " << fname << " does not have \"data\" in path.\n";

	if (iPos >= 3 && !strnicmp_c<"bi/">(&filename[iPos - 3]))
		iPos -= 3;

	if (iPos && iPos != filename.npos)
		filename.erase(0, iPos);
}

void idxSubFile::PrintFilename() noexcept
{
	cout_w() << filename << '\n';
}    

bool idxSubFile::Extract(FILE* fp2) noexcept
{
	if (buffer)
	{
		free(buffer);
		buffer = nullptr;
	}

	buffer = (char*)calloc(fileSize, sizeof(char));
	if (!buffer)
		return false;

	fseek(fp2, beginFileOffset, SEEK_SET);

	IO_BUF_CHECK(fread(buffer, sizeof(char), fileSize, fp2), fileSize, buffer, NULL )
	
	fs::path hPath = filename;
	if (!fs::exists(hPath))
		fs::create_directories(hPath);

	FILE* fpout = fopen(filename.c_str(), "wb");
	if (!fpout)
	{
		free(buffer);
		buffer = nullptr;
		return false;
	}
	
	bool result = PrepareDataOut();
	if (result)
		IO_BUF_CHECK(fwrite(buffer, sizeof(char), fileSize, fpout), fileSize, buffer, fpout);

	free(buffer);
	buffer = nullptr;
	fclose(fpout);
	return result;
} 

	  
bool idxSubFile::ExtractFile(FILE* fp, FILE* fp2) noexcept
{
	Clean();

	if (fp)
	{
		bool result = Read(fp);
		if (result && fp2)
			return Extract(fp2);

		return result;
	}
	else
		return false;
}


bool idxSubFile::WriteFile(FILE* fp, FILE* fp2, const char* fname) noexcept
{
	FILE* fpin = fopen(fname, "rb");
	if (!fpin) [[unlikely]]
	{
		cout_r() << "Unable to open file: " << fname << '\n';
		return false;
	}

	Clean();
	fileSize = UTIL_GetFileSize(fname);
	if (fileSize == -1) [[unlikely]]
	{
		fclose(fpin);
		cout_r() << "Cannot stat: " << fname << '\n';
		return false;
	}

	buffer = (char*)calloc(fileSize, sizeof(char));
	if (!buffer)
	{
		fclose(fpin);
		return false;
	}

	IO_BUF_CHECK(fread(buffer, 1, fileSize, fpin), fileSize, buffer, fpin)
		fclose(fpin);

	beginFileOffset = ftell(fp2);

	if (GatherInfo() && Write(fp))
	{
		IO_BUF_CHECK(fwrite(buffer, sizeof(char), fileSize, fp2), fileSize, buffer, NULL)
			fflush(fp);
		fflush(fp2);
		free(buffer);
		buffer = nullptr;
		return true;
	}
	else
	{
		cout_r() << "Unable to pack file: " << fname << '\n';
		free(buffer);
		buffer = nullptr;
		return false;
	}
}

bool idxFile::OpenFiles(const string& szFileName, const char* mode ) noexcept
{
	fs::path hPath = szFileName;

	if (hPath.has_extension())
	{
		auto pos = szFileName.find_last_of('.');
		string s = szFileName.substr(0, pos);

		fp = fopen((s + IDX_SUFFIX).c_str(), mode);
		fp2 = fopen((s + DAT_SUFFIX).c_str(), mode);

		if (!fp)
			cout_r() << "Failed to open \"" << s << IDX_SUFFIX << "\".\n";
		if (!fp2)
			cout_r() << "Failed to open \"" << s << DAT_SUFFIX << "\".\n";
	}
	else
	{
		fp = fopen((szFileName + IDX_SUFFIX).c_str(), mode);
		fp2 = fopen((szFileName + DAT_SUFFIX).c_str(), mode);

		if (!fp)
			cout_r() << "Failed to open \"" << szFileName << IDX_SUFFIX << "\".\n";
		if (!fp2)
			cout_r() << "Failed to open \"" << szFileName << DAT_SUFFIX << "\".\n";
	}

	return fp && fp2;
}

void idxFile::CloseFiles() noexcept
{
	if (fp)
		fclose(fp);

	if (fp2)
		fclose(fp2);

	fp = fp2 = nullptr;
}

	  
bool idxFile::Write(const char* filename, const list<string>& rgszFiles) noexcept
{
	Init();
	if (!OpenFiles(filename, "wb"))
		return false;

	WriteHeader();
	Allocate();

	for (const auto& szFile : rgszFiles)
	{
		subFiles->ParseFilename(szFile.c_str());

		if (!subFiles->WriteFile(fp, fp2, szFile.c_str()))
			cout_r() << "Failed to pack file: " << szFile << '\n';
		else
			++numFiles;
	}

	WriteHeader();
	CloseFiles();
	return numFiles > 0;
}

bool idxFile::Extract(const char* filename, bool bListOnly) noexcept
{
	if (!OpenFiles(filename, "rb"))
		return false;

	if (!ReadHeader())
	{
		CloseFiles();
		return false;
	}

	Allocate();
	size_t success_count = 0;
	for (size_t i = 0; i < numFiles; i++)
	{
		if (!subFiles->ExtractFile(fp, bListOnly ? nullptr : fp2))
			cout_r() << "Error reading archived file '" << subFiles->filename << "' from pack: '" << filename << "'.\n";
		else
		{
			success_count++;
			if (bListOnly)
				subFiles->PrintFilename();
			else
				cout_gray() << "Extracting file " << i << '/' << numFiles << '\n';
		}
	}

	CloseFiles();
	return success_count == numFiles;
}

/*Sound sub idx file*/

bool sidxSubFile::Read(FILE* fp) noexcept
{
	if (!fp)
		return false;

	IO_CHECK(fread(&beginFileOffset, sizeof(int), 1, fp), 1)
		IO_CHECK(fread(&fileSize, sizeof(int), 1, fp), 1)
		IO_CHECK(fread(control.data(), sizeof(int), 4, fp), 4)

		/*read filename*/
		filename.clear();
	do
	{
		filename.push_back(fgetc(fp));
	}
	while (filename.back() != '\0');

	fseek(fp, 3, SEEK_CUR);
	return true;
}

bool sidxSubFile::Write(FILE* fp) noexcept
{
	if (fp == nullptr)
		return false;

	IO_CHECK(fwrite(&beginFileOffset, sizeof(int), 1, fp), 1)
		IO_CHECK(fwrite(&fileSize, sizeof(int), 1, fp), 1)
		IO_CHECK(fwrite(control.data(), sizeof(int), 4, fp), 4)

		IO_CHECK(fwrite(filename.c_str(), sizeof(char), filename.length(), fp), filename.length())	// Shouldn't this include '\0' as well?

		IO_CHECK(fwrite(&STRDELIM, sizeof(int), 1, fp), 1)

		return true;
}

bool sidxSubFile::GatherInfo() noexcept
{
	if (stristr(filename.c_str(), ".mp3"))
	{
		control[0] = control[1] = control[2] = 0;
		control[3] = MP3TAG;
	}
	else
	{
		int* iptr = (int*)buffer;
		while (*iptr != 544501094/*fmt */ && iptr < (int*)(buffer + fileSize - 16))
			iptr++;

		control[0] = *(iptr + 3);
		control[1] = 16;
		control[2] = *(((short*)(iptr)) + 5);
		control[3] = *(((short*)(iptr)) + 4) == 1 ? 1 : 2; /*compression?*/
	}
	return true;
}
  
  
/*Sound idx file*/
void sidxFile::Allocate() noexcept
{
	if (subFiles != nullptr)
		delete subFiles;

	subFiles = new sidxSubFile;
}

void sidxFile::Init() noexcept
{
	fileVersion = 4;
	filetype = SSND;
}

void sidxFile2::Init() noexcept
{
	fileVersion = 5;
	filetype = SSND;
}

bool sidxFile::ReadHeader() noexcept
{
	char checktype[_countof(SSND)] = { '\0' };
	IO_CHECK(fread(checktype , _countof(SSND) - 1 , 1 , fp) , 1)

		if (strcmp(checktype , SSND) != 0)
		{
			cout_r() << "File is not a sound idx archive.\n";
			return false;
		}

	fseek(fp , 12 , SEEK_SET);
	IO_CHECK(fread(&numFiles , sizeof(int) , 1 , fp) , 1)
		fseek(fp , 24 , SEEK_SET);
	return true;
}

void sidxFile::WriteHeader() noexcept
{
	constexpr int zero[2] = { 0, 0 };

	fseek(fp , 0 , SEEK_SET);
	fseek(fp2 , 0 , SEEK_SET);

	fwrite(filetype , sizeof(char) , strlen(filetype) , fp);
	fwrite(&fileVersion , sizeof(int) , 1 , fp);
	fwrite(&numFiles , sizeof(int) , 1 , fp);
	fwrite(zero , sizeof(int) , 2 , fp);

	fwrite(filetype , sizeof(char) , strlen(filetype) , fp2);
	fwrite(&fileVersion , sizeof(int) , 1 , fp2);
	fwrite(&numFiles , sizeof(int) , 1 , fp2);
	fwrite(zero , sizeof(int) , 2 , fp2);

	fflush(fp);
	fflush(fp2);
}

/*Animation sub idx file*/
void aidxSubFile::PrintFilename() noexcept
{
	if (m_flScale != 1.0f)
		cout_r() << filename << "[scale = " << m_flScale << "]\n";
	else
		__super::PrintFilename();
}
	
bool aidxSubFile::PrepareDataOut() noexcept
{
	if (enableScaling)
	{
		bool result;
		m_flScale = 1.0f / m_flScale;
		result = Scale();
		m_flScale = 1.0f / m_flScale;
		return result;
	}
	else
		return true;
}

bool aidxSubFile::Scale() noexcept
{
	if (!buffer)
		return false;

	if ((m_flScale != 1.0f) && (buffer[4] == 1))
	{
		float* start = (float*)&buffer[(numFrames * numBones * 16) + 5];
		int loop = numFrames * 3 * 2 + (4 * ((numFrames - 1) / 2)) + 8;

		for (int i = 0; i < loop; i++)
			start[i] *= m_flScale;
	}

	return true;
}
	
bool aidxSubFile::Read(FILE* fp) noexcept
{
	if (!fp)
		return false;

	IO_CHECK(fread(&entrySize , sizeof(int) , 1 , fp) , 1)
		IO_CHECK(fread(&beginFileOffset , sizeof(int) , 1 , fp) , 1)
		IO_CHECK(fread(&fileSize , sizeof(int) , 1 , fp) , 1)
		IO_CHECK(fread(&m_flScale , sizeof(float) , 1 , fp) , 1)
		IO_CHECK(fread(&numFrames , sizeof(short) , 1 , fp) , 1)
		IO_CHECK(fread(&numBones , sizeof(short) , 1 , fp) , 1)
		IO_CHECK(fread(&type , sizeof(char) , 1 , fp) , 1)

		unsigned int sz = entrySize - 9;
	filename.resize(255 , '\0');
	if (sz > filename.size() + 1)
	{
		IO_CHECK(fread(filename.data() , sizeof(char) , filename.size() , fp) , filename.size())
			cout_r() << "Warning: filename of length " << sz << " (bytes) truncated to: \n" << filename << '\n';
	}
	else
		IO_CHECK(fread(filename.data() , sizeof(char) , sz , fp) , sz)

		return true;
}

bool aidxSubFile::Write(FILE* fp) noexcept
{
	if (!fp)
		return false;

	size_t len = filename.length() + 1 , esize = len + 9;

	IO_CHECK(fwrite(&esize , sizeof(int) , 1 , fp) , 1)
		IO_CHECK(fwrite(&beginFileOffset , sizeof(int) , 1 , fp) , 1)
		IO_CHECK(fwrite(&fileSize , sizeof(int) , 1 , fp) , 1)
		IO_CHECK(fwrite(&m_flScale , sizeof(float) , 1 , fp) , 1)
		IO_CHECK(fwrite(&numFrames , sizeof(short) , 1 , fp) , 1)
		IO_CHECK(fwrite(&numBones , sizeof(short) , 1 , fp) , 1)
		IO_CHECK(fwrite(&type , sizeof(char) , 1 , fp) , 1)

		IO_CHECK(fwrite(filename.c_str() , sizeof(char) , len , fp) , len)

		return true;
}

bool aidxSubFile::GatherInfo() noexcept
{
	if (!buffer)
		return false;

	entrySize = filename.length() + 10;
	numFrames = ((short*)buffer)[0];
	numBones = ((short*)buffer)[1];
	type = ((char*)buffer)[4];
	return Scale();
}
  
void aidxFile::Allocate() noexcept
{
	if (subFiles)
		delete subFiles;

	subFiles = new aidxSubFile(true);
}

void aidxFile2::Allocate() noexcept
{
	if (subFiles)
		delete subFiles;

	subFiles = new aidxSubFile(false);
}

void aidxFile::Init() noexcept
{
	fileVersion = 4;
	filetype = SANM;
}

void aidxFile2::Init() noexcept
{
	fileVersion = 9;
	filetype = SANM;
}
   
bool aidxFile::ReadHeader() noexcept
{
	char checktype[_countof(SANM)] = { '\0' };

	IO_CHECK(fread(checktype, _countof(SANM) - 1, 1, fp), 1)
		if (strcmp(checktype, SANM) != 0)
		{
			fprintf(stderr, "File is not an animation idx archive.\n");
			return false;
		}
	fseek(fp, 16, SEEK_SET);
	IO_CHECK(fread(&numFiles, sizeof(int), 1, fp), 1)
		return true;
}

void aidxFile::WriteHeader() noexcept
{
	constexpr short zero = 0;
	fseek(fp, 0, SEEK_SET);
	fseek(fp2, 0, SEEK_SET);

	fwrite(filetype, sizeof(char), strlen(filetype) + 1, fp);
	fwrite(&zero, sizeof(short), 1, fp);
	fwrite(&fileVersion, sizeof(int), 1, fp);
	fwrite(&numFiles, sizeof(int), 1, fp);

	fwrite(filetype, sizeof(char), strlen(filetype) + 1, fp2);
	fwrite(&zero, sizeof(short), 1, fp2);
	fwrite(&fileVersion, sizeof(int), 1, fp2);
	fwrite(&numFiles, sizeof(int), 1, fp2);

	fflush(fp);
	fflush(fp2);
}

/*Skeleton sub idx file*/
bool kidxSubFile::Read(FILE* fp) noexcept
{
	if (!fp)
		return false;

	IO_CHECK(fread(&len, sizeof(int), 1, fp), 1)

		filename.resize(len + 1, '\0');

	IO_CHECK(fread(&beginFileOffset, sizeof(int), 1, fp), 1)
		IO_CHECK(fread(&fileSize, sizeof(int), 1, fp), 1)
		IO_CHECK(fread(filename.data(), len, 1, fp), 1)

		return true;
}

bool kidxSubFile::Write(FILE* fp) noexcept
{
	if (!fp)
		return false;

	IO_CHECK(fwrite(&len, sizeof(int), 1, fp), 1)
		IO_CHECK(fwrite(&beginFileOffset, sizeof(int), 1, fp), 1)
		IO_CHECK(fwrite(&fileSize, sizeof(int), 1, fp), 1)
		IO_CHECK(fwrite(filename.c_str(), sizeof(char), filename.length(), fp), filename.length())
		return true;
}

void kidxSubFile::ParseFilename(const char* fname) noexcept
{
	char* tstr = _strdup(fname);
	strrep(tstr, '\\', '/');
	int slen = strlen(tstr);
	char* str = tstr + slen - basename_length(tstr, slen);
	filename = str;
	free(tstr);
}

bool kidxSubFile::GatherInfo() noexcept
{
	if (!buffer)
		return false;

	len = filename.length();
	len++;

	return true;
}

/*Skeleton idx file*/
void kidxFile::Allocate() noexcept
{
	if (subFiles)
		delete subFiles;

	subFiles = new kidxSubFile;
}

void kidxFile::Init() noexcept
{
	fileVersion = 3;
	filetype = SSKL;
}

void kidxFile2::Init() noexcept
{
	fileVersion = 0x18000E;
	filetype = SSKL;
}
   
bool kidxFile::ReadHeader() noexcept
{
	char checktype[_countof(SSKL)] = { '\0' };

	IO_CHECK(fread(checktype, _countof(SSKL) - 1, 1, fp), 1)

		if (strcmp(checktype, SSKL) != 0)
		{
			fprintf(stderr, "File is not a skeleton idx archive.\n");
			return false;
		}

	fseek(fp, 16, SEEK_SET);
	IO_CHECK(fread(&numFiles, sizeof(int), 1, fp), 1)
		return true;
}

void kidxFile::WriteHeader() noexcept
{
	constexpr short zero = 0;

	fseek(fp, 0, SEEK_SET);
	fseek(fp2, 0, SEEK_SET);

	fwrite(filetype, sizeof(char), strlen(filetype) + 1, fp);
	fwrite(&zero, sizeof(short), 1, fp);
	fwrite(&fileVersion, sizeof(int), 1, fp);
	fwrite(&numFiles, sizeof(int), 1, fp);

	fwrite(filetype, sizeof(char), strlen(filetype) + 1, fp2);
	fwrite(&zero, sizeof(short), 1, fp2);
	fwrite(&fileVersion, sizeof(int), 1, fp2);
	fwrite(&numFiles, sizeof(int), 1, fp2);

	fflush(fp);
	fflush(fp2);
}

void eidxFile::Allocate() noexcept
{
	if (subFiles)
		delete subFiles;

	subFiles = new eidxSubFile;
}

void eidxFile::Init() noexcept
{
	fileVersion = 0x30;
	filetype = SEVT;
}

void eidxFile2::Init() noexcept
{
	fileVersion = 0x49;
	filetype = SEVT;
}

bool eidxFile::ReadHeader() noexcept
{
	char checktype[_countof(SEVT)] = { '\0' };
	IO_CHECK(fread(checktype, _countof(SEVT) - 1, 1, fp), 1)
		if (strcmp(checktype, SEVT) != 0)
		{
			fprintf(stderr, "File is not an event idx archive.\n");
			return false;
		}
	fseek(fp, 16, SEEK_SET);
	IO_CHECK(fread(&numFiles, sizeof(int), 1, fp), 1)
		return true;
}

void eidxFile::WriteHeader() noexcept
{
	constexpr int zero = 0;

	fseek(fp, 0, SEEK_SET);
	fseek(fp2, 0, SEEK_SET);

	fwrite(filetype, sizeof(char), strlen(filetype) + 1, fp);
	fwrite(&zero, sizeof(int), 1, fp);
	fwrite(&fileVersion, sizeof(int), 1, fp);
	fwrite(&numFiles, sizeof(int), 1, fp);

	fwrite(filetype, sizeof(char), strlen(filetype) + 1, fp2);
	fwrite(&zero, sizeof(int), 1, fp2);
	fwrite(&fileVersion, sizeof(int), 1, fp2);
	fwrite(&numFiles, sizeof(int), 1, fp2);

	fflush(fp);
	fflush(fp2);
}

void eidxSubFile::ParseFilename(const char* name) noexcept
{
	idxSubFile::ParseFilename(name);
	int len = filename.length(), fr = len - basename_length(filename.c_str(), len);

	if (isdigit(filename[fr]) == false)
	{
		cout_r() << std::format(
			"File name does not match '<type_number><record_name>' naming convention.\n"
			"Expected a number between 1 and 4 (inclusive), got: '{}' in: '{}'.\n",
			filename[fr],
			filename);

		tainted = true;
		return;
	}

	int type = atoi(filename.c_str() + (fr * sizeof(char)));
	if (type < 1 || type > 4)
	{
		cout_r() << std::format(
			"File name does not match '<type_number><record_name>' naming convention.\n"
			"Expected a number between 1 and 4 (inclusive), got: '{}', in: '{}'.\n",
			type,
			filename);

		tainted = true;
	}
	else
	{
		frameId = type;
		tainted = false;
	}
}

char code(unsigned int num) noexcept
{
	int c = (int)(log((double)num) / log(10.0f));
	return (char)(c + 97);
}

bool eidxSubFile::Read(FILE* fp) noexcept
{
	if (!fp || tainted)
		return false;

	int rlen = 0, new_frame = 0;
	fread(&rlen, sizeof(int), 1, fp);
	if (rlen != 4)
	{
		cout_r() << "Bad record length: " << rlen << "\nOnly records of length 4 are supported.\n";
		return false;
	}

	fread(&beginFileOffset, sizeof(int), 1, fp);
	fread(&fileSize, sizeof(int), 1, fp);
	fread(&new_frame, sizeof(int), 1, fp);
	if (new_frame != frameId)
	{
		num = 0;
		frameId = new_frame;
	}

	num++;
	filename.clear();
	filename = std::format("{}_{}_{}.bin", frameId, code(num), num);
	return true;
}

bool eidxSubFile::Write(FILE* fp) noexcept
{
	if (!fp || tainted)
		return false;

	constexpr int four = 4;

	IO_CHECK(fwrite(&four, sizeof(int), 1, fp), 1)
		IO_CHECK(fwrite(&beginFileOffset, sizeof(int), 1, fp), 1)
		IO_CHECK(fwrite(&fileSize, sizeof(int), 1, fp), 1)
		IO_CHECK(fwrite(&frameId, sizeof(int), 1, fp), 1)
		return true;
}
