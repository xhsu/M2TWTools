#include <iostream>
#include <fstream>

import battle_models;
import export_descr_unit;
import UtlRandom;
import UtlString;

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

	edu_file_t file("ss6.4_edu.txt");
	file.Initialize();
	file.Parse();

	//std::string s = "stat_pri_ex      4, 999, 1";
	//wpn_stat_ex_t obj;

	//obj.Parse(s);

	//std::cout << obj.ToString() << std::endl;

	std::cout << file.Count() << " units presented in the file.\n\n";
	std::cout << file.m_Units.front() << std::endl;
	std::cout << *UTIL_GetRandomOne(file.m_Units) << std::endl;
	for (const auto& ins : file.m_Units)
		if (ins.m_stat_pri.m_bMusketShotSet)
		{
			std::cout << ins << std::endl;
			break;
		}
	std::cout << file.m_Units.back() << std::endl;

	if (std::ofstream of("output.txt"); of)
	{
		of << *UTIL_GetRandomOne(file.m_Units) << std::endl;
		of << *UTIL_GetRandomOne(file.m_Units) << std::endl;
		of << *UTIL_GetRandomOne(file.m_Units) << std::endl;

		of.close();
	}

	return EXIT_SUCCESS;
}
