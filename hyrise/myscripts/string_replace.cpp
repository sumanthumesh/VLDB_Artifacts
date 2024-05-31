#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

bool replace_in_line(std::string &line, std::string &search, std::string &replace)
{
    std::size_t pos = line.find(search);

    if (pos != std::string::npos)
    {
        line.replace(pos, replace.length(), replace);
        return true;
    }
    return false;
}

void replace_in_file(std::string in_file, std::string out_file, std::vector<std::pair<std::string, std::string>> &snr_terms)
{
    std::ifstream in_f(in_file);
    std::ofstream out_f(out_file);
    std::string line;

    int64_t count = 0;

    while (std::getline(in_f, line))
    {
        count++;
        if (count % 100000 == 0)
            std::cout << "Completed " << count << "\n";
        if (line.find("0xa") != std::string::npos || line.find("0xb") != std::string::npos)
        {
            for (auto &s : snr_terms)
            {
                bool flag = replace_in_line(line, s.first, s.second);
                // If we have a positive replace we can stop scanning
                if (flag)
                    break;
            }
        }
        out_f << line << "\n";
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "Insufficient arguments\n";
        exit(1);
    }
    else if (argc % 2 == 0)
    {
        std::cerr << "Need pairs of search replace terms\n";
        exit(1);
    }

    std::string input_filename = std::string(argv[1]);
    std::string output_filename = std::string(argv[2]);

    std::vector<std::pair<std::string, std::string>> snr_terms;

    for (int i = 3; i < argc; i += 2)
    {
        snr_terms.push_back(std::make_pair(std::string(argv[i]), (std::string(argv[i + 1]))));
    }

    // for(size_t i = 0;i<snr_terms.size();i++)
    // {
    //     std::cout<<"Search "<<snr_terms[i].first<<" Replace "<<snr_terms[i].second<<"\n";
    // }

    std::cout << "Processing " << input_filename << " and writing to " << output_filename << "\n";
    std::cout << "Using " << snr_terms.size() << " search n replace pairs\n";
    replace_in_file(input_filename, output_filename, snr_terms);

    return 0;
}