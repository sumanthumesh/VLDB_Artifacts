#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifndef COALESCE_BLK_SIZE
    #define COALESCE_BLK_SIZE 0x1<<20
#endif

std::vector<std::string> splitString(const std::string &input, char delimiter)
{
    std::vector<std::string> tokens;
    std::istringstream tokenStream(input);
    std::string token;

    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }

    return tokens;
}

uint64_t round_down(uint64_t addr)
{
    uint64_t line_size = 0x20;
    return addr % line_size == 0 ? addr : addr - (addr % line_size);
}

uint64_t round_up(uint64_t addr)
{
    uint64_t line_size = 0x20;
    return addr % line_size == 0 ? addr : addr + (line_size - addr % line_size);
}

int get_matching_blk_id(std::pair<uint64_t, uint64_t> addr,
                        std::vector<std::pair<uint64_t, uint64_t>> &blks)
{
    if (blks.empty())
        return -1;
    uint64_t blk_size = COALESCE_BLK_SIZE;
    addr.first = round_down(addr.first);
    addr.second = round_up(addr.second);
    for (std::size_t i = 0; i < blks.size(); i++)
    {
        if (addr.second >= blks[i].first &&
            addr.second - blks[i].first <= blk_size)
            return i;
        else if (blks[i].second >= addr.first &&
                 blks[i].second - addr.first <= blk_size)
            return i;
        else if (addr.first >= blks[i].first && blks[i].first >= addr.second &&
                 addr.second >= blks[i].second)
            return i;
        else if (blks[i].first >= addr.first && addr.first >= blks[i].second &&
                 blks[i].second >= addr.first)
            return i;
    }
    return -1;
}

void coalesce(uint64_t start, uint64_t size,
              std::vector<std::pair<uint64_t, uint64_t>> &blks)
{
    std::pair<uint64_t, uint64_t> addr = std::make_pair(start, start + size);
    int blk_id = get_matching_blk_id(addr, blks);
    if (blk_id == -1)
    {
        blks.push_back(
            std::make_pair(round_down(addr.first), round_up(addr.second)));
        return;
    }
    if (round_up(addr.second) > blks[blk_id].second)
        blks[blk_id].second = round_up(addr.second);
    if (round_down(addr.first) < blks[blk_id].first)
        blks[blk_id].first = round_down(addr.first);
}