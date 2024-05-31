#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

uint64_t temp_counter = 0;

typedef struct
{
    uint8_t table_id;
    uint64_t chunk_id;
    uint8_t column_id;
} decoded_sgmt;

void transform_address(uint64_t &addr)
{
    addr = addr | 0xa0000000000000;
}

decoded_sgmt get_seg_details(uint64_t encoded_sgmt)
{
    decoded_sgmt s;
    s.column_id = encoded_sgmt & 0x00000000000000ff;        // Only lsb 8 bits
    s.chunk_id = (encoded_sgmt & 0x00ffffffffffff00) >> 8;  // Only middle 48 bits
    s.table_id = (encoded_sgmt & 0xff00000000000000) >> 56; // Only msb 8 bits
    // std::cout<<std::hex<<encoded_sgmt<<"\n";
    // std::cout<<std::hex<<(uint64_t)s.table_id<<","<<s.chunk_id<<","<<(uint64_t)s.column_id<<"\n";
    return s;
}

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

std::vector<uint64_t> get_blk_idx(uint64_t &addr, std::vector<std::pair<uint64_t, uint64_t>> &addr_blocks, uint64_t min = 0, uint64_t max = std::numeric_limits<uint64_t>::max())
{
    std::vector<uint64_t> blk_idx;
    if (addr < min || addr > max)
        return blk_idx;
    for (size_t i = 0; i < addr_blocks.size(); i++)
    {
        if (addr >= addr_blocks[i].first && addr <= addr_blocks[i].second)
            blk_idx.push_back(i);
    }
    return blk_idx;
}

void process_file(const std::string &src_file, const std::string &dst_file, std::vector<std::pair<uint64_t, uint64_t>> &addr_blocks, std::vector<uint64_t> &mapping_index, uint64_t min = 0, uint64_t max = 0)
{
    std::ifstream in(src_file);
    std::ofstream op(dst_file);

    std::string line;
    uint64_t ins_gap, addr;

    std::cout << "Processing file " << src_file << "\n";

    while (std::getline(in, line))
    {
        if (line.find("Segment") != std::string::npos)
            continue;
        ins_gap = std::stoull(splitString(line, ',')[0]);
        addr = std::stoull(splitString(line, ',')[2], nullptr, 16);
        auto blk_idx = get_blk_idx(addr, addr_blocks);
        uint64_t selected_idx;
        // std::cout << "TID:" << blk_idx << "\n";
        if(blk_idx.size() == 1)
        {
            selected_idx = blk_idx[0];
        }
        else if(blk_idx.size() > 1)
        {
            selected_idx = blk_idx[0];
            for(size_t i=0;i<blk_idx.size();i++)
            {
                auto& ith_blk = addr_blocks[blk_idx[i]];
                auto& sel_blk = addr_blocks[selected_idx];
    
                if((ith_blk.second - ith_blk.first) < (sel_blk.second - sel_blk.first))
                {
                    selected_idx = blk_idx[i];
                }
            }
        }
        if (blk_idx.size() >= 0)
        {
            decoded_sgmt s = get_seg_details(mapping_index[selected_idx]);
            if(s.table_id == 5 && s.column_id == 3)
                temp_counter++;
            addr = addr | 0xa00000000000000 | (uint64_t)s.table_id << 52 | (uint64_t)s.column_id << 48;
        }
        op << std::hex << "0x" << addr << " " << splitString(line, ',')[1] << " " << std::dec << ins_gap << "\n";
    }
}

void parse_addr_blocks(std::string &range_file, std::vector<std::pair<uint64_t, uint64_t>> &addr_blks, std::vector<uint64_t> &mapping_index)
{
    std::ifstream f(range_file);

    std::string line;
    uint64_t start, end;
    uint64_t encoded_sgmt_info;
    while (std::getline(f, line))
    {
        if (line.find("S") != std::string::npos)
        {
            // I'm leaving 1 bytes for column id, 1 bytes for table id and remaining 6 bytes for chunk id
            std::vector<std::string> split_line = splitString(line, ',');
            uint8_t table_id = std::stoi(split_line[1]);
            uint64_t chunk_id = std::stoull(split_line[2]);
            uint8_t col_id = std::stoi(split_line[3]);
            // std::cout<<(uint64_t)table_id<<","<<chunk_id<<","<<(uint64_t)col_id<<"\n";
            // Encode it
            encoded_sgmt_info = 0x0000000000000000 | (uint64_t)table_id << 56 | chunk_id << 8 | col_id;
            // std::cout<<std::hex<<encoded_sgmt_info<<"\n";
            continue;
        }
        std::vector<std::string> split_line = splitString(line, ',');
        if (split_line.size() != 3)
        {
            std::cerr << "Some problem with this line " << line << "\n";
            exit(1);
        }
        start = std::stoull(split_line[1], nullptr, 16);
        end = std::stoull(split_line[2], nullptr, 16);
        addr_blks.push_back(std::make_pair(start, end));
        mapping_index.push_back(encoded_sgmt_info);
    }
    std::ofstream m("mapping_index.txt");
    for (size_t i = 0; i < mapping_index.size(); i++)
    {
        m << std::dec << i << ":" << std::hex << mapping_index[i] << "\n";
    }
}

std::pair<uint64_t, uint64_t> min_max(std::vector<std::pair<uint64_t, uint64_t>> &v)
{
    uint64_t min_start = std::numeric_limits<uint64_t>::max(), max_end = 0;

    for (auto &ele : v)
    {
        // Compute additional statistic
        if (ele.first < min_start)
            min_start = ele.first;
        if (ele.second > max_end)
            max_end = ele.second;
    }
    return std::make_pair(min_start, max_end);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Insuffienct arguments, first one should be path to ranges, second and onwards should be to roitrace files\n";
        exit(1);
    }
    std::string range_file(argv[1]);
    std::vector<std::string> src_files;
    for (int i = 2; i < argc; i++)
    {
        src_files.push_back(argv[i]);
    }

    std::cout << "Reading ranges file" << range_file << "\n";
    std::vector<std::pair<uint64_t, uint64_t>> addr_blks;
    std::vector<uint64_t> mapping_index;
    parse_addr_blocks(range_file, addr_blks, mapping_index);
    std::cout << "ABLKR:" << addr_blks.size() << "\n";
    std::cout << "Processed ranges file\n";
    // Computer min-max statistics to speed up comparisons
    std::pair<uint64_t, uint64_t> bounds = min_max(addr_blks);

    // This is the most time consuming part
    // Gonna try and multithread it
    std::cout << "List of files to process\n";
    for (auto &s : src_files)
        std::cout << s << "\n";

    std::vector<std::thread> thread_vec;

    // Launch all the threads
    for (size_t i = 0; i < src_files.size(); i++)
    {
        thread_vec.push_back(std::thread([i, &src_files, &addr_blks, &mapping_index, &bounds]()
                                         { process_file(src_files[i], "iso_part_" + std::to_string(i), addr_blks, mapping_index, bounds.first, bounds.second); }));
    }

    // Wait for workers to finish
    for (size_t i = 0; i < src_files.size(); i++)
    {
        thread_vec[i].join();
    }

    std::cout<<"temp counter "<<temp_counter<<"\n";

    return 0;
}