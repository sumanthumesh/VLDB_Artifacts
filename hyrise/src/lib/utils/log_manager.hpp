#pragma once

#include <chrono>
#include <functional>
#include <tbb/concurrent_vector.h>

#include "types.hpp"

namespace hyrise
{

enum class LogLevel
{
    Debug,
    Info,
    Warning
};

struct LogEntry
{
    // We need system_clock here to provide human readable timestamps in
    // MetaLogTable.
    std::chrono::system_clock::time_point timestamp;
    LogLevel log_level;
    std::string reporter;
    std::string message;
};

class LogManager : public Noncopyable
{
  public:
    void add_message(const std::string &reporter, const std::string &message,
                     const LogLevel log_level = LogLevel::Debug);

    const tbb::concurrent_vector<LogEntry> &log_entries() const;

  protected:
    friend class Hyrise;
    friend class LogManagerTest;

  private:
    tbb::concurrent_vector<LogEntry> _log_entries;
};

} // namespace hyrise
