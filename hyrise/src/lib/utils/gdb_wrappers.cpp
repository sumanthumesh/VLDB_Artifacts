#include "utils/gdb_wrappers.hpp"
#include "hyrise.hpp"

using namespace hyrise;

void gdb_acctr()
{
    // Access storate manager
    auto &storage_manager = Hyrise::get().storage_manager;
    // From the storage manager try to access the table pointers and print out
    // the data if you can access them
    std::vector<std::string> table_names;
    table_names.emplace_back("customer");
    table_names.emplace_back("orders");
    table_names.emplace_back("lineitem");
    table_names.emplace_back("part");
    table_names.emplace_back("partsupp");
    table_names.emplace_back("supplier");
    table_names.emplace_back("nation");
    table_names.emplace_back("region");

    std::ofstream f("access_counters_" + std::to_string(getpid()) + ".txt", std::ios::app);

    f << "-----\n";

    // For every table print out some stuff
    for (std::string &table_name : table_names)
    {
        std::vector<std::string> field_names = storage_manager.get_table(table_name)->column_names();
        // Now print out all the segments in each table
        // Iterate over all chunks
        auto num_chunks = storage_manager.get_table(table_name)->chunk_count();
        for (auto chunk_id = ChunkID{0}; chunk_id < num_chunks; chunk_id++)
        {
            auto chunk_ptr = storage_manager.get_table(table_name)->get_chunk(chunk_id);
            // Iterate over all segments
            auto col_count = chunk_ptr->column_count();
            for (auto col_id = ColumnID{0}; col_id < col_count; col_id++)
            {
                auto segment_ptr = chunk_ptr->get_segment(col_id);
                if (const auto &encoded_segment = std::dynamic_pointer_cast<AbstractEncodedSegment>(segment_ptr))
                {
                    const auto &segment = std::dynamic_pointer_cast<AbstractSegment>(segment_ptr);
                    switch (encoded_segment->encoding_type())
                    {
                    case EncodingType::Unencoded:
                    {
                        Fail("An actual segment should never have this type");
                    }
                    case EncodingType::Dictionary:
                    {
                        f << table_name << "," << chunk_id << "," << field_names[col_id] << "," << segment->access_counter.to_string() << "\n";
                        break;
                    }
                    default:
                        break;
                    }
                }
                else
                {
                    Fail("Unknown segment type");
                }
            }
        }
    }
}