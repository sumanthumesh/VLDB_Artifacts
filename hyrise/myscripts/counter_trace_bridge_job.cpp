#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// Function to split strings
std::vector<std::string> splitString(const std::string &str, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream stream(str);

    // Use getline to split the string based on delimiter
    while (std::getline(stream, token, delimiter))
    {
        tokens.push_back(token);
    }

    return tokens;
}

// Load table column details
std::map<std::string, std::string> load_table_details(std::string filepath)
{
    std::map<std::string, std::string> table_details;
    std::ifstream f(filepath);
    std::string line;
    while (std::getline(f, line))
    {
        auto split_string = splitString(line, ',');
        std::string column_name = split_string[0];
        uint table_id = std::stoul(split_string[1]);
        uint column_id = std::stoul(split_string[2]);
        std::stringstream search_key;
        search_key << std::hex << table_id << column_id;
        table_details.insert({search_key.str(), column_name});
    }
    return table_details;
}

std::map<std::string, uint64_t> count_columns(std::string filepath, std::map<std::string, std::string> table_details, std::string out_file)
{
    std::map<std::string, uint64_t> counter;
    std::ifstream f(filepath);
    std::ofstream outf(out_file);
    if (!outf.is_open())
    {
        std::cerr << "Unable to open " << out_file << "\n";
        exit(1);
    }
    std::string line;
    std::map<std::string, bool> mapping; // true means CXL, false means DAM
    // Initialize the mapping to all CXL
    for (auto it = table_details.begin(); it != table_details.end(); it++)
    {
        mapping[it->first] = true;
    }
    while (std::getline(f, line))
    {
        // The address is the first space separated value
        // We mainly need first three characters to find out if it is a table or non table access
        std::string sub_str = line.substr(0, 3);
        // We should know if it is a table or non table access immediately
        if (sub_str != "0xa" && sub_str != "0xb")
        {
            continue;
        }

        std::string search_key = line.substr(3, 3);

        switch (line[2])
        {
        case 'a':
            mapping[search_key] = false;
            break;
        case 'b':
            mapping[search_key] = true;
            break;
        default:
            std::cerr << "Incorrect char in line " << line << "\n";
        }

        // Update counters
        if (counter.count(search_key) == 0)
            counter[search_key] = 1;
        else
            counter[search_key]++;
    }
    // Print output
    for (auto it = table_details.begin(); it != table_details.end(); it++)
    {
        outf << it->second << "," << counter[it->first] << "," << (mapping[it->first] ? "CXL" : "DAM") << "\n";
    }
    return counter;
}

int main(int argc, char *argv[])
{
    std::string filepath = argv[1];
    std::string output_file;
    if (argc == 3)
        output_file = argv[2];
    else
        output_file = "column_count.csv";

    auto table_details = load_table_details("job_columns.dat");
    count_columns(filepath, table_details, output_file);
    return 0;
}