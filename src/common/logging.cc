// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Shiguang Yan (yanshiguang02@baidu.com)
//         Kai Zhang (cs.zhangkai@outlook.com)

#include "logging.h"

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syscall.h>
#include <dirent.h>
#include <stdarg.h>
#include <assert.h>
#include <string>
#include <set>
#include <queue>
#include <utility>
#include <functional>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace orion {
namespace common {

/// uses logging namespace to contains all the internal variables
namespace logging {

const int DEBUG = 2;
const int INFO = 4;
const int WARNING = 8;
const int FATAL = 16;

// global settings
int g_log_level = INFO;
int64_t g_log_size = 0;
int32_t g_log_count = 0;
FILE* g_log_file = stdout;
std::string g_log_file_name;
FILE* g_warning_file = nullptr;
int64_t g_total_size_limit = 0;

// log file names and counters
std::set<std::string> g_log_set;
int64_t g_cur_total_size = 0;

/**
 * @brief Creates a new log file, and uses current time as part of filename
 * @param append [IN] decides if the file needs truncating when file exists
 * @return       true if a new valid file is created, otherwise false
 */
bool get_new_log(bool append) {
    char buf[30];
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    const time_t seconds = tv.tv_sec;
    struct tm t;
    localtime_r(&seconds, &t);
    snprintf(buf, 30,
        "%02d-%02d.%02d:%02d:%02d.%06d",
        t.tm_mon + 1,
        t.tm_mday,
        t.tm_hour,
        t.tm_min,
        t.tm_sec,
        static_cast<int>(tv.tv_usec));
    std::string full_path(g_log_file_name + ".");
    full_path.append(buf);
    size_t idx = full_path.rfind('/');
    if (idx == std::string::npos) {
        idx = 0;
    } else {
        idx += 1;
    }
    const char* mode = append ? "ab" : "wb";
    FILE* fp = fopen(full_path.c_str(), mode);
    if (fp == nullptr) {
        return false;
    }
    if (g_log_file != stdout) {
        fclose(g_log_file);
    }
    g_log_file = fp;
    remove(g_log_file_name.c_str());
    symlink(full_path.substr(idx).c_str(), g_log_file_name.c_str());
    g_log_set.insert(full_path);
    while ((g_log_count != 0 && static_cast<int64_t>(g_log_set.size()) > g_log_count)
            || (g_total_size_limit && g_cur_total_size > g_total_size_limit)) {
        std::set<std::string>::iterator it = g_log_set.begin();
        if (it != g_log_set.end()) {
            struct stat sta;
            if (-1 == lstat(it->c_str(), &sta)) {
                return false;
            }
            remove(it->c_str());
            g_cur_total_size -= sta.st_size;
            g_log_set.erase(it++);
        } else {
            break;
        }
    }
    return true;
}

/**
 * @brief Async write log to file, friendly to efficiency
 */
class AsyncLogger {
public:
    AsyncLogger() : _work(std::bind(&AsyncLogger::async_write, this)),
            _log_buffer(new logbuf_t()), _stop(false) { }
    ~AsyncLogger() {
        _stop = true;
        _flush_cv.notify_one();
        _work.join();
        // close fd
    }
    /// this logger is not designed to move nor copy
    AsyncLogger(const AsyncLogger&) = delete;
    void operator=(const AsyncLogger&) = delete;

    /// Writes logs to a buffer, notice the work thread, and returns immediately
    void write(int log_level, const char* buffer, int32_t len) {
        std::string* log_str = new std::string(buffer, len);
        std::lock_guard<std::mutex> locker(_mutex);
        _log_buffer->push(std::make_pair(log_level, log_str));
        _flush_cv.notify_one();
    }

    /// Wakeup work thread now and write all the logs in buffer
    /// will block until all the logs have written to file
    void flush() {
        std::unique_lock<std::mutex> locker(_mutex);
        _log_buffer->push(std::make_pair(0, nullptr));
        _flush_cv.notify_one();
        _done_cv.wait(locker);
    }
private:
    /// Main work threads to deal with the log buffer
    void async_write();
    typedef std::queue< std::pair<int, std::string*> > logbuf_t;
private:
    std::mutex _mutex;
    std::condition_variable _flush_cv;
    std::condition_variable _done_cv;
    std::thread _work;
    std::unique_ptr<logbuf_t> _log_buffer;
    bool _stop;
};

void AsyncLogger::async_write() {
    int64_t log_len = 0;
    int64_t warning_len = 0;
    int64_t cur_size = 0;
    // Uses a temp buffer to reduce the lock time
    std::unique_ptr<logbuf_t> work_buffer(new logbuf_t());
    while (true) {
        while (!work_buffer->empty()) {
            int log_level = work_buffer->front().first;
            std::string* str = work_buffer->front().second;
            work_buffer->pop();
            if (g_log_file != stdout && g_log_size != 0 && str != nullptr
                    && cur_size + static_cast<int64_t>(str->length()) > g_log_size) {
                // switch log file if size of single file is large enough
                g_cur_total_size += cur_size + static_cast<int64_t>(str->length());
                get_new_log(false);
                cur_size = 0;
            }
            if (str != nullptr && !str->empty()) {
                size_t ret = fwrite(str->data(), 1, str->size(), g_log_file);
                log_len += ret;
                if (g_log_size != 0) {
                    cur_size += ret;
                }
                if (g_warning_file != nullptr && log_level >= WARNING) {
                    warning_len += fwrite(str->data(), 1, str->size(), g_warning_file);
                }
            }
            delete str;
        }
        if (log_len != 0) {
            fflush(g_log_file);
        }
        if (warning_len != 0) {
            // since warning_len will be modified only
            // when g_warning_file is not null,
            // so fflush here should be safety
            fflush(g_warning_file);
        }
        std::unique_lock<std::mutex> locker(_mutex);
        if (!_log_buffer->empty()) {
            std::swap(_log_buffer, work_buffer);
            continue;
        }
        // let flush operation return
        _done_cv.notify_all();
        if (_stop) {
            break;
        }
        _flush_cv.wait(locker);
        log_len = 0;
        warning_len = 0;
    }
}

AsyncLogger g_logger;

bool recover_history(const char* path) {
    std::string log_path(path);
    size_t idx = log_path.rfind('/');
    std::string dir = "./";
    std::string log(path);
    if (idx != std::string::npos) {
        dir = log_path.substr(0, idx + 1);
        log = log_path.substr(idx + 1);
    }
    struct dirent *entry = NULL;
    DIR *dir_ptr = opendir(dir.c_str());
    if (dir_ptr == NULL) {
        return false;
    }
    std::vector<std::string> loglist;
    while ((entry = readdir(dir_ptr)) != NULL) {
        if (std::string(entry->d_name).find(log) != std::string::npos) {
            std::string file_name = dir + std::string(entry->d_name);
            struct stat sta;
            if (-1 == lstat(file_name.c_str(), &sta)) {
                return false;
            }
            if (S_ISREG(sta.st_mode)) {
                loglist.push_back(dir + std::string(entry->d_name));
                g_cur_total_size += sta.st_size;
            }
        }
    }
    closedir(dir_ptr);
    std::sort(loglist.begin(), loglist.end());
    for (const auto& cur : loglist) {
        g_log_set.insert(cur);
    }
    while ((g_log_count != 0 && static_cast<int64_t>(g_log_set.size()) > g_log_count)
            || (g_total_size_limit != 0 && g_cur_total_size > g_total_size_limit)) {
        auto it = g_log_set.begin();
        if (it != g_log_set.end()) {
            struct stat sta;
            if (-1 == lstat(it->c_str(), &sta)) {
                return false;
            }
            remove(it->c_str());
            g_cur_total_size -= sta.st_size;
            g_log_set.erase(it++);
        }
    }
    return true;
}

} // namespace logging

void set_log_level(int level) {
    logging::g_log_level = level;
}

bool set_log_file(const char* path, bool append) {
    logging::g_log_file_name.assign(path);
    return logging::get_new_log(append);
}

bool set_warning_file(const char* path, bool append) {
    const char* mode = append ? "ab" : "wb";
    FILE* fp = fopen(path, mode);
    if (fp == nullptr) {
        return false;
    }
    if (logging::g_warning_file != nullptr) {
        fclose(logging::g_warning_file);
    }
    logging::g_warning_file = fp;
    return true;
}

bool set_log_size(int size) {
    if (size < 0) {
        return false;
    }
    logging::g_log_size = static_cast<int64_t>(size) << 20;
    return true;
}

bool set_log_count(int count) {
    if (count < 0 || logging::g_total_size_limit != 0) {
        return false;
    }
    logging::g_log_count = count;
    if (!logging::recover_history(logging::g_log_file_name.c_str())) {
        return false;
    }
    return true;
}

bool set_log_size_limit(int size) {
    if (size < 0 || logging::g_log_count != 0) {
        return false;
    }
    logging::g_total_size_limit = static_cast<int64_t>(size) << 20;
    if (!logging::recover_history(logging::g_log_file_name.c_str())) {
        return false;
    }
    return true;
}

/// uses va_list to operate parameters in log function
void logv(int log_level, const char* format, va_list ap) {
    static __thread uint64_t thread_id = 0;
    static __thread char tid_str[32];
    static __thread int tid_str_len = 0;
    // record thread id
    if (thread_id == 0) {
        thread_id = syscall(__NR_gettid);
        tid_str_len = snprintf(tid_str, sizeof(tid_str), " %5d ", static_cast<int32_t>(thread_id));
    }

    static const char level_char[] = {
        'V', 'D', 'I', 'W', 'F'
    };
    char cur_level = level_char[0];
    if (log_level < DEBUG) {
        cur_level = level_char[0];
    } else if (log_level < INFO) {
        cur_level = level_char[1];
    } else if (log_level < WARNING) {
        cur_level = level_char[2];
    } else if (log_level < FATAL) {
        cur_level = level_char[3];
    } else {
        cur_level = level_char[4];
    }

    // we try twice:
    //   the first time with a fixed-size stack allocated buffer
    //   the second time with a much larger dynamically allocated buffer
    char buffer[500];
    for (int iter = 0; iter < 2; iter++) {
        char* base;
        int bufsize;
        if (iter == 0) {
            bufsize = sizeof(buffer);
            base = buffer;
        } else {
            bufsize = 30000;
            base = new char[bufsize];
        }
        char* p = base;
        char* limit = base + bufsize;

        // first: log level as a single character
        *p++ = cur_level;
        *p++ = ' ';
        // second: current time in two rows, for date and time
        {
            struct timeval tv;
            gettimeofday(&tv, nullptr);
            struct tm t;
            localtime_r(&tv.tv_sec, &t);
            int32_t ret = snprintf(p, limit - p, "%02d/%02d %02d:%02d:%02d.%06d",
                    t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
                    static_cast<int>(tv.tv_usec));
            p += ret;
        }
        memcpy(p, tid_str, tid_str_len);
        p += tid_str_len;

        // print the message
        if (p < limit) {
            va_list backup_ap;
            va_copy(backup_ap, ap);
            p += vsnprintf(p, limit - p, format, backup_ap);
            va_end(backup_ap);
        }

        // truncate to available space if necessary
        if (p >= limit) {
            if (iter == 0) {
                continue;       // Try again with larger buffer
            } else {
                p = limit - 1;
            }
        }

        // add newline if necessary
        if (p == base || p[-1] != '\n') {
            *p++ = '\n';
        }

        assert(p <= limit);
        logging::g_logger.write(log_level, base, p - base);
        if (log_level >= FATAL) {
            // log function will abort program so flush all logs here
            logging::g_logger.flush();
        }
        if (base != buffer) {
            delete[] base;
        }
        break;
    }
}

void log(int level, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    if (level >= logging::g_log_level) {
        logv(level, fmt, ap);
    }
    va_end(ap);
    if (level >= FATAL) {
        // automatically abort program when fatal problems occur
        abort();
    }
}

} // namespace common
} // namespace orion

