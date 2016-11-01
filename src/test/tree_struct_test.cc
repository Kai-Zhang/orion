// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#include "storage/tree_struct.h"
#include <gtest/gtest.h>

#include <map>
#include <vector>
#include <algorithm>
#include <memory>
#include "storage/data_store.h"
#include "common/const.h"

namespace orion {
namespace testcase {

/// Mock the data iterator to match mock data store
class MockDataIterator : public storage::DataIterator {
public:
    typedef std::map<std::string, std::string> pool_t;
    MockDataIterator(pool_t& data) : _pool(data), _cur(_pool.end()) { }
    virtual ~MockDataIterator() { }

    virtual std::string key() const {
        return _cur->first;
    }
    virtual std::string value() const {
        return _cur->second;
    }
    virtual bool done() const {
        return _cur == _pool.end();
    }
    virtual DataIterator* seek(const std::string& key) {
        for (auto it = _pool.begin(); it != _pool.end(); ++it) {
            if (it->first >= key) {
                _cur = it;
                break;
            }
        }
        return this;
    }
    virtual DataIterator* next() {
        ++_cur;
        return this;
    }
private:
    pool_t& _pool;
    pool_t::iterator _cur;
};

/// Mock the data store, provide in-memory storage
class MockDataStore : public storage::DataStore {
public:
    MockDataStore() { }
    virtual ~MockDataStore() { }

    virtual int32_t get(std::string& value, const std::string& ns,
            const std::string& key) const {
        auto it = _store.find(ns);
        if (it == _store.end()) {
            return status_code::NOT_FOUND;
        }
        auto jt = it->second.find(key);
        if (jt == it->second.end()) {
            return status_code::NOT_FOUND;
        }
        value = jt->second;
        return status_code::OK;
    }
    virtual int32_t put(const std::string& ns, const std::string& key,
            const std::string& value) {
        _store[ns][key] = value;
        return status_code::OK;
    }
    virtual int32_t remove(const std::string& ns, const std::string& key) {
        auto it = _store.find(ns);
        if (it == _store.end()) {
            return status_code::NOT_FOUND;
        }
        auto jt = it->second.find(key);
        if (jt == it->second.end()) {
            return status_code::NOT_FOUND;
        }
        it->second.erase(jt);
        return status_code::OK;
    }
    virtual storage::DataIterator* iter(const std::string& ns) const {
        auto it = _store.find(ns);
        if (it == _store.end()) {
            return nullptr;
        }
        auto map_ptr = const_cast< std::map<std::string, std::string>* >(&it->second);
        return new MockDataIterator(*map_ptr);
    }
private:
    // data structure is (ns, [<key, value>]...)
    std::map< std::string, std::map<std::string, std::string> > _store;
};

} // namespace testcase
} // namespace orion

TEST(TreeStructureTest, NormalTest) {
    std::unique_ptr<orion::storage::TreeStructure> tree(
            new orion::storage::TreeStructure(new orion::testcase::MockDataStore()));
    orion::storage::ValueInfo value;
    value.temp = false;
    value.owner = "myself"; // should be ignored

    // The tree will be like:
    //   / - /testa - /testaa - /testaaa
    //     |        |         +- /testaab
    //     |        +- /testab - /testaba
    //     +- /testb - /testba - /testbaa
    //     |                   +- /testbab
    //     +- /testc - /testca - /testcaa
    value.value = "/";
    EXPECT_EQ(tree->put("test", value.value, value), orion::status_code::OK);
    value.value = "/testa/testaa/testaaa";
    EXPECT_EQ(tree->put("test", value.value, value), orion::status_code::OK);
    value.value = "/testa/testaa/testaab";
    EXPECT_EQ(tree->put("test", value.value, value), orion::status_code::OK);
    value.value = "/testa/testab/testaba";
    EXPECT_EQ(tree->put("test", value.value, value), orion::status_code::OK);
    value.value = "/testb/testba/testbaa";
    EXPECT_EQ(tree->put("test", value.value, value), orion::status_code::OK);
    value.value = "/testb/testba/testbab";
    EXPECT_EQ(tree->put("test", value.value, value), orion::status_code::OK);
    value.value = "/testc/testca/testcaa";
    EXPECT_EQ(tree->put("test", value.value, value), orion::status_code::OK);

    // normal query
    EXPECT_EQ(tree->get(value, "test", "/testa/testaa/testaab"), orion::status_code::OK);
    EXPECT_FALSE(value.temp);
    EXPECT_EQ(value.owner, "");
    EXPECT_EQ(value.value, "/testa/testaa/testaab");

    // query a node which is automatically created
    // node should contains empty value and query should be succeeded
    EXPECT_EQ(tree->get(value, "test", "/testa/testaa"), orion::status_code::OK);
    EXPECT_FALSE(value.temp);
    EXPECT_EQ(value.owner, "");
    EXPECT_EQ(value.value, "");

    // query an inexist node
    EXPECT_EQ(tree->get(value, "test", "/testc/testaa"), orion::status_code::NOT_FOUND);

    // query an inexist node with much more depth
    std::string long_key = "/testa/testaa/testaaa/testaaaa/testaaaa/testaaaa";
    EXPECT_EQ(tree->get(value, "test", long_key), orion::status_code::NOT_FOUND);

    // different level should be independent
    value.value = "/testa/testaa/testaa";
    EXPECT_EQ(tree->put("test", value.value, value), orion::status_code::OK);
    value.value = "";
    EXPECT_EQ(tree->get(value, "test", "/testa/testaa/testaa"), orion::status_code::OK);
    EXPECT_FALSE(value.temp);
    EXPECT_EQ(value.owner, "");
    EXPECT_EQ(value.value, "/testa/testaa/testaa");

    // normal list
    std::vector<std::string> result;
    for (std::unique_ptr<orion::storage::StructureIterator>
            it(tree->list("test", "/")); !it->done(); it->next()) {
        EXPECT_FALSE(it->temp());
        EXPECT_EQ(it->owner(), "");
        EXPECT_TRUE(it->value().empty() || it->key() == it->value());
        result.push_back(it->key());
    }
    EXPECT_EQ(result.size(), 3);
    std::sort(result.begin(), result.end());
    EXPECT_EQ(result[0], "/testa");
    EXPECT_EQ(result[1], "/testb");
    EXPECT_EQ(result[2], "/testc");

    // list a leaf node
    std::unique_ptr<orion::storage::StructureIterator>
        leaf_it(tree->list("test", "/testa/testaa/testaaa"));
    EXPECT_TRUE(leaf_it->done());

    // list an inexist node
    std::unique_ptr<orion::storage::StructureIterator>
        empty_it(tree->list("test", "/testf"));
    EXPECT_TRUE(empty_it->done());

    // remove a normal leaf
    EXPECT_EQ(tree->remove("test", "/testa/testaa/testaaa"), orion::status_code::OK);
    EXPECT_EQ(tree->get(value, "test", "/testa/testaa/testaaa"), orion::status_code::NOT_FOUND);

    // try to remove a non-empty directory
    EXPECT_EQ(tree->remove("test", "/testc"), orion::status_code::INVALID);
    EXPECT_EQ(tree->get(value, "test", "/testc"), orion::status_code::OK);

    // try to remove an inexist node
    EXPECT_EQ(tree->remove("test", "/testf"), orion::status_code::NOT_FOUND);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

