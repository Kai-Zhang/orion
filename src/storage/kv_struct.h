// Copyright (c) 2017, Kai-Zhang
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#ifndef ORION_STORAGE_KV_STRUCT_H
#define ORION_STORAGE_KV_STRUCT_H
#include "storage/structure.h"

#include "storage/data_store.h"
#include "proto/serialize.pb.h"
#include "common/const.h"
#include <memory>

namespace orion {
namespace storage {

/// iterator on the kv structure
class KVIterator : public StructureIterator {
public:
    KVIterator(DataIterator* it) : _it(it) { }
    virtual ~KVIterator() { }

    virtual bool temp() const {
        return _value.temp;
    }

    virtual std::string key() const {
        return _key;
    }

    virtual std::string value() const {
        return _value.value;
    }

    virtual std::string owner() const {
        return _value.owner;
    }

    virtual bool done() const {
        return _it->done();
    }

    virtual StructureIterator* next() {
        _it->next();
        if (!_it->done()) {
            _key = get_origin_key(it->key());
            proto::DataValue data;
            if (!data.ParseFromString(it->value())) {
                return this;
            }
            _value = { data.type() == proto::NODE_TEMP, data.has_value(),
                       data.value(), data.owner() };
        }
        return this;
    }

private:
    /// returns the original key of a structured key in underlying storage
    std::string get_origin_key(const std::string& structured) const {
        return structured.substr(1);
    }

private:
    std::unique_ptr<DataIterator> _it;
    std::string _key;
    ValueInfo _value;
};

/**
 * @brief Structure provides plain key-value data i/o
 */
class KVStructure : public BasicStructure {
public:
    /// the structure needs a underlying data storage
    /// and will not owner nor release this pointer
    KVStructure(DataStore* store) : _underlying(store) { }
    virtual ~KVStructure() { }

    virtual int32_t get(ValueInfo& info, const std::string& ns,
            const std::string& key) const {
        std::string raw_value;
        int32_t ret = _underlying->get(raw_value, ns, get_structured_key(key));
        if (ret != status_code::OK) {
            return ret;
        }
        proto::DataValue value;
        if (!value.ParseFromString(raw_value)) {
            return status_code::INVALID;
        }
        info = { value.type() == proto::NODE_TEMP, value.has_value(),
                 value.value(), value().owner() };
        return status_code::OK;
    }

    virtual int32_t put(const std::string& ns, const std::string& key,
            const ValueInfo& info) {
        proto::DataValue value;
        value.set_value(info.value);
        value.set_type(info.temp ? proto::NODE_TEMP : proto::NODE_PERMANENT);
        if (info.temp) {
            value.set_owner(info.owner);
        }
        value.set_last_modified(timestamp());
        std::string raw_value;
        if (!value.SerializeToString(&raw_string)) {
            return status_code::INVALID;
        }
        return _underlying->put(ns, get_structured_key(key), raw_value);
    }

    virtual int32_t remove(const std::string& ns, const std::string& key) {
        return _underlying->remove(ns, get_structured_key(key));
    }

    /**
     * @brief Returns a iterator starting from the given key
     * @param ns   [IN] namespace of the specified key
     * @param key  [IN] start key to list
     * @return     a StructureIterator pointer
     *             the iterator will automatically seek to the key
     */
    virtual StructureIterator* list(const std::string& ns,
            const std::string& key) const {
        auto it = _underlying->iter(ns);
        return new KVIterator(it->seek(get_structured_key(key)));
    }
private:
    /// returns the key used in underlying storage
    std::string get_structured_key(const std::string& key) const {
        return std::string('.') + key;
    }
private:
    DataStore* _underlying;
};

} // namespace storage
} // namespace orion

#endif // ORION_STORAGE_KV_STRUCT_H

