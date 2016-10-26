// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Shiguang Yan (yanshiguang02@baidu.com)
//         Kai Zhang (cs.zhangkai@outlook.com)
//

#ifndef BAIDU_ORION_COMMON_THREAD_POOL_H
#define BAIDU_ORION_COMMON_THREAD_POOL_H
#include <string>
#include <vector>
#include <deque>
#include <queue>
#include <map>
#include <sstream>
#include <functional>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

namespace orion {
namespace common {

/**
 * @brief A thread pool using C++11 threading library
 */
class ThreadPool {
public:
    ThreadPool(int32_t thread_num = 10) :
            _last_task_id(0), _running_id(0), _pending(0), _stop(false),
            _schedule_cost_sum(0), _schedule_count(0),
            _task_cost_sum(0), _task_count(0) {
        start(thread_num);
    }
    ~ThreadPool() {
        stop(false);
    }
    /// ThreadPool does not support move nor copy
    ThreadPool(const ThreadPool&) = delete;
    void operator=(const ThreadPool&) = delete;

    /// ThreadPool accepts std::function<void()> as a single task
    typedef std::function<void()> task_t;

    /// Starts all working threads, should not be called by user
    bool start(int32_t thread_num) {
        std::lock_guard<std::mutex> locker(_mutex);
        if (_threads.size() != 0) {
            return false;
        }
        _stop = false;
        for (int i = 0; i < thread_num; ++i) {
            _threads.push_back(std::thread(std::bind(&ThreadPool::main_proc, this)));
        }
        return true;
    }

    bool stop(bool wait) {
        while (wait && _pending > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        {
            std::lock_guard<std::mutex> locker(_mutex);
            _stop = true;
        }
        _cond_var.notify_all();
        for (auto& t : _threads) {
            t.join();
        }
        _threads.clear();
        return true;
    }

    void add_task(const task_t& task) {
        std::unique_lock<std::mutex> locker(_mutex);
        if (_stop) {
            return;
        }
        _normal_queue.push_back(TaskMeta(0, get_micros(), task));
        ++_pending;
        locker.unlock();
        _cond_var.notify_one();
    }

    /// Priority task will be scheduled immediately
    void add_priority_task(const task_t& task) {
        std::unique_lock<std::mutex> locker(_mutex);
        if (_stop) {
            return;
        }
        _normal_queue.push_front(TaskMeta(0, get_micros(), task));
        ++_pending;
        locker.unlock();
        _cond_var.notify_one();
    }

    /// Delays a task for a few milliseconds before scheduling it
    int64_t delay_task(int64_t delay_in_milliseconds, const task_t& task) {
        std::unique_lock<std::mutex> locker(_mutex);
        if (_stop) {
            return 0;
        }
        int64_t now = get_micros();
        int64_t exe_time = now + delay_in_milliseconds * 1000;
        TaskMeta meta(++_last_task_id, exe_time, task);
        _time_queue.push(meta);
        _latest[meta.id] = meta;
        locker.unlock();
        _cond_var.notify_one();
        return meta.id;
    }

    /**
     * @brief Cancels a delay task
     * @param task_id     [IN] task id returned by delay_task function
     * @param block       [IN] block if current task is running, default to true
     * @param is_running  [OUT] return true if the specified task is running
     * @return            false if task_id does not exist or task is running when block is false
     */
    bool cancel_task(int64_t task_id, bool block = true, bool* is_running = nullptr) {
        if (task_id == 0) {
            if (is_running != nullptr) {
                *is_running = false;
            }
            return false;
        }
        timespec ts = {0, 100000};
        do {
            std::lock_guard<std::mutex> locker(_mutex);
            // check if specified task is running
            if (_running_id != task_id) {
                auto it = _latest.find(task_id);
                if (it == _latest.end()) {
                    if (is_running != nullptr) {
                        *is_running = false;
                    }
                    return false;
                }
                _latest.erase(it);
                return true;
            } else if (!block) {
                if (is_running != nullptr) {
                    *is_running = false;
                }
                return false;
            }
        } while (nanosleep(&ts, &ts) || true);
        // never arrive here
        return true;
    }

    int64_t pending() const {
        return _pending;
    }

    /**
     * ThreadPool keeps four counters for profiling:
     *   schedule_cost_sum records the total time of task scheduling
     *   schedule_count records the times of task scheduling
     *   task_cost_sum records the total time of task running
     *   task_count records the times of task running
     * profiling_str returns a string contains 3 space-separated numbers
     *   first is the average cost of thread pool scheduling in ms
     *   second is the average time of user task in ms
     *   third is the total times of task running since latest called
     *
     * @brief Returns a string contains internal profile counters
     * @param  null
     * @return profiling string contains 3 space-separated numbers
     */
    std::string profiling_str() {
        int64_t schedule_cost_sum;
        int64_t schedule_count;
        int64_t task_cost_sum;
        int64_t task_count;
        {
            std::lock_guard<std::mutex> locker(_mutex);
            schedule_cost_sum = _schedule_cost_sum;
            _schedule_cost_sum = 0;
            schedule_count = _schedule_count;
            _schedule_count = 0;
            task_cost_sum = _task_cost_sum;
            _task_cost_sum = 0;
            task_count = _task_count;
            _task_count = 0;
        }
        std::stringstream ss;
        ss << (schedule_count == 0 ? 0 : schedule_cost_sum / schedule_count / 1000)
            << " " << (task_count == 0 ? 0 : task_cost_sum / task_count / 1000)
            << " " << task_count;
        return ss.str();
    }
private:
    /// Working process that executes user tasks
    void main_proc() {
        // loops until recevies a stop command
        while (true) {
            std::unique_lock<std::mutex> locker(_mutex);
            // wait until there is something to work on
            while (_time_queue.empty() && _normal_queue.empty() && !_stop) {
                _cond_var.wait(locker);
            }
            if (_stop) {
                break;
            }
            // check for delay tasks
            if (!_time_queue.empty()) {
                int64_t now_time = get_micros();
                auto cur_task = _time_queue.top();
                int64_t wait_time = cur_task.exe_time - now_time;
                if (wait_time <= 0) {
                    // time is up for current delay task
                    _time_queue.pop();
                    auto it = _latest.find(cur_task.id);
                    if (it != _latest.end() && it->second.exe_time == cur_task.exe_time) {
                        _schedule_cost_sum += now_time - cur_task.exe_time;
                        ++_schedule_count;
                        const auto& task_exec = cur_task.task;
                        _latest.erase(it);
                        _running_id = cur_task.id;
                        locker.unlock();
                        // execute user task here
                        task_exec();
                        int64_t end_time = get_micros();
                        locker.lock();
                        _task_cost_sum += end_time - now_time;
                        ++_task_count;
                        _running_id = 0;
                    }
                    continue;
                } else if (_normal_queue.empty() && !_stop) {
                    // no normal tasks and wait for some time to execute next delay task
                    _cond_var.wait_for(locker, std::chrono::microseconds(wait_time));
                    continue;
                }
            }
            // check for normal tasks
            if (!_normal_queue.empty()) {
                auto task_exec = _normal_queue.front().task;
                int64_t exe_time = _normal_queue.front().exe_time;
                _normal_queue.pop_front();
                --_pending;
                int64_t start_time = get_micros();
                _schedule_cost_sum += start_time - exe_time;
                ++_schedule_count;
                locker.unlock();
                // execute user task here
                task_exec();
                int64_t end_time = get_micros();
                locker.lock();
                _task_cost_sum += end_time - start_time;
                ++_task_count;
            }
        }
    }

    /// uses chrono library for time acquiring
    int64_t get_micros() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::system_clock::now().time_since_epoch()).count();
    }

    /// structure to save the meta information of a user task
    struct TaskMeta {
        int64_t id;
        int64_t exe_time;
        task_t task;
        TaskMeta() { }
        TaskMeta(int64_t id, int64_t exe_time, const task_t& task) :
                id(id), exe_time(exe_time), task(task) { }
        bool operator<(const TaskMeta& meta) const {
            return (exe_time != meta.exe_time) ?
                   (exe_time > meta.exe_time) : (id > meta.id);
        }
    };
private:
    /// global mutex for condition variable and critical area
    std::mutex _mutex;
    std::condition_variable _cond_var;
    /// pool that holds all working threads
    std::vector<std::thread> _threads;
    /// double queue holding normal tasks
    std::deque<TaskMeta> _normal_queue;
    /// priority queue for easily getting a most recent delayed task
    std::priority_queue<TaskMeta> _time_queue;
    /// for task id lookup
    std::map<int64_t, TaskMeta> _latest;
    int64_t _last_task_id;
    int64_t _running_id;
    volatile int _pending;
    bool _stop;

    int64_t _schedule_cost_sum;
    int64_t _schedule_count;
    int64_t _task_cost_sum;
    int64_t _task_count;
};

}
}

#endif

