#include <iostream>
#include <vector>

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

	/*
	edu_file_t file("export_descr_unit.txt");
	file.Initialize();

	std::cout << file.GetLine() << std::endl;
	file.Skip(4);

	for (int i = 0; i < 10; i++)
	{
		file.Skip(i);
		file.Rewind(i);
		std::cout << file.GetLine() << std::endl;

		file.Rewind();
	}
	*/

	std::string s = "stat_pri_armour  6, 6, 6, metal";
	stat_pri_armour_t obj;

	obj.Parse(s);

	std::cout << obj.ToString() << std::endl;

	return EXIT_SUCCESS;
}
