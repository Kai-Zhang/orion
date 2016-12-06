// Copyright (c) 2017, Kai-Zhang
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#include "authenticator.h"

#include "storage/tree_struct.h"
#include "common/const.h"

namespace orion {
namespace server {

const std::string Authenticator::s_user_prefix("/user/");

Authenticator::Authenticator(storage::DataStore* store) :
        _underlying(new storage::TreeStructure(store)) { }

int32_t Authenticator::add(const std::string& user, const std::string& token) {
    if (!validate(user)) {
        // username is not valid
        return status_code::INVALID;
    }
    if (_users.find(user) != _users.end()) {
        // user has been registered
        return status_code::EXISTED;
    }
    storage::ValueInfo value = { false, false, token, "" };
    int32_t ret = _underlying->put(common::INTERNAL_NS, s_user_prefix + user, value);
    if (ret != status_code::OK) {
        return ret;
    }
    std::lock_guard<std::mutex> locker(_mutex);
    _users[user] = token;
    return status_code::OK;
}

int32_t Authenticator::del(const std::string& user) {
    std::unique_lock<std::mutex> locker(_mutex);
    auto it = _users.find(user);
    if (it == _users.end()) {
        // user does not exist
        return status_code::INVALID;
    }
    locker.unlock();
    int32_t ret = _underlying->remove(common::INTERNAL_NS, s_user_prefix + user);
    if (ret != status_code::OK) {
        return ret;
    }
    locker.lock();
    _users.erase(it);
    return status_code::OK;
}

int32_t Authenticator::auth(const std::string& user, const std::string& token) {
    if (use_backdoor(user)) {
        // internal users has all priviledge
        return true;
    }
    std::lock_guard<std::mutex> locker(_mutex);
    auto it = _users.find(user);
    return it != _users.cend() && it->second == token;
}

std::vector<std::string> Authenticator::list() {
    std::lock_guard<std::mutex> locker(_mutex);
    std::unique_ptr<storage::StructureIterator> it(
            _underlying->list(common::INTERNAL_NS, s_user_prefix));
    std::vector<std::string> result;
    for (; !it->done(); it->next()) {
        result.push_back(it->key());
    }
    return result;
}

bool Authenticator::validate(const std::string& user) const {
    return !user.empty() && user != common::INTERNAL_NS &&
        user.find_first_of('/') != std::string::npos;
}

bool Authenticator::use_backdoor(const std::string& user) const {
    // only internal user can have the backdoor priviledge
    return user == common::INTERNAL_NS;
}

} // namespace server
} // namespace orion

