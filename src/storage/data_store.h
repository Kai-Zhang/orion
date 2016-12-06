// Copyright (c) 2017, Kai-Zhang
// Use of this source code is governed by a MIT license that can be
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
    // seek to position whose key is equal or first greater than provided key
    // this method should be called before other method
    // lack of calling seek may lead to undefined behaviour
    virtual DataIterator* seek(const std::string& key) = 0;
    virtual DataIterator* next() = 0;

    virtual ~DataIterator() { }
};

/// interface of underlying storage, provide namespace and kv i/o
class DataStore {
public:
    virtual int32_t get(std::string& value, const std::string& ns,
            const std::string& key) const = 0;
    virtual int32_t put(const std::string& ns, const std::string& key,
            const std::string& value) = 0;
    virtual int32_t remove(const std::string& ns, const std::string& key) = 0;
    /**
     * @brief Returns DataIterator for a certain namespace
     * @param ns  [IN] namespace of the data
     * @return    DataIterator pointer which needs to call seek first
     */
    virtual DataIterator* iter(const std::string& ns) const = 0;

    virtual ~DataStore() { }
};

/// create data store as singleton
class DataStoreFactory {
public:
    static DataStore* get();
private:
    static std::unique_ptr<DataStore> _s_store;
};

} // namespace storage
} // namespace orion

#endif // ORION_STORAGE_DATA_STORE_H

