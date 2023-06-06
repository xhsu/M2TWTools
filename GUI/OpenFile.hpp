#pragma once

#include <filesystem>
#include <optional>

extern std::optional<std::filesystem::path> Win32_OpenFileDialog(wchar_t const *pszTitle, wchar_t const *pszFilter, wchar_t const *pszDesc, bool bMultiSelection = false) noexcept;
extern std::optional<std::filesystem::path> Win32_SaveFileDialog(wchar_t const *pszTitle, wchar_t const *pszDefault, wchar_t const *pszFilter, wchar_t const *pszDesc) noexcept;
extern std::optional<std::filesystem::path> Win32_SelectFolderDialog(wchar_t const* pszTitle, wchar_t const* pszDefaultPath = nullptr) noexcept;