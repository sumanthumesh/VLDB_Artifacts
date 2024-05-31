#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// Structure to hold the value side of the hash map
// The field names are self explanatory
typedef struct
{
    uint64_t start_addr;
    uint64_t end_addr;
    uint8_t table_id;
    uint64_t chunk_id;
    uint8_t column_id;
} hash_table_value;

// Function to display the values in a single hash_table_value instance
void print_hash_table_value(hash_table_value h)
{
    std::cout << std::hex << h.start_addr << ","
              << h.end_addr << ","
              << std::dec << (uint32_t)h.table_id << ","
              << h.chunk_id << ","
              << (uint32_t)h.column_id << "\n";
}

// Split a string into a vector of strings
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

// Remove white space from beginning or ending of string
// Similar to strip function in python
std::string trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t\n\r");
    size_t last = str.find_last_not_of(" \t\n\r");
    if (first == std::string::npos || last == std::string::npos)
    {
        return "";
    }
    return str.substr(first, last - first + 1);
}

// Go through the details.dat file and construct a map of column name to (table_id,column_id)
std::map<std::string, std::pair<uint8_t, uint8_t>> parse_column_details(std::string &filename)
{
    std::ifstream f(filename);
    std::string line;

    std::map<std::string, std::pair<uint8_t, uint8_t>> table_details;

    while (std::getline(f, line))
    {
        auto split_string = splitString(line, ',');
        table_details[split_string[0]] = std::make_pair(static_cast<uint8_t>(std::stoi(split_string[1])), static_cast<uint8_t>(std::stoi(split_string[2])));
    }
    return table_details;
}

// Parse mem blk file and generate a hash table that we will look up later
std::map<uint64_t, hash_table_value> parse_mem_blk_file(std::string &filename)
{
    // Open file
    std::ifstream f(filename);

    // Load the table details
    // In our case a map of column name to table id and column id
    std::string details_file = std::string("details.dat");
    auto table_details = parse_column_details(details_file);

    for (auto &pair : table_details)
    {
        std::cout << pair.first << ":" << (uint32_t)pair.second.first << "," << (uint32_t)pair.second.second << "\n";
    }

    // The hash map we need to construct
    std::map<uint64_t, hash_table_value> addr_table;

    std::pair<uint8_t, uint8_t> curr_table_column_id;
    uint64_t chunk_id;
    std::string line;

    std::string column_name;

    while (std::getline(f, line))
    {
        if (line.find("Table") != std::string::npos)
        {
            column_name = trim(splitString(line, ':')[3]);
            curr_table_column_id = table_details[column_name];
            chunk_id = std::stoull(trim(splitString(splitString(line, ',')[1], ':')[1]));
            continue;
        }
        else if (line.find("-----") != std::string::npos || line.find("VEC") != std::string::npos)
        {
            continue;
        }
        auto start_addr = std::stoull(trim(splitString(line, ':')[1]), nullptr, 16);
        auto size = std::stoull(splitString(splitString(splitString(line, ':')[0], '(')[1], ')')[0]);
        auto end_addr = start_addr + size;
        addr_table[start_addr] = {start_addr, end_addr, curr_table_column_id.first, chunk_id, curr_table_column_id.second};
    }
    return addr_table;
}

hash_table_value search_addr_blocks(uint64_t addr, std::map<uint64_t, hash_table_value> &addr_blocks)
{
    // DO a binary search through keys of the addr_blocks structure to find the first occurence where key > addr
    auto it = addr_blocks.upper_bound(addr);
    if (it != addr_blocks.begin())
    {
        --it;
        if (addr >= it->second.start_addr && addr < it->second.end_addr)
        {
            return it->second;
        }
    }
    hash_table_value h = {0, 0, 0, 0, 0};
    return h;
}

// Function which takes in a given address, column and table id it belomgs to and remaps it
// NOTE, it changes addr in place
void remap(uint64_t &addr, hash_table_value &h)
{
    addr = addr | 0xa00000000000000 | (uint64_t)h.table_id << 52 | (uint64_t)h.column_id << 48;
}

// Function which goes throuhgt the roitrace.csv file and generates the new output trace file
void parse_trace_file(std::string pin_filename, std::map<uint64_t, hash_table_value> &addr_blocks, std::string out_filename)
{
    // Open roitrace file
    std::ifstream f_in(pin_filename);

    // Open the output file
    std::ofstream f_out(out_filename);

    // Go through the pin file line by line and check if it lies in the hash table
    std::string line;
    while (std::getline(f_in, line))
    {
        auto split_string = splitString(line, ',');
        uint64_t addr = std::stoull(split_string[2], nullptr, 16);
        auto h = search_addr_blocks(addr, addr_blocks);
        if (h.start_addr != 0 || h.end_addr != 0)
        {
            // Remap the address if it is found in a column
            remap(addr, h);
        }
        // Write out the new line to trace file in cxlsim format
        f_out << std::hex << "0x" << addr << " " << split_string[1] << " " << split_string[0] << "\n";
    }
}

int main(int argc, char *argv[])
{
    // Need two arguments, as specified in the error message
    if (argc != 4)
    {
        std::cerr << "Expected two args (1) mem_blk file path and (2) roitrace path, instead received " << argc << " arguments\n";
        exit(1);
    }

    std::string mem_blk_file = argv[1], pin_trace_file = argv[2], out_trace_file = argv[3];

    // Generate the hashmap based on the mem blk file
    //  The key is starting address and value is addr range and segment information
    auto start = std::chrono::system_clock::now();
    std::map<uint64_t, hash_table_value> addr_table = parse_mem_blk_file(mem_blk_file);
    auto end = std::chrono::system_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Time taken for parsing mem_blk file: " << duration.count() << "ms\n";

    print_hash_table_value(addr_table[0x7f61c3a6f630]);

    print_hash_table_value(search_addr_blocks(0x7f61c3a6f636, addr_table));

    start = std::chrono::system_clock::now();
    parse_trace_file(pin_trace_file, addr_table, out_trace_file);
    end = std::chrono::system_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time taken for parsing roitrace file: " << duration.count() << "ms\n";

    return 0;
}