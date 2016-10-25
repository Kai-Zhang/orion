#include "tree_builder.h"

#include <memory>
#include "proto/serialize.pb.h"
#include "common/const.h"

namespace orion {
namespace storage {

bool TreeBuilder::build(const std::string& leaf_name) {
    if (leaf_name.empty()) {
        return false;
    }
    if (leaf_name[0] != '/') {
        // TODO
    }
    size_t cur_pos = 1;
    DataStore* store = DataStore::get();
    int64_t last_prefix = 0;
    while (cur_pos < leaf_name.length()) {
        size_t next_sep = leaf_name.find_first_of('/', cur_pos);
        const auto& cur_layer = leaf_name.substr(cur_pos, next_sep - cur_pos);
        if (cur_layer.empty()) {
            continue;
        }
        const auto& key = std::to_string(last_prefix) + cur_layer;
        std::string raw_value;
        if (store->get(_ns, raw_value, key) == status::NOT_FOUND) {
            // TODO create inexist node
            return false;
        }
        proto::DataValue value;
        if (!value.ParseFromString(raw_value)) {
            // TODO
            return false;
        }
        _chain.push_back(key);
        last_prefix = value.prefix();
        cur_pos = next_sep + 1;
    }
    return true;
}

LeafInfo TreeBuilder::current() const {
    LeafInfo info;
    if (_chain.empty()) {
        return info;
    }
    DataStore* store = DataStore::get();
    std::string raw_value;
    if (store->get(_ns, raw_value, _chain.back() != status::OK)) {
        return info;
    }
    proto::DataValue value;
    if (!value.ParseFromString(raw_value)) {
        return info;
    }
    info.temp = value.type() == proto::NODE_TEMP;
    info.key = _cur_key;
    info.value = value.value();
    info.owner = value.owner();
    return info;
}

LeafInfo TreeBuilder::parent() const {
    LeafInfo info;
    if (_chain.size() <= 1) {
        return info;
    }
    DataStore* store = DataStore::get();
    std::string raw_value;
    if (store->get(_ns, raw_value, *(_chain.rbegin() + 1)) != status::OK) {
        return info;
    }
    proto::DataValue value;
    if (!value.ParseFromString(raw_value)) {
        return info;
    }
    info.temp = value.type() == proto::NODE_TEMP;
    info.key = _cur_key.substr(0, _cur_key.find_last_of('/'));
    info.value = value.value();
    info.owner = value.owner();
}

std::vector<LeafInfo> TreeBuilder::children() const {
    std::vector<LeafInfo> infos;
    if (_chain.empty()) {
        return infos;
    }
    DataStore* store = DataStore::get();
    std::string raw_value;
    if (store->get(_ns, raw_value, _chain.back()) != status::OK) {
        return infos;
    }
    proto::DataValue value;
    if (!value.ParseFromString(raw_value)) {
        return infos;
    }
    const auto& start_key = std::to_string(value.prefix());
    const auto& end_key = std::to_string(value.prefix() + 1);
    std::unique_ptr<DataIterator> it = store->iter(_ns);
    if (it.get() == NULL) {
        return infos;
    }
    for (it->seek(start_key); !it->done() && it->key() < end_key; it->next()) {
        if (!value.ParseFromString(it->value())) {
            continue;
        }
        LeafInfo info;
        info.temp = value.type() == proto::NODE_TEMP;
        info.key = _cur_key + it->key().substr(start_key.length());
        info.value = value.value();
        info.owner = value.owner();
    }
    return infos;
}

int32_t TreeBuilder::put(const std::string& value) {
    if (_chain.empty()) {
        // TODO
    }
    DataStore* store = DataStore::get();
    return store->put(_ns, _chain.back(), value);
}

int32_t TreeBuilder::get(std::string& value) const {
    if (_chain.empty()) {
        return status::NOT_FOUND;
    }
    DataStore* store = DataStore::get();
    return store->get(_ns, _chain.back(), value);
}

int32_t TreeBuilder::remove() {
    if (_chain.empty()) {
        return status::NOT_FOUND;
    }
    DataStore* store = DataStore::get();
    return store->remove(_ns, _chain.back());
}

}
}

