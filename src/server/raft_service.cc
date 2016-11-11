// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#include "raft_service.h"

namespace orion {
namespace raft {

void RaftService::append(::google::protobuf::RpcController* controller,
                         const AppendEntriesRequest* request,
                         AppendEntriesResponse* response,
                         ::google::protobuf::Closure* done) {
}

void RaftService::vote(::google::protobuf::RpcController* controller,
                       const VoteRequest* request,
                       VoteResponse* response,
                       ::google::protobuf::Closure* done) {
}

} // namespace raft
} // namespace orion

