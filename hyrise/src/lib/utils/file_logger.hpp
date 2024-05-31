#pragma once

#include <chrono>
#include <functional>
#include <tbb/concurrent_vector.h>

#include "types.hpp"
#include <fstream>

namespace hyrise
{

void log_to_file(std::string s);

} // namespace hyrise
