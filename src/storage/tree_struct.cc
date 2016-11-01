// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#include "tree_struct.h"

#include <memory>
#include "proto/serialize.pb.h"
#include "storage/data_store.h"
#include "common/const.h"

namespace orion {
namespace storage {

class TreeIterator : public StructureIterator {
public:
    TreeIterator(DataIterator* it, const std::string& prefix) :
            _it(it), _prefix(prefix) {
        if (done()) {
            return;
        }
        _key = get_origin_key(_it->key());
        parse_raw_value(_value, _it->value());
    }
    virtual ~TreeIterator() { }

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
        return _it->done() || (_it->key().compare(0, _prefix.length(), _prefix) != 0);
    }

    virtual StructureIterator* next() {
        _it->next();
        if (!done()) {
            _key = get_origin_key(_it->key());
            parse_raw_value(_value, _it->value());
        }
        return this;
    }

private:
    std::string get_origin_key(const std::string& structured) const {
        size_t sep = structured.find_first_of('#');
        return sep != std::string::npos ? structured.substr(sep + 1) : structured;
    }

    bool parse_raw_value(ValueInfo& info, const std::string& raw) const {
        proto::DataValue data;
        if (!data.ParseFromString(raw)) {
            return false;
        }
        info = { data.type() == proto::NODE_TEMP, data.value(), data.owner() };
        return true;
    }
private:
    std::unique_ptr<DataIterator> _it;
    std::string _prefix;
    std::string _key;
    ValueInfo _value;
};

int32_t TreeStructure::get(ValueInfo& info, const std::string& ns,
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
    info = { value.type() == proto::NODE_TEMP, value.value(), value.owner() };
    return status_code::OK;
}

int32_t TreeStructure::put(const std::string& ns, const std::string& key,
        const ValueInfo& info) {
    // prepare data value
    proto::DataValue cur_node;
    cur_node.set_value(info.value);
    cur_node.set_type(info.temp ? proto::NODE_TEMP : proto::NODE_PERMANENT);
    if (info.temp) {
        cur_node.set_owner(info.owner);
    }
    int64_t now = timestamp();
    cur_node.set_last_modified(now);
    std::string raw_value;
    if (!cur_node.SerializeToString(&raw_value)) {
        return status_code::INVALID;
    }
    // put current node to database
    int32_t ret = _underlying->put(ns, get_structured_key(key), raw_value);
    if (ret != status_code::OK) {
        return ret;
    }
    // loop to create parent node
    proto::DataValue parent_node;
    std::string parent = key;
    while ((parent = get_parent(parent)) != "") {
        // get current node value
        std::string cur_value;
        ret = _underlying->get(cur_value, ns, get_structured_key(parent));
        if (ret != status_code::OK && ret != status_code::NOT_FOUND) {
            // database error
            return ret;
        }
        if (ret == status_code::OK) {
            // node has existed, renew the modified time
            if (!parent_node.ParseFromString(cur_value)) {
                return status_code::INVALID;
            }
            parent_node.set_last_modified(now);
        } else {
            // node has not existed, create an empty node
            parent_node.CopyFrom(cur_node);
            parent_node.clear_value();
        }
        if (!parent_node.SerializeToString(&raw_value)) {
            return status_code::INVALID;
        }
        if (_underlying->put(ns, get_structured_key(parent), raw_value)
                != status_code::OK) {
            return status_code::INVALID;
        }
    }
    return status_code::OK;
}

int32_t TreeStructure::remove(const std::string& ns, const std::string& key) {
    std::unique_ptr<StructureIterator> it(list(ns, key));
    if (!it->done()) {
        return status_code::INVALID;
    }
    return _underlying->remove(ns, get_structured_key(key));
}

StructureIterator* TreeStructure::list(const std::string& ns,
        const std::string& key) const {
    auto it = _underlying->iter(ns);
    const std::string& list_key = get_list_key(key);
    return new TreeIterator(it->seek(list_key), list_key);
}

} // namespace storage
} // namespace orion

