#include <tinyfiledialogs/tinyfiledialogs.h>

#include <filesystem>
#include <optional>
#include "OpenFile.hpp"

std::optional<std::filesystem::path> Win32_OpenFileDialog(wchar_t const *pszTitle, wchar_t const *pszFilter, wchar_t const* pszDesc, bool bMultiSelection) noexcept
{
	auto const file = tinyfd_openFileDialogW(
		pszTitle,
		L"",
		1,
		&pszFilter,
		pszDesc,
		bMultiSelection
	);

	if (file)
		return file;

	return std::nullopt;
}

std::optional<std::filesystem::path> Win32_SaveFileDialog(wchar_t const *pszTitle, wchar_t const* pszDefault, wchar_t const* pszFilter, wchar_t const* pszDesc) noexcept
{
	auto const file = tinyfd_saveFileDialogW(
		pszTitle,
		pszDefault,
		1,
		&pszFilter,
		pszDesc
	);

	if (file)
		return file;

	return std::nullopt;
}
