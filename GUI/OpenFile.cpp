#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>      // For common windows data types and function headers

#include <filesystem>
#include <optional>

std::optional<std::filesystem::path> Win32_OpenFileDialog(wchar_t const *const pszFilter) noexcept
{
	wchar_t filename[MAX_PATH]{};

	OPENFILENAME ofn{};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = nullptr;  // If you have a window to center over, put its HANDLE here
	ofn.lpstrFilter = pszFilter;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = L"Select UI XML file ...";
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_LONGNAMES;

	if (GetOpenFileName(&ofn))
	{
		return filename;
	}

	return std::nullopt;
}
