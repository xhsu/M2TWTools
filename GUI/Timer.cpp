#include "Timer.hpp"

#include <chrono>

using namespace std::literals;
using namespace std::chrono;

static decltype(high_resolution_clock::now()) g_Start;

void Timer::Start() noexcept
{
	g_Start = high_resolution_clock::now();
}

double Timer::Now() noexcept
{
	auto const delta = high_resolution_clock::now() - g_Start;
	return delta.count() * 1e-9;
}
