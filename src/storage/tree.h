// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#ifndef BAIDU_ORION_STORAGE_TREE_BUILDER_H
#define BAIDU_ORION_STORAGE_TREE_BUILDER_H
#include <string>
#include <vector>

namespace orion {
namespace storage {

struct LeafInfo {
    bool temp;
    std::string key;
    std::string value;
    std::string owner;
};

class TreeBuilder {
public:
    TreeBuilder(const std::string& ns) : _ns(ns) { }
    ~TreeBuilder() { }

    int32_t build(const std::string& leaf_name);

    LeafInfo current() const;
    LeafInfo parent() const;
    std::vector<LeafInfo> children() const;

    int32_t put(const std::string& value);
    int32_t get(std::string& value) const;
    int32_t remove();
private:
    std::string _ns;
    std::string _cur_key;
    std::vector<std::string> _chain;
};

}
}

#endif

