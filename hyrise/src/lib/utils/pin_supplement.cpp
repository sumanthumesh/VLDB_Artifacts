#include "utils/pin_supplement.hpp"

namespace hyrise
{

// To mark ROI begin for pintool
const char *parsec_roi_begin()
{
    hyrise::log_to_file("ROIStart\n");
    return NULL;
}

// To mark ROI end for pintool
const char *parsec_roi_end()
{
    hyrise::log_to_file("ROIEnd\n");
    return NULL;
}
} // namespace hyrise
