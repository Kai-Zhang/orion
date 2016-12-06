// Copyright (c) 2017, Kai-Zhang
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#ifndef ORION_SERVER_AUTHENTICATOR_H
#define ORION_SERVER_AUTHENTICATOR_H
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <mutex>

namespace orion {

namespace storage {

// forward declarations
class DataStore;
class TreeStructure;

} // namespace storage

namespace server {

/// provides user auth
class Authenticator {
public:
    /// Authenticator needs an underlying storage to save user information
    Authenticator(storage::DataStore* store);
    ~Authenticator() { }
    /// disable copy and move for authenticator
    Authenticator(const Authenticator&) = delete;
    void operator=(const Authenticator&) = delete;

    int32_t add(const std::string& user, const std::string& token);
    int32_t del(const std::string& user);
    /// check if the token of user is match
    int32_t auth(const std::string& user, const std::string& token);
    /// list all existing username
    std::vector<std::string> list();
private:
    // check if a username is valid
    // a valid username should not be conflict with internal user and
    // contain no '/', better composed by letters, numbers and '_'
    bool validate(const std::string& user) const;
    // backdoor here is for internal user to access data in all namespace
    // this is used for OP management or web monitor
    bool use_backdoor(const std::string& user) const;
private:
    // user information stores under the same directory
    // which is easy to list
    static const std::string s_user_prefix;

    std::unique_ptr<storage::TreeStructure> _underlying;
    std::mutex _mutex;
    // in-memory cache to speed up user auth
    std::map<std::string, std::string> _users;
};

} // namespace server
} // namespace orion

#endif // ORION_SERVER_AUTHENTICATOR_H

