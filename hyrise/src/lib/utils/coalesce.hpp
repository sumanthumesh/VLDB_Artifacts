#include <cstdint>
#include <string>
#include <vector>

void coalesce(uint64_t start, uint64_t size,
              std::vector<std::pair<uint64_t, uint64_t>> &blks);