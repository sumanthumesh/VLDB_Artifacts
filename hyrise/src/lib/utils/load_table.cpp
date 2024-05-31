#include "load_table.hpp"

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>

#include "storage/table.hpp"

#include "resolve_type.hpp"
#include "string_utils.hpp"

using namespace std::string_literals; // NOLINT

namespace hyrise
{

std::shared_ptr<Table> create_table_from_header(std::ifstream &infile,
                                                ChunkOffset chunk_size)
{
    std::string line;
    std::getline(infile, line);
    Assert(line.find('\r') == std::string::npos,
           "Windows encoding is not supported, use dos2unix");
    const auto column_names = split_string_by_delimiter(line, '|');
    std::getline(infile, line);
    auto column_types = split_string_by_delimiter(line, '|');

    auto column_nullable = std::vector<bool>{};
    for (auto &type : column_types)
    {
        auto type_nullable = split_string_by_delimiter(type, '_');
        type = type_nullable[0];

        auto nullable = type_nullable.size() > 1 && type_nullable[1] == "null";
        column_nullable.push_back(nullable);
    }

    TableColumnDefinitions column_definitions;
    const auto column_name_count = column_names.size();
    for (auto index = size_t{0}; index < column_name_count; ++index)
    {
        const auto data_type =
            data_type_to_string.right.find(column_types[index]);
        Assert(data_type != data_type_to_string.right.end(),
               std::string("Invalid data type ") + column_types[index] +
                   " for column " + column_names[index]);
        column_definitions.emplace_back(column_names[index], data_type->second,
                                        column_nullable[index]);
    }

    return std::make_shared<Table>(column_definitions, TableType::Data,
                                   chunk_size, UseMvcc::Yes);
}

std::shared_ptr<Table> create_table_from_header(const std::string &file_name,
                                                ChunkOffset chunk_size)
{
    std::ifstream infile(file_name);
    Assert(infile.is_open(), "load_table: Could not find file " + file_name);
    return create_table_from_header(infile, chunk_size);
}

std::shared_ptr<Table> load_table(const std::string &file_name,
                                  ChunkOffset chunk_size,
                                  FinalizeLastChunk finalize_last_chunk)
{
    std::ifstream infile(file_name);
    Assert(infile.is_open(), "load_table: Could not find file " + file_name);

    auto table = create_table_from_header(infile, chunk_size);

    std::string line;
    while (std::getline(infile, line))
    {
        auto string_values = split_string_by_delimiter(line, '|');
        auto variant_values = std::vector<AllTypeVariant>(string_values.size());

        const auto string_value_count = string_values.size();
        for (auto column_id = ColumnID{0}; column_id < string_value_count;
             ++column_id)
        {
            if (table->column_is_nullable(column_id) &&
                string_values[column_id] == "null")
            {
                variant_values[column_id] = NULL_VALUE;
            }
            else
            {
                resolve_data_type(
                    table->column_data_type(column_id),
                    [&](auto data_type_t)
                    {
                        using ColumnDataType =
                            typename decltype(data_type_t)::type;
                        variant_values[column_id] =
                            AllTypeVariant{boost::lexical_cast<ColumnDataType>(
                                string_values[column_id])};
                    });
            }
        }

        table->append(variant_values);

        auto mvcc_data = table->last_chunk()->mvcc_data();
        mvcc_data->set_begin_cid(ChunkOffset{table->last_chunk()->size() - 1},
                                 CommitID{0});
    }

    // All other chunks have been finalized by Table::append() when they reached
    // their capacity.
    if (!table->empty() && finalize_last_chunk == FinalizeLastChunk::Yes)
    {
        table->last_chunk()->finalize();
    }

    return table;
}

} // namespace hyrise