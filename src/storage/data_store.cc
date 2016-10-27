// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#include "data_store.h"

#include <mutex>
#include "leveldb/db.h"
#include "common/logging.h"
#include "common/const.h"

namespace orion {
namespace storage {

class DataIteratorImpl : public DataIterator {
public:
    DataIteratorImpl(leveldb::Iterator* it, const std::string& ns) :
            _it(it), _ns(ns) { }
    virtual ~DataIteratorImpl() {
        if (_it != NULL) {
            delete _it;
            _it = NULL;
        }
    }

    virtual std::string key() const {
        return _it != NULL ? remove_ns(it->key().ToString()) : "";
    }

    virtual std::string value() const {
        return _it != NULL ? _it->value().ToString() : "";
    }

    virtual bool done() const {
        return _it != NULL ? !_it->Valid() : false;
    }

    virtual DataIterator* seek(const std::string& key) {
        if (_it != NULL) {
            _it->Seek(get_key_in_ns(_ns, key));
        }
        return this;
    }

    virtual DataIterator* next() {
        if (_it != NULL) {
            _it->Next();
        }
        return this;
    }
private:
    // The data is constructed as follows:
    // for every single ns, the kv should be:
    //   /ns/key -> value
    // for default ns, the ns should be empty:
    //   //key -> value
    std::string remove_ns(const std::string& raw_str) const {
        // ignore the first /
        size_t ns_sep = raw_str.find_first_of("/", 1);
        return ns_sep == std::string::npos ? raw_str : raw_str.substr(ns_sep + 1);
    }
    std::string get_key_in_ns(const std::string& ns, const std::string& key) const {
        return std::string("/") + ns + "/" + key;
    }
private:
    leveldb::Iterator* _it;
    std::string _ns;
};

class DataStoreImpl : public DataStore {
public:
    DataStoreImpl(leveldb::DB* db) : _db(db) { }
    virtual ~DataStoreImpl() { }

    virtual int32_t get(std::string& value, const std::string& ns,
            const std::string& key) const {
        leveldb::Status st = _db->Get(leveldb::ReadOptions(),
                get_key_in_ns(ns, key), &value);
        return st.ok() ? status_code::OK : (
                         st.IsNotFound() ? status_code::NOT_FOUND :
                                           status_code::DATABASE_ERROR);
    }

    virtual int32_t put(const std::string& ns, const std::string& key,
            const std::string& value) {
        leveldb::Status st = _db->Put(leveldb::WriteOptions(),
                get_key_in_ns(ns, key), value);
        return st.ok() ? status_code::OK : status_code::DATABASE_ERROR;
    }

    virtual int32_t remove(const std::string& ns, const std::string& key) {
        leveldb::Status st = _db->Delete(leveldb::WriteOptions(), get_key_in_ns(ns, key));
        return st.ok() ? status_code::OK : status_code::DATABASE_ERROR;
    }

    virtual DataIterator* iter(const std::string& ns) const {
        return new DataIteratorImpl(_db->NewIterator(leveldb::ReadOptions()), ns);
    }
private:
    std::string get_key_in_ns(const std::string& ns, const std::string& key) const {
        return std::string("/") + ns + "/" + key;
    }
private:
    std::mutex _mu;
    std::unique_ptr<leveldb::DB> _db;
};

DataStore* DataStoreFactory::get() {
    if (_store != nullptr) {
        return _store.get();
    }
    // TODO use proper dir path
    std::string full_name = "";//data_dir_ + "/" + name + "@db";
    leveldb::Options options;
    options.create_if_missing = true;
    // TODO compression enable?
    options.compression = leveldb::kSnappyCompression;
    // TODO specify size?
    options.write_buffer_size = 0;//FLAGS_ins_data_write_buffer_size * 1024 * 1024;
    options.block_size = 0;//FLAGS_ins_data_block_size * 1024;
    LOG(INFO, "[data]: block_size: %d, writer_buffer_size: %d", 
        options.block_size,
        options.write_buffer_size);
    leveldb::DB* current_db = NULL;
    leveldb::Status st = leveldb::DB::Open(options, full_name, &current_db);
    if (!st.ok() || current_db == nullptr) {
        // TODO maybe abort here?
        return nullptr;
    }
    _store = new DataStoreImpl(current_db);
    return _store.get();
}

} // namespace storage
} // namespace orion

