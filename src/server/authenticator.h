// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#ifndef ORION_SERVER_AUTHENTICATOR_H
#define ORION_SERVER_AUTHENTICATOR_H
#include <string>
#include <map>
#include <mutex>

namespace orion {
namespace server {

class DataStore;

class Authenticator {
public:
    Authenticator(DataStore* store) : _underlying(store) { }
    ~Authenticator() { }
    Authenticator(const Authenticator&) = delete;
    void operator=(const Authenticator&) = delete;

    int32_t add(const std::string& user, const std::string& token);
    int32_t del(const std::string& user, const std::string& token);
    int32_t auth(const std::string& user, const std::string& token);
private:
    bool validate(const std::string& user) const;
private:
    DataStore* _underlying;
    std::mutex _mutex;
    std::map<std::string, std::string> _users;
};

} // namespace server
} // namespace orion

#endif // ORION_SERVER_AUTHENTICATOR_H

