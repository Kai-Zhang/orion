// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Shiguang Yan (yanshiguang02@baidu.com)
//         Kai Zhang (cs.zhangkai@outlook.com)

#ifndef BAIDU_ORION_COMMON_LOGGING_H
#define BAIDU_ORION_COMMON_LOGGING_H
#include <sstream>

namespace orion {
namespace common {

/// Sets the log level and level lower than this will not be displayed
void set_log_level(int level);

/// Sets the log file, default to stdout
bool set_log_file(const char* path, bool append = false);

/// Sets the warning file, default to nullptr
/// same warning log will be write to log file either
bool set_warning_file(const char* path, bool append = false);

/// Sets the max size of a single log file, default to no limit(aka 0)
/// the size is in MB
bool set_log_size(int size);

/// Sets the max number of log files, default to no limit(aka 0)
bool set_log_count(int count);

/// Sets the max size of all log files, default to no limit(aka 0)
/// the size is in MB
bool set_log_size_limit(int size);

// NO_LOGGING_FUNCTION is defined to avoid name conflict if another log library
//   is used and user need to set some parameters in this logging library
#ifndef NO_LOGGING_FUNCTION

namespace logging {

extern const int DEBUG;
extern const int INFO;
extern const int WARNING;
extern const int FATAL;

}

/**
 * @brief C-style log function
 * @param level [IN] specify the log level
 * @param fmt   [IN] string format
 * @param ...   [IN] proper parameters for format
 * @return      void
 * @notice Better not to call this function
 *         following macro automatically add filename and line number in log
 */
void log(int level, const char* fmt, ...);

/**
 * @brief Provides streaming syntax for logging
 */
class LogStream {
public:
    LogStream(int level) : _level(level) { }
    ~LogStream() {
        log(_level, "%s", _oss.str().c_str());
    }
    /// Not necessary for this stream to move or copy
    LogStream(const LogStream&) = delete;
    void operator=(const LogStream&) = delete;

    template<class T>
    LogStream& operator<<(const T& t) {
        _oss << t;
        return *this;
    }
private:
    int _level;
    std::ostringstream _oss;
};

#endif // NO_LOGGING_FUNCTION

} // namespace common

#ifndef NO_LOGGING_FUNCTION
using common::logging::DEBUG;
using common::logging::INFO;
using common::logging::WARNING;
using common::logging::FATAL;
#endif

} // namespace orion

#ifndef NO_LOGGING_FUNCTION

/// LOG macro uses C-style syntax
#define LOG(level, fmt, args...) ::orion::common::log(level, \
        "[%s:%d] " fmt, __FILE__, __LINE__, ##args)
/// LOGS macro uses stream-style syntax and will write to log buffer
/// when the stream object is destroyed
#define LOGS(level) ::orion::common::LogStream(level)

#endif // NO_LOGGING_FUNCTION

#endif  // BAIDU_ORION_COMMON_LOGGING_H

