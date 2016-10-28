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
CXXFLAGS += $(OPT) -pipe -MMD -W -Wall -fPIC --std=c++11 $(INCPATH)
LDFLAGS += -lpthread -lrt
PROTOC = protoc

# source file definitions
PROTO_FILE = $(wildcard src/proto/*.proto)
PROTO_SRC = $(patsubst %.proto, %.pb.cc, $(PROTO_FILE))
PROTO_OBJ = $(patsubst $.cc, %.o, $(PROTO_SRC))

OBJS = $(PROTO_OBJ)
BIN = ins

# build all
all: $(BIN)

# dependencies

# targets

# phony
.PHONY: clean
clean:
	@rm -rf $(BIN) $(OBJS)

