#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <list>

#include <cassert>

#include "tinyxml2/tinyxml2.h"

import UtlRandom;
import UtlString;
import battle_models;
import descr_mercenaries;
import descr_regions;
import export_descr_unit;
import ui_xmls;

using namespace tinyxml2;

void CompleteSettlement(void) noexcept
{
	descr_regions_t file;
	std::list<std::string> rgExists;

	if (std::ifstream f("descr_strat.txt"); f)
	{
		bool bInside = false;
		while (!f.eof())
		{
			std::string sz;
			std::getline(f, sz);

			UTIL_Trim(sz);

			if (auto pos = sz.find_first_of(';'); pos != sz.npos)
				sz.erase(pos);

			if (!bInside && sz.starts_with("settlement"))
			{
				std::getline(f, sz);
				UTIL_Trim(sz);

				if (!sz.empty() && sz[0] == '{')
					bInside = true;

				continue;
			}
			else if (bInside && sz.starts_with('}'))
			{
				bInside = false;
				continue;
			}

			if (bInside && sz.starts_with("region "))
				rgExists.emplace_back(sz.substr(strlen_c("region ")));
		}

		f.close();
	}

	if (std::ofstream f("output.txt"); f)
	{
		for (const auto& ins : file.m_Regions)
		{
			if (std::find(rgExists.begin(), rgExists.end(), ins.Get<Region_t::m_szProvincialName>()) != rgExists.end())
				continue;

			f <<
				"settlement\n"
				"{\n"
				"\tlevel village\n"
				"\tregion " << ins.Get<Region_t::m_szProvincialName>() << '\n' <<
				"\t\n"
				"\tyear_founded 0\n"
				"\tpopulation 400\n"
				"\tplan_set default_set\n"
				"\tfaction_creator " << ins.Get<Region_t::m_szCityBuilder>() << '\n' <<
				"}\n"
				<< std::endl;
		}

		f.close();
	}
}

void CheckUnitPresence(void) noexcept
{
	std::list<std::string> rgValidTraits;
	if (std::ifstream f("export_descr_character_traits.txt"); f)
	{
		while (!f.eof())
		{
			std::string sz;
			std::getline(f, sz);

			if (auto pos = sz.find_first_of(';'); pos != sz.npos)
				sz.erase(pos);

			UTIL_Trim(sz);
			if (sz.empty())
				continue;

			if (!sz.starts_with("Trait "))
				continue;

			sz.erase(0, strlen_c("Trait "));
			rgValidTraits.emplace_back(std::move(sz));
		}

		f.close();
	}

	if (std::ifstream f("descr_strat.txt"); f)
	{
		size_t iLine = 0;

		while (!f.eof())
		{
			++iLine;

			std::string sz;
			std::getline(f, sz);

			if (auto pos = sz.find_first_of(';'); pos != sz.npos)
				sz.erase(pos);

			UTIL_Trim(sz);
			if (sz.empty())
				continue;

			if (!sz.starts_with("traits "))
				continue;

			sz.erase(0, strlen_c("traits "));

			std::list<std::string> rgsz;
			std::list<std::pair<std::string, short>> rgTraits;

			UTIL_Split(sz, rgsz, ',');
			for (auto& s : rgsz)
			{
				UTIL_Trim(s);

				auto breakpos = s.find_first_of(' ');
				assert(breakpos != s.npos);

				rgTraits.emplace_back(std::make_pair(s.substr(0, breakpos), UTIL_StrToNum<short>(s.substr(breakpos))));
			}

			assert(!rgTraits.empty());

			for (const auto& [szTrait, iLevel] : rgTraits)
				if (std::find(rgValidTraits.begin(), rgValidTraits.end(), szTrait) == rgValidTraits.end())
					std::cout << "Trait \"" << szTrait << "\" no found in line " << iLine << std::endl;
		}

		f.close();
	}
}

void ValidateUnitsForDesrcStart(void) noexcept
{
	edu_file_t f_edu("vanilla_edu.txt");
	f_edu.Initialize();
	f_edu.Parse();

	if (std::ifstream f("descr_strat.txt"); f)
	{
		size_t iLine = 0;

		while (!f.eof())
		{
			++iLine;

			std::string sz;
			std::getline(f, sz);

			if (auto pos = sz.find_first_of(';'); pos != sz.npos)
				sz.erase(pos);

			UTIL_Trim(sz);
			if (sz.empty())
				continue;

			if (!sz.starts_with("unit"))
				continue;

			sz.erase(0, 5);
			auto pos = sz.find("exp");
			sz.erase(pos);
			UTIL_Trim(sz);

			if (std::find_if(f_edu.m_Units.begin(), f_edu.m_Units.end(), [&](const Unit_t& ins) { return ins.m_type == sz; }) == f_edu.m_Units.end())
				std::cout << "Unit \"" << sz << "\" no found at line " << iLine << std::endl;
		}
	}
}

void DelNonexistUnitForDesrcMercs(void) noexcept
{
	descr_mercenaries_t f;
	std::cout << f.m_Pools.front() << std::endl;
	std::cout << *UTIL_GetRandomOne(f.m_Pools) << std::endl;
	std::cout << f.m_Pools.back() << std::endl;

	edu_file_t f_edu("vanilla_edu.txt");
	f_edu.Initialize();
	f_edu.Parse();

	for (auto& Pool : f.m_Pools)
	{
		for (auto it = Pool.m_rgMercenaries.begin(); it != Pool.m_rgMercenaries.end(); /* Do nothing */)
		{
			if (std::find_if(f_edu.m_Units.begin(), f_edu.m_Units.end(), [&](const Unit_t& Unit) { return it->m_szUnit == Unit.m_type; }) == f_edu.m_Units.end())
			{
				std::cout << "Unknown unit \"" << it->m_szUnit << "\" in pool \"" << Pool.m_szName << '"' << std::endl;
				it = Pool.m_rgMercenaries.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	if (std::ofstream fout("output.txt"); fout)
	{
		fout << f;
		fout.close();
	}
}

void RemoveFaction(modeldb_file_t& f, const std::string& szFaction)
{
	size_t i = 0;
	for (auto itBM = f.m_rgBattleModels.begin(); itBM != f.m_rgBattleModels.end(); /* Do Nothing*/)
	{
		bool bAmericaUnit = itBM->m_UnitTex.size() <= 2 && itBM->m_UnitTex.contains(szFaction) && itBM->m_AttachmentTex.size() <= 2 && itBM->m_AttachmentTex.contains(szFaction);

		if (!bAmericaUnit)
		{
			for (auto it = itBM->m_UnitTex.begin(); it != itBM->m_UnitTex.end(); /* Do Nothing */)
			{
				if (it->first == szFaction)
					it = itBM->m_UnitTex.erase(it);
				else
					++it;
			}

			for (auto it = itBM->m_AttachmentTex.begin(); it != itBM->m_AttachmentTex.end(); /* Do Nothing */)
			{
				if (it->first == szFaction)
					it = itBM->m_AttachmentTex.erase(it);
				else
					++it;
			}
		}

		if (itBM->m_szName.find("mount_") == std::string::npos && (bAmericaUnit || itBM->m_UnitTex.empty() || itBM->m_AttachmentTex.empty()))
		{
			++i;
			std::cout << "Removing entry \"" << itBM->m_szName << "\".\n";
			itBM = f.m_rgBattleModels.erase(itBM);
		}
		else
			++itBM;
	}

	std::cout << i << " entry(ies) was(were) removed for " << szFaction << '\n';
}

int main(int argc, char** argv)
{
	/*
	modeldb_file_t file("battle_models.modeldb");
	//modeldb_file_t file("battle_models_long_lod.modeldb");
	//modeldb_file_t file("test.modeldb");

	file.Initialize();
	std::cout << "Total models: " << file.m_iTotalModels << std::endl;
	std::cout << "======================================================\n";
	std::cout << *UTIL_GetRandomOne(file.m_rgBattleModels) << std::endl;
	std::cout << "======================================================\n";
	std::cout << file.m_rgBattleModels.back();

	//file.Save("output_test.modeldb");
	file.Save("battle_models.modeldb");
	*/

	FixUIXML("shared.sd.xml");

	return EXIT_SUCCESS;
}
