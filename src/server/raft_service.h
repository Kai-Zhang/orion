// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#ifndef ORION_RAFT_RAFT_SERVICE_H
#define ORION_RAFT_RAFT_SERVICE_H
#include "proto/raft.pb.h"

namespace orion {
namespace raft {

class RaftService : public Raft {
public:
    RaftService();
    virtual ~RaftService();

    virtual void append(::google::protobuf::RpcController* controller,
                        const AppendEntriesRequest* request,
                        AppendEntriesResponse* response,
                        ::google::protobuf::Closure* done);
    virtual void vote(::google::protobuf::RpcController* controller,
                      const VoteRequest* request,
                      VoteResponse* response,
                      ::google::protobuf::Closure* done);
};

} // namespace raft
} // namespace orion

#endif // ORION_RAFT_RAFT_SERVICE_H

