module;

#include <iostream>
#include <string>

#include <cstring>

#include "tinyxml2/tinyxml2.h"

export module ui_xmls;

import UtlString;
import UtlWinConsole;

using namespace std::string_literals;
using namespace std::string_view_literals;

using std::string;

export bool CheckUIXML(const char* pszFile) noexcept
{
	tinyxml2::XMLDocument xml;
	if (xml.LoadFile(pszFile) != tinyxml2::XML_SUCCESS) [[unlikely]]
	{
		cout_w() << "Fail on loading file \"" << pszFile << "\".\n";
		return false;
	}

	auto pRoot = xml.FirstChildElement("root");
	if (!pRoot) [[unlikely]]
	{
		cout_w() << "Node 'root' no found in file.\n";
		return false;
	}

	auto pTexPages = pRoot->FirstChildElement("texture_pages");
	if (!pTexPages) [[unlikely]]
	{
		cout_w() << "Node 'texture_pages' no found under the node 'root'.\n";
		return false;
	}

	size_t i = 0;
	for (auto p = pTexPages->FirstChildElement("page"); p != nullptr; p = p->NextSiblingElement("page"))
	{
		const char* psz = nullptr;

		if (p->QueryStringAttribute("file", &psz) != tinyxml2::XML_SUCCESS) [[unlikely]]
		{
			cout_w() << "Attribute 'file' no found in entry '" << p->Value() << "' at index " << i << ".\n";
			return false;
		}

		++i;
	}

	if (size_t iCount = pTexPages->UnsignedAttribute("count"); i != iCount)	[[unlikely]]
	{
		cout_w() << "The 'count' attribute of node 'texture_pages' expected to be " << i << ", but " << iCount << " found.\n";
		return false;
	}

	auto pSprites = pRoot->FirstChildElement("sprites");
	if (!pSprites) [[unlikely]]
	{
		cout_w() << "Node 'sprites' no found under the node 'root'.\n";
		return false;
	}

	i = 0;
	for (auto p = pSprites->FirstChildElement("sprite"); p != nullptr; p = p->NextSiblingElement("sprite"))
	{
		if (size_t iIndex = p->UnsignedAttribute("index"); i != iIndex) [[unlikely]]
		{
			const char* psz = nullptr;
			p->QueryStringAttribute("name", &psz);

			cout_w() << "The 'index' attribute of node 'sprite'(named: \"" << psz << "\") expected to be " << i << ", but " << iIndex << " found.\n";
			return false;
		}

		++i;
	}

	if (size_t iCount = pSprites->UnsignedAttribute("count"); i != iCount) [[unlikely]]
	{
		cout_w() << "The 'count' attribute of node 'sprites' expected to be " << i << ", but " << iCount << " found.\n";
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
		cout_w() << "File damaged beyond mendable: File \"" << pszFile << "\" doesn't exists.\n";
		return;
	}

	auto pRoot = xml.FirstChildElement("root");
	if (!pRoot) [[unlikely]]
	{
		cout_w() << "File damaged beyond mendable: Node 'root' no found in file.\n";
		return;
	}

	auto pTexPages = pRoot->FirstChildElement("texture_pages");
	if (!pTexPages) [[unlikely]]
	{
		cout_w() << "File damaged beyond mendable: Node 'texture_pages' no found under the node 'root'.\n";
		return;
	}

	size_t i = 0;
	for (auto p = pTexPages->FirstChildElement("page"); p != nullptr; p = p->NextSiblingElement("page"))
	{
		++i;
	}

	if (size_t iCount = pTexPages->UnsignedAttribute("count"); i != iCount) [[unlikely]]
	{
		cout_w() << "Mending: 'count' attribute mismatched under node 'texture_pages'. Value changed to " << i << " from " << iCount << ".\n";
		pTexPages->SetAttribute("count", i);
		bShouldSave = true;
	}

	auto pSprites = pRoot->FirstChildElement("sprites");
	if (!pSprites) [[unlikely]]
	{
		cout_w() << "File damaged beyond mendable: Node 'sprites' no found under the node 'root'.\n";
		return;
	}

	i = 0;
	for (auto p = pSprites->FirstChildElement("sprite"); p != nullptr; p = p->NextSiblingElement("sprite"))
	{
		if (size_t iIndex = p->UnsignedAttribute("index"); i != iIndex) [[unlikely]]
		{
			const char* psz = nullptr;
			p->QueryStringAttribute("name", &psz);

			cout_w() << "Mending: 'index' attribute of node 'sprite'(named: \"" << psz << "\") changed to " << i << " from " << iIndex << ".\n";
			p->SetAttribute("index", i);
			bShouldSave = true;
		}

		++i;
	}

	if (size_t iCount = pSprites->UnsignedAttribute("count"); i != iCount) [[unlikely]]
	{
		cout_w() << "Mending: 'count' attribute mismatched under node 'sprites'. Value changed to " << i << " from " << iCount << ".\n";
		pSprites->SetAttribute("count", i);
		bShouldSave = true;
	}

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

		cout_lime() << "Mended file saved as: " << pOutpath << '\n';
		xml.SaveFile(pOutpath);

		delete[] pOutpath;
	}
	else
	{
		cout_lime() << "File \"" << pszFile << "\" checks out.\n";
	}
}
