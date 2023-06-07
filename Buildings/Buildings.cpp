
#include <stdint.h>
#include <stdio.h>

#include <fmt/color.h>

#include <array>
#include <experimental/generator>
#include <functional>
#include <ranges>
#include <set>
#include <string_view>
#include <variant>
#include <vector>

#include "Requirements.hpp"

using namespace std::literals;

using std::array;
using std::experimental::generator;
using std::set;
using std::string;
using std::string_view;
using std::variant;
using std::vector;


int32_t main(int32_t argc, char* argv[]) noexcept
{
	Requirements::UnitTest();
}
