// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#ifndef ORION_STORAGE_TREE_STRUCT_H
#define ORION_STORAGE_TREE_STRUCT_H
#include "storage/structure.h"
#include <vector>
#include <algorithm>
#include <chrono>

namespace orion {
namespace storage {

class DataStore;

class TreeStructure : public BasicStructure {
public:
    TreeStructure(DataStore* store) : _underlying(store) { }
    virtual ~TreeStructure() { }

    virtual int32_t get(ValueInfo& info, const std::string& ns,
            const std::string& key) const;
    virtual int32_t put(const std::string& ns, const std::string& key,
            const ValueInfo& info);
    virtual int32_t remove(const std::string& ns, const std::string& key);
    virtual StructureIterator* list(const std::string& ns,
            const std::string& key) const;
private:
    std::string get_structured_key(const std::string& key) const {
        int level = std::count(key.cbegin(), key.cend(), '/');
        // ignore the trailing /
        if (key.back() == '/') {
            --level;
        }
        return std::to_string(level) + '#' + key;
    }

    std::string get_list_key(const std::string& key) const {
        // add one more trailing / to increase level number
        std::string prefix = key;
        if (prefix.back() != '/') {
            prefix.push_back('/');
        }
        int level = std::count(prefix.cbegin(), prefix.cend(), '/');
        return std::to_string(level) + '#' + prefix;
    }

    std::string get_parent(const std::string& key) const {
        // ignore the possible trailing /
        size_t last_sep = key.find_last_of('/', key.length() - 2);
        return last_sep != std::string::npos ? key.substr(0, last_sep) : "";
    }

    int64_t timestamp() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch()).count();
    }
private:
    DataStore* _underlying;
};

} // namespace storage
} // namespace orion

#endif // ORION_STORAGE_TREE_STRUCT_H

