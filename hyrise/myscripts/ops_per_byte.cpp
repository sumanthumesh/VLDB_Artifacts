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

double get_ops_per_byte(std::string filename)
{
    std::ifstream fin(filename);
    std::string line;
    uint64_t num_llc_misses = 0;
    double ops = 0;
    while(std::getline(fin,line))
    {
        num_llc_misses++;
        uint64_t ins_gap = std::stoull(splitString(line,' ')[2]);
        ops += ins_gap;
    }
    return ops / (num_llc_misses*64);
}

int main(int argc, char* argv[])
{
    std::string trace_file = argv[1];
    std::string out_file;
    if(argc == 3)
        out_file = argv[2];
    else
        out_file = "oppb.txt";
    double result = get_ops_per_byte(trace_file);
    std::cout<<"OPB:"<<result<<"\n";
    return 0;
}