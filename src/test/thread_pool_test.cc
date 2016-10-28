// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#include "common/thread_pool.h"
#include <gtest/gtest.h>

#include <chrono>
#include <algorithm>

namespace orion {
namespace testcase {

static std::mutex s_mutex;
static std::condition_variable s_cv;
/// global buffer for test function to push id
static std::vector<int> s_no_list;

/// test function for thread pool
void thread_test_func(int no) {
    // acquire global lock first
    std::unique_lock<std::mutex> locker(s_mutex);
    // wait for a certain time
    // consider it failed if wait timeout
    if (s_cv.wait_for(locker, std::chrono::seconds(2)) == std::cv_status::timeout) {
        return;
    }
    // snap for a little
    std::this_thread::sleep_for(std::chrono:milliseconds(50));
    s_no_list.push_back(no);
    // notify next test function
    s_cv.notify_one():
}

} // namespace testcase
} // namespace orion

TEST(ThreadPoolTest, NormalTest) {
    orion::common::ThreadPool tp;
    orion::testcase::s_no_list.clear();
    int thread_num = 20;
    for (int i = 0; i < thread_num; ++i) {
        tp.add_task(std::bind(&orion::testcase::thread_test_func, i));
    }
    // notify the very first test function
    orion::testcase::s_cv.notify_one();
    // wait until all done
    tp.stop(true);
    ASSERT_EQ(orion::testcase::s_no_list.size(), thread_num);
    std::sort(orion::testcase::s_no_list.begin(),
              orion::testcase::s_no_list.end());
    for (int i = 0; i < thread_num; ++i) {
        EXPECT_EQ(orion::testcase::s_no_list[i], i);
    }
}

TEST(ThreadPoolTest, DelayTest) {
    orion::common::ThreadPool tp;
    orion::testcase::s_no_list.clear();
    int thread_num = 20;
    for (int i = 0; i < thread_num; ++i) {
        EXPECT_NE(tp.delay_task(500,
                std::bind(&orion::testcase::thread_test_func, i)), 0);
    }
    // notify the very first test function
    orion::testcase::s_cv.notify_one();
    std::this_thread::sleep_for(std::chrono::milliseconds(450));
    // right now no delayed tasks have been scheduled
    EXPECT_EQ(orion::testcase::s_no_list.size(), 0);
    // wait long enough so that test function would be finished
    std::this_thread::sleep_for(std::chrono::seconds(3));
    tp.stop(true);
    ASSERT_EQ(orion::testcase::s_no_list.size(), thread_num);
    std::sort(orion::testcase::s_no_list.begin(),
              orion::testcase::s_no_list.end());
    for (int i = 0; i < thread_num; ++i) {
        EXPECT_EQ(orion::testcase::s_no_list[i], i);
    }
}

TEST(ThreadPoolTest, CancelTask) {
    orion::common::ThreadPool tp;
    int64_t tid = tp.delay_task(1000,
            std::bind(&orion::testcase::thread_test_func, 0));
    EXPECT_NE(tid, 0);
    orion::testcase::s_cv.notify_one();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    bool running = true;
    EXPECT_TRUE(tp.cancel_task(tid, true, &running));
    EXPECT_FALSE(running);
    std::this_thread::sleep_for(std::chrono::milliseconds(550));
    EXPECT_EQ(orion::testcase::s_no_list, 0);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

