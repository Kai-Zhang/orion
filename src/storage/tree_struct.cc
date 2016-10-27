// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#include "tree_struct.h"

#include <memory>
#include "storage/data_store.h"
#include "common/const.h"

namespace orion {
namespace storage {

class TreeIterator : public StructureIterator {
public:
    TreeIterator(DataIterator* it) : _it(it) {
        if (_it->done()) {
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
        return _it->done();
    }

    virtual StructureIterator* next() {
        _it->next();
        _key = get_origin_key(_it->key());
        parse_raw_value(_value, _it->value());
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
        info.temp = data.type() == proto::NODE_TEMP;
        info.value = data.value();
        info.owner = data.owner();
        return true;
    }
private:
    std::unique_ptr<DataIterator> _it;
    std::string _key;
    ValueInfo _value;
};

TreeStructure::TreeStructure() {
    _underlying = DataStoreFactory::get();
}

int32_t TreeStructure::get(ValueInfo& info) const {
    std::string raw_value;
    int32_t ret = _underlying->get(raw_value, _ns, get_structured_key(_key));
    if (ret != status_code::OK) {
        return ret;
    }
    return _value.ParseFromString(raw_value) ? status_code::OK : status_code::INVALID;
}

int32_t TreeStructure::put(const ValueInfo& info) {
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
    int32_t ret = _underlying->put(_ns, get_structured_key(_key), raw_value);
    if (ret != status_code::OK) {
        return ret;
    }
    // loop to create parent node
    proto::DataValue parent_node;
    std::string parent = _key;
    while ((parent = get_parent(parent)) != "") {
        // get current node value
        std::string cur_value;
        ret = _underlying->get(cur_value, _ns, get_structured_key(parent));
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
        if (_underlying->put(_ns, get_structured_key(parent), raw_value)
                != status_code::OK) {
            return status_code::INVALID;
        }
    }
}

int32_t TreeStructure::remove() {
    std::unique_ptr<DataIterator> it(_underlying->iter(_ns));
    it->seek(get_list_key(_key));
    if (!it->done()) {
        return status_code::INVALID;
    }
    return _underlying->remove(_ns, get_structured_key(_key));
}

StructureIterator* TreeStructure::list() const {
    auto it = _underlying->iter(_ns);
    return new TreeIterator(it->seek(get_list_key(_key)));
}

} // namespace storage
} // namespace orion

