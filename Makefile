# Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Author: Kai Zhang (cs.zhangkai@outlook.com)

# import dependency path information
include depends.mk

# compile flags
OPT ?= -O2 -g2
CXX = g++
INCPATH = -I./src -I./thirdparty/leveldb/include \
		  -I$(PROTOBUF_DIR)/include -I$(SOFA_PBRPC_DIR)/include \
		  -I$(GFLAGS_DIR)/include -I$(GTEST_DIR)/include
CXXFLAGS += $(OPT) -pipe -MMD -W -Wall -fPIC --std=c++11
LDFLAGS += -lpthread -lrt -L./thirdparty/leveldb -lleveldb \
		   -L$(PROTOBUF_DIR)/lib -lprotobuf \
		   -L$(SOFA_PBRPC_DIR)/lib -lsofa-pbrpc \
		   -L$(GFLAGS_DIR)/lib -lgflags
TESTFLAGS = -L$(GTEST_DIR)/lib -lgtest
PROTOC = $(PROTOBUF_DIR)/bin/protoc

# source file definitions
PROTO_FILE = $(wildcard src/proto/*.proto)
PROTO_SRC = $(patsubst %.proto, %.pb.cc, $(PROTO_FILE))
PROTO_HEADER = $(patsubst %.proto, %.pb.h, $(PROTO_FILE))
PROTO_OBJ = $(patsubst %.cc, %.o, $(PROTO_SRC))

ORION_SRC = $(wildcard src/storage/*.cc) $(wildcard src/server/*.cc) \
			$(wildcard src/common/*.cc) $(PROTO_SRC)
ORION_OBJ = $(patsubst %.cc, %.o, $(ORION_SRC))

TEST_THREAD_POOL_SRC = src/test/thread_pool_test.cc
TEST_THREAD_POOL_OBJ = $(patsubst %.cc, %.o, $(TEST_THREAD_POOL_SRC))

TEST_TREE_STRUCT_SRC = src/test/tree_struct_test.cc \
					   src/storage/tree_struct.cc src/proto/serialize.pb.cc
TEST_TREE_STRUCT_OBJ = $(patsubst %.cc, %.o, $(TEST_TREE_STRUCT_SRC))

OBJS = $(PROTO_OBJ) $(ORION_OBJ) $(TEST_THREAD_POOL_OBJ) $(TEST_TREE_STRUCT_OBJ)
BIN = orion
TESTS = test_thread_pool test_tree_struct
DEPS = $(patsubst %.o, %.d, $(OBJS))

# build all
all: $(BIN) $(TESTS)

# dependencies
$(OBJS): $(PROTO_HEADER) $(PROTO_SRC)
-include $(DEPS)

# targets
%.pb.cc %.pb.h: %.proto
	$(PROTOC) --proto_path=./src/proto --cpp_out=./src/proto $<

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(INCPATH) -c -o $@ $<

orion: $(ORION_OBJ)
	$(CXX) $(ORION_OBJ) -o $@ $(LDFLAGS)

tests: $(TESTS)

test_thread_pool: $(TEST_THREAD_POOL_OBJ)
	$(CXX) $(TEST_THREAD_POOL_OBJ) -o $@ $(LDFLAGS) $(TESTFLAGS)

test_tree_struct: $(TEST_TREE_STRUCT_OBJ)
	$(CXX) $(TEST_TREE_STRUCT_OBJ) -o $@ $(LDFLAGS) $(TESTFLAGS)

# phony
.PHONY: clean
clean:
	@rm -rf $(BIN) $(OBJS) $(DEPS) $(TESTS)
	@rm -rf $(PROTO_SRC) $(PROTO_HEADER)

