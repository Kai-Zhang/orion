// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#ifndef ORION_STORAGE_TREE_STRUCT_H
#define ORION_STORAGE_TREE_STRUCT_H
#include "storage/structure.h"
#include "proto/serialize.pb.h"
#include <vector>
#include <algorithm>
#include <chrono>

namespace orion {
namespace storage {

class DataStore;

class TreeStructure : public BasicStructure {
public:
    TreeStructure();
    virtual ~TreeStructure() { }

    virtual int32_t build(const std::string& ns, const std::string& key) {
        _ns = ns;
        _key = key;
    }

    virtual int32_t get(ValueInfo& info) const;
    virtual int32_t put(const ValueInfo& info);
    virtual int32_t remove();
    virtual StructureIterator* list() const;
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
        return get_structured_key(key.back() == '/' ? key + "/" : key + "//");
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
    std::string _ns;
    std::string _key;
    proto::DataValue _value;
};

} // namespace storage
} // namespace orion

#endif // ORION_STORAGE_TREE_STRUCT_H

