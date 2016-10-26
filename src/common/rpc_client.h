// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: yanshiguang02@baidu.com

#ifndef  ORION_RPC_RPC_CLIENT_H
#define  ORION_RPC_RPC_CLIENT_H

#include <sofa/pbrpc/pbrpc.h>
#include <mutex>
#include <functional>
#include "common/thread_pool.h"
#include "common/logging.h"

namespace orion {
namespace rpc {

/**
 * @brief Wrapper for sofa-pbrpc interfaces,
 *        provides trivial interface and channel management
 */
class RpcClient {
public:
    RpcClient() {
        // define client object, which only one is needed for a single client program
        sofa::pbrpc::RpcClientOptions options;
        options.max_pending_buffer_size = 10;
        _client = new sofa::pbrpc::RpcClient(options);
    }
    ~RpcClient() {
        delete _client;
    }
    /// disable copy for RpcClient, move maybe needed in the future
    RpcClient(const RpcClient&) = delete;
    void operator=(const RpcClient&) = delete;

    /**
     * @brief Gets a stub to launch RPC
     * @param server [IN] sepcify the address of remote server
     * @param stub   [OUT] returns a pointer to the stub, user needs to delete it
     * @return       true if successfully created(always true for now)
     */
    template <class T>
    bool get_stub(const std::string server, T** stub) {
        std::lock_guard<std::mutex> locker(_host_map_lock);
        sofa::pbrpc::RpcChannel* channel = NULL;
        auto it = _host_map.find(server);
        if (it != _host_map.end()) {
            channel = it->second;
        } else {
            // define a channel, which represents a particular connection to server
            sofa::pbrpc::RpcChannelOptions channel_options;
            channel = new sofa::pbrpc::RpcChannel(_client, server, channel_options);
            _host_map[server] = channel;
        }
        *stub = new T(channel);
        return true;
    }

    /**
     * @brief Sends a request to remote server synchronously
     * @param stub        [IN] stub to handle the RPC
     * @param func        [IN] specify the process to call
     * @param request     [IN] user defined request proto
     * @param response    [OUT] user defined response proto
     * @param rpc_timeout [IN] set the timeout for rpc client
     * @param retry_times [IN] set the times for client to retry
     * @return            true if get proper response in retry times
     */
    template <class Stub, class Request, class Response, class Callback>
    bool send_request(Stub* stub, void(Stub::*func)(
                    google::protobuf::RpcController*,
                    const Request*, Response*, Callback*),
                    const Request* request, Response* response,
                    int32_t rpc_timeout, int retry_times) {
        // use controller to manage this RPC process
        sofa::pbrpc::RpcController controller;
        // set timeout time, default to 10s
        controller.SetTimeout(rpc_timeout * 1000L);
        for (int32_t retry = 0; retry < retry_times; ++retry) {
            (stub->*func)(&controller, request, response, NULL);
            if (controller.Failed()) {
                if (retry < retry_times - 1) {
                    LOG(WARNING, "send failed, retry ...\n");
                    usleep(1000000);
                } else {
                    LOG(WARNING, "send request fail: %s\n", controller.ErrorText().c_str());
                }
            } else {
                return true;
            }
            controller.Reset();
        }
        return false;
    }

    /**
     * @brief Sends a request to remote server asynchronously
     * @param stub        [IN] stub to handle the RPC
     * @param func        [IN] specify the process to call
     * @param request     [IN] user defined request proto
     * @param response    [OUT] user defined response proto
     * @param callback    [IN] callback function used when get proper response
     * @param rpc_timeout [IN] set the timeout for rpc client
     * @param retry_times [IN] set the times for client to retry(@deprecated)
     * @return            void
     */
    template <class Stub, class Request, class Response, class Callback>
    void async_request(Stub* stub, void(Stub::*func)(
                    google::protobuf::RpcController*,
                    const Request*, Response*, Callback*),
                    const Request* request, Response* response,
                    std::function<void (const Request*, Response*, bool, int)> callback,
                    int32_t rpc_timeout, int /*retry_times*/) {
        sofa::pbrpc::RpcController* controller = new sofa::pbrpc::RpcController();
        controller->SetTimeout(rpc_timeout * 1000L);
        google::protobuf::Closure* done = 
            sofa::pbrpc::NewClosure(&RpcClient::template rpc_callback<Request, Response, Callback>,
                                          controller, request, response, callback);
        (stub->*func)(controller, request, response, done);
    }

    /**
     * @brief Wrapper for user callback, user callback will focus on status and content
     * @param rpc_controller [IN] the controller defined to manage the connection
     * @param request        [IN] user defined request proto
     * @param response       [IN] user defined response proto
     * @param callback       [IN] user defined callback function
     * @return               void
     */
    template <class Request, class Response, class Callback>
    static void rpc_callback(sofa::pbrpc::RpcController* rpc_controller,
                            const Request* request,
                            Response* response,
                            std::function<void (const Request*, Response*, bool, int)> callback) {
        bool failed = rpc_controller->Failed();
        int error = rpc_controller->ErrorCode();
        if (failed || error) {
            if (error != sofa::pbrpc::RPC_ERROR_SEND_BUFFER_FULL) {
                LOG(WARNING, "RPC callback: %s\n", rpc_controller->ErrorText().c_str());
            }
        }
        delete rpc_controller;
        callback(request, response, failed, error);
    }
private:
    sofa::pbrpc::RpcClient* _client;
    std::map<std::string, sofa::pbrpc::RpcChannel*> _host_map;
    std::mutex _host_map_lock;
};

} // namespace rpc
} // namespace orion

#endif  // ORION_RPC_RPC_CLIENT_H

