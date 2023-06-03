#pragma once

#include <filesystem>
#include <optional>

extern std::optional<std::filesystem::path> Win32_OpenFileDialog(wchar_t const *const pszFilter = L"Any file (*.*)\0*.*\0") noexcept;
