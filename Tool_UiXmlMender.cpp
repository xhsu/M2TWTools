/*
* Dec 14 2021
*/

#include <filesystem>
#include <iostream>

#include <cstring>

import UtlWinConsole;
import ui_xmls;

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
	if (argc <= 1)	// All.
	{
		auto CurPath = fs::current_path();
		for (const auto& Entry : fs::directory_iterator(CurPath))
		{
			if (Entry.is_directory())
				continue;

			auto Path = Entry.path();
			if (!Path.has_extension())
				continue;

			if (!_stricmp(".xml", Path.extension().string().c_str()))
			{
				cout_w() << "Scanning file: " << Entry.path().string() << '\n';
				FixUIXML(Entry.path().string().c_str());
			}
		}
	}
	else
	{
		FixUIXML(argv[1]);
	}

	cout_w();
	system("pause");
	return EXIT_SUCCESS;
}
