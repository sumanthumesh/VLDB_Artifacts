#pragma once

#include "storage/table.hpp"

namespace hyrise
{

/**
 * Indicates whether the comparison of two tables should happen order sensitive
 * (Yes) or whether it should just be checked whether both tables contain the
 * same rows, independent of order.
 */
enum class OrderSensitivity
{
    Yes,
    No
};

/**
 * "Strict" enforces that both tables have precisely the same column types,
 * "Lenient" allows float instead of double, double instead of float, long
 * instead of int, int instead of long. We need this for comparing Hyrise with
 * SQLite since the column types of the latter might differ from Hyrise's.
 */
enum class TypeCmpMode
{
    Strict,
    Lenient
};

/**
 * When comparing tables generated by Hyrise to those generated by, e.g. SQLite,
 * minor differences are to be expected (since sqlite uses double for
 * arithmetics, Hyrise might use float) so for large numbers
 * FloatComparisonMode::RelativeDifference is better since it allows derivation
 * independent of the absolute value. When checking against manually generated
 * tables, FloatComparisonMode::AbsoluteDifference is the better choice.
 */
enum class FloatComparisonMode
{
    RelativeDifference,
    AbsoluteDifference
};

/*
 * As SQLite has a weird type concept, we ignore (NOT) NULL constraints for
 * tables retrieved from SQLite.
 */

enum class IgnoreNullable
{
    Yes,
    No
};

/**
 * Helper method to compare two segments for equality. Function creates
 * temporary tables and uses the check_table_equals method. As NULLable
 * information is stored in the table, not in the segment, IgnoreNullable::Yes
 * is implied.
 */
bool check_segment_equal(
    const std::shared_ptr<AbstractSegment> &actual_segment,
    const std::shared_ptr<AbstractSegment> &expected_segment,
    OrderSensitivity order_sensitivity, TypeCmpMode type_cmp_mode,
    FloatComparisonMode float_comparison_mode);

// Compares two tables for equality
// @return  A human-readable description of the table-mismatch, if any
//          std::nullopt if the Tables are the same
std::optional<std::string>
check_table_equal(const std::shared_ptr<const Table> &actual_table,
                  const std::shared_ptr<const Table> &expected_table,
                  OrderSensitivity order_sensitivity, TypeCmpMode type_cmp_mode,
                  FloatComparisonMode float_comparison_mode,
                  IgnoreNullable ignore_nullable);

} // namespace hyrise
