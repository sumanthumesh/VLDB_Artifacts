#include "abstract_segment.hpp"
#include "operators/print.hpp"
#include "utils/file_logger.hpp"

namespace hyrise
{

AbstractSegment::AbstractSegment(const DataType data_type)
    : _data_type(data_type)
{
#ifdef NEW_SEG_PRINTS
    log_to_file("New ABS Segment\n");
#endif
}

DataType AbstractSegment::data_type() const { return _data_type; }

} // namespace hyrise
