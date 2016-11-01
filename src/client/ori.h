// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#ifndef ORION_CLIENT_SDK_ORI_H
#define ORION_CLIENT_SDK_ORI_H
#include <stdint.h>
#include <string>

namespace orion {

class ScanIterator {
public:
    virtual std::string key() = 0;
    virtual std::string value() = 0;
    virtual ScanIterator* next() = 0;
    virtual bool done() = 0;
    virtual int32_t status() = 0;

    virtual ~ScanIterator() { }
};

struct WatchParam {
    std::string key;
    std::string value;
    bool deleted;
    void* context;
}

typedef void (*watch_cb_t)(const WatchParam& param, int32_t status);
typedef void (*timeout_cb_t)(void* ctx);

class Ori {
public:
    virtual int32_t put(const std::string& key, const std::string& value, bool temp = false) = 0;
    virtual int32_t get(std::string& value, const std::string& key) = 0;
    virtual int32_t remove(const std::string& key) = 0;
    virtual ScanIterator* scan(const std::string& start, const std::string end) = 0;
    virtual ScanIterator* list(const std::string& key) = 0;
    virtual int32_t watch(const std::string& key, watch_cb_t user_callback, void* context) = 0;
    virtual int32_t lock(const std::string& key) = 0;
    virtual int32_t try_lock(const std::string& key) = 0;
    virtual int32_t unlock(const std::string& key) = 0;
    virtual int32_t login(const std::string& user, const std::string& token) = 0;
    virtual int32_t logout(const std::string& user) = 0;
    virtual int32_t enroll(const std::string& user, const std::string& token) = 0;
    virtual int32_t destroy(const std::string& user) = 0;
    virtual int32_t timeout_handler(timeout_cb_t handler) = 0;
    // show cluster
    // get stats
    virtual std::string current_session() = 0;
    virtual std::string current_user() = 0;
    virtual bool is_logged_in() = 0;

    static std::string cluster_status(int32_t cluster_status);
    static std::string status_name(int32_t return_status);

    virtual ~Ori() { }
};

} // namespace orion

#endif // ORION_CLIENT_SDK_ORI_H

