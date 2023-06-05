#include "GameInterfaceFile.hpp"

#include <fmt/color.h>
#include <stb_image.h>

#include <array>
#include <map>
#include <ranges>
#include <set>
#include <tuple>
#include <vector>

namespace fs = std::filesystem;

using namespace std::literals;

using std::array;
using std::map;
using std::set;
using std::string;
using std::tuple;
using std::vector;

set<Image_t> GameInterfaceFile_t::Images() const noexcept
{
	return
		m_rgSprites
		| std::views::transform([](Sprite_t const &spr) noexcept -> Image_t { return spr.m_Image; })
		| std::ranges::to<set>();
}

void GameInterfaceFile_t::Import(fs::path const& Path) noexcept
{
	Clear();

	tinyxml2::XMLDocument xml;
	if (xml.LoadFile(Path.u8string().c_str()) != tinyxml2::XML_SUCCESS) [[unlikely]]
	{
		fmt::print(fg(fmt::color::red), "File no found or cannot be process: {}\n", Path.u8string());
		return;
	}

	auto pRoot = xml.FirstChildElement("root");
	if (!pRoot) [[unlikely]]
	{
		fmt::print(fg(fmt::color::red), "File damaged beyond mendable: Node 'root' no found.\n");
		return;
	}

	static constexpr array rgszKeyNodeNames{ "version"sv, "enumeration_name"sv, "texture_pages"sv, "sprites"sv };
	array<decltype(pRoot), decltype(rgszKeyNodeNames){}.size()> rgpKeyNodes{};

	for (auto &&[szNodeName, pKeyNode] : std::views::zip(rgszKeyNodeNames, rgpKeyNodes))
	{
		if (pKeyNode = pRoot->FirstChildElement(szNodeName.data()); pKeyNode == nullptr)
			fmt::print(fg(fmt::color::red), "File damaged beyond mendable: Node 'root/{}' no found.\n", szNodeName);
	}

	// Read version

	auto &pVer = rgpKeyNodes[0];
	if (auto const p = pVer->GetText(); p != nullptr)
		m_Version = atoi(p);
	else
	{
		fmt::print(fg(fmt::color::red), "File damaged beyond mendable: 'root/{}' value missing.\n", rgszKeyNodeNames[0]);
		return;
	}

	// Read enumeration_name

	auto &pName = rgpKeyNodes[1];
	if (auto const p = pName->GetText(); p != nullptr)
		m_EnumerationName = (string)p;
	else
	{
		fmt::print(fg(fmt::color::red), "File damaged beyond mendable: 'root/{}' value missing.\n", rgszKeyNodeNames[1]);
		return;
	}

	// Read texture_pages

	auto &pPages = rgpKeyNodes[2];
	vector<fs::path> rgszPaths{};
	if (auto const iCount = pPages->IntAttribute("count", -1); iCount > 0)
		rgszPaths.reserve(iCount);
	else
		fmt::print(fg(fmt::color::light_golden_rod_yellow), "File damaged: 'root/{}.count' attribute missing.\n", rgszKeyNodeNames[2]);

	auto const szParentPath = Path.parent_path();
	for (auto pTex = pPages->FirstChildElement("page"); pTex != nullptr; pTex = pTex->NextSiblingElement("page"))
	{
		auto const p = pTex->Attribute("file");

		if (!p)
		{
			fmt::print(fg(fmt::color::red), "File damaged beyond mendable: 'root/{}/page.file' value missing.\n", rgszKeyNodeNames[2]);
			return;
		}

		rgszPaths.emplace_back(szParentPath / L"southern_european" / L"interface" / p);

		if (!fs::exists(rgszPaths.back()))
			fmt::print(fg(fmt::color::light_golden_rod_yellow), "File no found: '{}'\n", rgszPaths.back().u8string());
	}

	// Read sprites

	auto &pSprites = rgpKeyNodes[3];
	if (auto const iCount = pSprites->IntAttribute("count", -1); iCount > 0)
		m_rgSprites.reserve(iCount);
	else
		fmt::print(fg(fmt::color::light_golden_rod_yellow), "File damaged: 'root/{}.count' attribute missing.\n", rgszKeyNodeNames[3]);

	for (auto pSpr = pSprites->FirstChildElement("sprite"); pSpr != nullptr; pSpr = pSpr->NextSiblingElement("sprite"))
	{
		try
		{
			m_rgSprites.emplace_back(
				Sprite_t
				{
					.m_Name{ pSpr->Attribute("name") },
					.m_Image{ rgszPaths.at(pSpr->IntAttribute("page", -1)) },
					.m_Rect{ pSpr->IntAttribute("left", -1), pSpr->IntAttribute("right", -1), pSpr->IntAttribute("top", -1), pSpr->IntAttribute("bottom", -1), },
					.m_OfsX = pSpr->IntAttribute("x_offset", -1),
					.m_OfsY = pSpr->IntAttribute("y_offset", -1),
					.m_IsAlpha = pSpr->BoolAttribute("alpha", true),
					.m_IsCursor = pSpr->BoolAttribute("cursor", false),
				}
			);
		}
		catch (...)
		{
			fmt::print(fg(fmt::color::red), "File damaged beyond mendable: missing element or illegal value in 'root/{}/sprite.<???>'\n", rgszKeyNodeNames[3]);
			return;
		}
	}

	if (Config::ShouldSort)
		std::ranges::sort(m_rgSprites);
}

void GameInterfaceFile_t::Export(tinyxml2::XMLDocument *xml) const noexcept
{
	auto pRoot = xml->NewElement("root");
	xml->InsertFirstChild(pRoot);	// TBH, this is stupit.

	auto pVer = pRoot->InsertNewChildElement("version");
	pVer->SetText(std::to_string(m_Version).c_str());

	auto pName = pRoot->InsertNewChildElement("enumeration_name");
	pName->SetText(m_EnumerationName.c_str());

	auto const rgszImages = Images();

	auto pPages = pRoot->InsertNewChildElement("texture_pages");
	pPages->SetAttribute("count", rgszImages.size());

	for (auto &&[szFileName, iWidth, iHeight] :
		rgszImages
		| std::views::transform([](Image_t const &Img) noexcept { return tuple{ Img.m_Path.filename().u8string(), Img.m_iWidth, Img.m_iHeight }; })
		)
	{
		auto pTex = pPages->InsertNewChildElement("page");
		pTex->SetAttribute("file", szFileName.c_str());
		pTex->SetAttribute("width", iWidth);
		pTex->SetAttribute("height", iHeight);
		pTex->SetAttribute("force32bit", 0);	// #RESEARCH what is this?
	}

	// #UPDATE_AT_CPP23 try views::enumerate and to a map?
	map<fs::path, int> Mapping{};
	for (int i = 0; auto && Img : rgszImages)
		Mapping[Img.m_Path] = i++;

	auto pSprites = pRoot->InsertNewChildElement("sprites");
	pSprites->SetAttribute("count", m_rgSprites.size());

	for (int i = 0; auto &&Sprite : m_rgSprites)
	{
		auto pSpr = pSprites->InsertNewChildElement("sprite");
		pSpr->SetAttribute("index", i++);
		pSpr->SetAttribute("name", Sprite.m_Name.c_str());
		pSpr->SetAttribute("page", Mapping.at(Sprite.m_Image.m_Path));
		pSpr->SetAttribute("left", Sprite.m_Rect.m_left);
		pSpr->SetAttribute("right", Sprite.m_Rect.m_right);
		pSpr->SetAttribute("top", Sprite.m_Rect.m_top);
		pSpr->SetAttribute("bottom", Sprite.m_Rect.m_bottom);
		pSpr->SetAttribute("x_offset", Sprite.m_OfsX);
		pSpr->SetAttribute("y_offset", Sprite.m_OfsY);
		pSpr->SetAttribute("alpha", (int)Sprite.m_IsAlpha);
		pSpr->SetAttribute("cursor", (int)Sprite.m_IsCursor);	// M2TW requires 1 or 0, not 'true' or 'false'
	}
}
