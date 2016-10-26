// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#ifndef ORION_STORAGE_DATA_STORE_H
#define ORION_STORAGE_DATA_STORE_H
#include <string>
#include <map>
#include <memory>
#include <mutex>

namespace orion {
namespace storage {

/// iterator over underlying storage
class DataIterator {
public:
    virtual std::string key() const = 0;
    virtual std::string value() const = 0;
    virtual bool done() const = 0;
    virtual DataIterator* seek(const std::string& key) = 0;
    virtual DataIterator* next() = 0;

    virtual ~DataIterator() { }
};

/// interface over underlying storage, provide namespace and kv i/o
class DataStore {
public:
    virtual int32_t get(std::string& value, const std::string& ns,
            const std::string& key) const = 0;
    virtual int32_t put(const std::string& ns, const std::string& key,
            const std::string& value) = 0;
    virtual int32_t remove(const std::string& ns, const std::string& key) = 0;
    virtual DataIterator* iter(const std::string& ns) const = 0;

    virtual ~DataStore() { }
};

/// create data store as singleton
class DataStoreFactory {
public:
    static DataStore* get();
private:
    static std::unique_ptr<DataStore> _store;
};

} // namespace storage
} // namespace orion

#endif // ORION_STORAGE_DATA_STORE_H

