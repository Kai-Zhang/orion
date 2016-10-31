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
INCPATH = -I./src
CXXFLAGS += $(OPT) -pipe -MMD -W -Wall -fPIC --std=c++11
LDFLAGS += -lpthread -lrt
PROTOC = $(PROTOBUF_DIR)/bin/protoc

# source file definitions
PROTO_FILE = $(wildcard src/proto/*.proto)
PROTO_SRC = $(patsubst %.proto, %.pb.cc, $(PROTO_FILE))
PROTO_OBJ = $(patsubst %.cc, %.o, $(PROTO_SRC))

ORION_SRC = $(wildcard src/storage/*.cc) $(wildcard src/server/*.cc)
ORION_OBJ = $(patsubst %.cc, %.o, $(ORION_SRC))

OBJS = $(PROTO_OBJ) $(ORION_OBJ)
BIN = orion
DEPS = $(patsubst %.o, %.d, $(OBJS))

# build all
all: $(BIN)

# dependencies
$(OBJS): $(PROTO_HEADER) $(PROTO_SRC)
-include $(DEPS)

# targets
%.pb.cc %.pb.h: %.proto
	$(PROTOC) --proto_path=./src/proto --cpp_out=./src/proto $<

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(INCPATH) -c $< -o $@

orion: $(ORION_OBJ)
	$(CXX) $< -o $@ $(LDFLAGS)

# phony
.PHONY: clean
clean:
	@rm -rf $(BIN) $(OBJS) $(DEPS)

