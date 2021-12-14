module;

#include <iostream>
#include <string>

#include <cstring>

#include <Windows.h>

#include "tinyxml2/tinyxml2.h"

export module ui_xmls;

import UtlColor;
import UtlString;

using namespace std::string_literals;
using namespace std::string_view_literals;

using std::string;

export bool CheckUIXML(const char* pszFile) noexcept
{
	tinyxml2::XMLDocument xml;
	if (xml.LoadFile(pszFile) != tinyxml2::XML_SUCCESS) [[unlikely]]
	{
		std::cout << "Fail on loading file \"" << pszFile << "\".\n";
		return false;
	}

	auto pRoot = xml.FirstChildElement("root");
	if (!pRoot) [[unlikely]]
	{
		std::cout << "Node 'root' no found in file.\n";
		return false;
	}

	auto pTexPages = pRoot->FirstChildElement("texture_pages");
	if (!pTexPages) [[unlikely]]
	{
		std::cout << "Node 'texture_pages' no found under the node 'root'.\n";
		return false;
	}

	size_t i = 0;
	for (auto p = pTexPages->FirstChildElement("page"); p != nullptr; p = p->NextSiblingElement("page"))
	{
		const char* psz = nullptr;

		if (p->QueryStringAttribute("file", &psz) != tinyxml2::XML_SUCCESS) [[unlikely]]
		{
			std::cout << "Attribute 'file' no found in entry '" << p->Value() << "' at index " << i << ".\n";
			return false;
		}

		++i;
	}

	if (size_t iCount = pTexPages->UnsignedAttribute("count"); i != iCount)	[[unlikely]]
	{
		std::cout << "The 'count' attribute of node 'texture_pages' expected to be " << i << ", but " << iCount << " found.\n";
		return false;
	}

	auto pSprites = pRoot->FirstChildElement("sprites");
	if (!pSprites) [[unlikely]]
	{
		std::cout << "Node 'sprites' no found under the node 'root'.\n";
		return false;
	}

	i = 0;
	for (auto p = pSprites->FirstChildElement("sprite"); p != nullptr; p = p->NextSiblingElement("sprite"))
	{
		if (size_t iIndex = p->UnsignedAttribute("index"); i != iIndex) [[unlikely]]
		{
			const char* psz = nullptr;
			p->QueryStringAttribute("name", &psz);

			std::cout << "The 'index' attribute of node 'sprite'(named: \"" << psz << "\") expected to be " << i << ", but " << iIndex << " found.\n";
			return false;
		}

		++i;
	}

	if (size_t iCount = pSprites->UnsignedAttribute("count"); i != iCount) [[unlikely]]
	{
		std::cout << "The 'count' attribute of node 'sprites' expected to be " << i << ", but " << iCount << " found.\n";
		return false;
	}

	return true;
}

export void FixUIXML(const char* pszFile) noexcept
{
	bool bShouldSave = false;

	tinyxml2::XMLDocument xml;
	if (xml.LoadFile(pszFile) != tinyxml2::XML_SUCCESS) [[unlikely]]
	{
		std::cout << "File damaged beyond mendable: File \"" << pszFile << "\" doesn't exists.\n";
		return;
	}

	auto pRoot = xml.FirstChildElement("root");
	if (!pRoot) [[unlikely]]
	{
		std::cout << "File damaged beyond mendable: Node 'root' no found in file.\n";
		return;
	}

	auto pTexPages = pRoot->FirstChildElement("texture_pages");
	if (!pTexPages) [[unlikely]]
	{
		std::cout << "File damaged beyond mendable: Node 'texture_pages' no found under the node 'root'.\n";
		return;
	}

	size_t i = 0;
	for (auto p = pTexPages->FirstChildElement("page"); p != nullptr; p = p->NextSiblingElement("page"))
	{
		++i;
	}

	if (size_t iCount = pTexPages->UnsignedAttribute("count"); i != iCount) [[unlikely]]
	{
		std::cout << "Mending: 'count' attribute mismatched under node 'texture_pages'. Value changed to " << i << " from " << iCount << ".\n";
		pTexPages->SetAttribute("count", i);
		bShouldSave = true;
	}

	auto pSprites = pRoot->FirstChildElement("sprites");
	if (!pSprites) [[unlikely]]
	{
		std::cout << "File damaged beyond mendable: Node 'sprites' no found under the node 'root'.\n";
		return;
	}

	i = 0;
	for (auto p = pSprites->FirstChildElement("sprite"); p != nullptr; p = p->NextSiblingElement("sprite"))
	{
		if (size_t iIndex = p->UnsignedAttribute("index"); i != iIndex) [[unlikely]]
		{
			const char* psz = nullptr;
			p->QueryStringAttribute("name", &psz);

			std::cout << "Mending: 'index' attribute of node 'sprite'(named: \"" << psz << "\") changed to " << i << " from " << iIndex << ".\n";
			p->SetAttribute("index", i);
			bShouldSave = true;
		}

		++i;
	}

	if (size_t iCount = pSprites->UnsignedAttribute("count"); i != iCount) [[unlikely]]
	{
		std::cout << "Mending: 'count' attribute mismatched under node 'sprites'. Value changed to " << i << " from " << iCount << ".\n";
		pSprites->SetAttribute("count", i);
		bShouldSave = true;
	}

	auto hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, WINCON_TEXT_GREEN + WINCON_BG_BLACK);

	if (bShouldSave)
	{
		auto len = strlen(pszFile) + strlen_c("_mended");
		char* pOutpath = new char[len + 1];
		strcpy(pOutpath, pszFile);

		if (auto p = stristr(pOutpath, ".xml"); p != nullptr)
			strcpy(p, "_mended.xml");
		else
			strcat(pOutpath, "_mended");

		pOutpath[len] = '\0';

		std::cout << "Mended file saved as: " << pOutpath << '\n';
		xml.SaveFile(pOutpath);

		delete[] pOutpath;
	}
	else
	{
		std::cout << "File \"" << pszFile << "\" checks out.\n";
	}

	SetConsoleTextAttribute(hConsole, WINCON_TEXT_WHITE + WINCON_BG_BLACK);
}
