// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#include "data_store.h"

#include <mutex>
#include "leveldb/db.h"

namespace orion {
namespace storage {

class DataIteratorImpl : public DataIterator {
public:
    DataIteratorImpl(leveldb::Iterator* it, const std::string& ns) :
            _it(it), _ns(ns) { }
    virtual ~DataIterator() {
        if (_it != NULL) {
            delete _it;
            _it = NULL;
        }
    }

    virtual std::string key() const;
    virtual std::string value() const;
    virtual bool done() const;
    virtual bool seek(const std::string& key);
    virtual std::string next();
private:
    leveldb::Iterator* _it;
    std::string _ns;
};

class DataStoreImpl : public DataStore {
public:
    virtual int32_t get(std::string& value, const std::string& ns,
            const std::string& key) const {
        leveldb::Status st = _db->Get(leveldb::ReadOptions(), key, &value);
        return st.ok() ? status::OK : (
                st.IsNotFound() ? status::NOT_FOUND : status::DATABASE_ERROR);
    }

    virtual int32_t put(const std::string& ns, const std::string& key,
            const std::string& value) {
        leveldb::Status st = _db->Put(leveldb::WriteOptions(), key, value);
        return st.ok() ? status::OK : status::DATABASE_ERROR;
    }

    virtual int32_t remove(const std::string& ns, const std::string& key) {
        leveldb::Status st = _db->Delete(leveldb::WriteOptions(), key);
        return st.ok() ? status::OK : status::DATABASE_ERROR;
    }

    virtual DataIterator* iter(const std::string& ns) const {
        return new DataIteratorImpl(_db->NewIterator(leveldb::ReadOptions()), ns);
    }
private:
    std::string get_key_in_ns(const std::string& ns, const std::string& key) {
        return std::string("/") + ns + "/" + key;
    }
private:
    std::mutex _mu;
    leveldb::DB* _db;
};

}
}

