// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#ifndef ORION_STORAGE_STRUCTURE_H
#define ORION_STORAGE_STRUCTURE_H
#include <string>

namespace orion {
namespace storage {

struct ValueInfo {
    bool temp;
    std::string value;
    std::string owner;
};

class StructureIterator {
public:
    virtual bool temp() const = 0;
    virtual std::string key() const = 0;
    virtual std::string value() const = 0;
    virtual std::string owner() const = 0;
    virtual bool done() const = 0;
    virtual StructureIterator* next() = 0;

    virtual ~StructureIterator() { }
};

class BasicStructure {
public:
    virtual int32_t get(ValueInfo& info, const std::string& ns,
            const std::string& key) const = 0;
    virtual int32_t put(const std::string& ns, const std::string& key,
            const ValueInfo& info) = 0;
    virtual int32_t remove(const std::string& ns, const std::string& key) = 0;
    virtual StructureIterator* list(const std::string& ns,
            const std::string& key) const = 0;

    virtual ~BasicStructure() { }
};

} // namespace storage
} // namespace orion

#endif // ORION_STORAGE_STRUCTURE_H

