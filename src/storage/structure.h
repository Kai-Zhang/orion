// Copyright (c) 2017, Kai-Zhang
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#ifndef ORION_STORAGE_STRUCTURE_H
#define ORION_STORAGE_STRUCTURE_H
#include <string>
#include <chrono>

namespace orion {
namespace storage {

/// value struct to describe a node in structure
struct ValueInfo {
    // true if the node is temporary
    bool temp;
    // true if the node is not create by user
    // will be ignored when putting
    bool intermediate;
    // user defined value, empty if it is the intermediate node
    std::string value;
    // session id of the node, empty if the node is not temporary
    std::string owner;
};

/// iterator over structured data
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

/// interface of structure storage
/// using underlying data storage to provide structured data i/o
class BasicStructure {
public:
    virtual int32_t get(ValueInfo& info, const std::string& ns,
            const std::string& key) const = 0;
    virtual int32_t put(const std::string& ns, const std::string& key,
            const ValueInfo& info) = 0;
    virtual int32_t remove(const std::string& ns, const std::string& key) = 0;
    /**
     * @brief Returns a iterator using the given key,
     *        the function has different definition in different implements
     * @param ns   [IN] namespace of the data
     * @param key  [IN] key for the iterator to start
     * @return     a StructureIterator pointer over a list of data
     */
    virtual StructureIterator* list(const std::string& ns,
            const std::string& key) const = 0;

    virtual ~BasicStructure() { }
protected:
    /// timestamp used to check modification
    int64_t timestamp() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

} // namespace storage
} // namespace orion

#endif // ORION_STORAGE_STRUCTURE_H

