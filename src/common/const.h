// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#ifndef ORION_COMMON_CONST_H
#define ORION_COMMON_CONST_H
#include <stdint.h>
#include <string>

namespace orion {

namespace status_code {

static const int32_t OK = 0;
static const int32_t DATABASE_ERROR = 1;
static const int32_t NOT_FOUND = 2;
static const int32_t INVALID = 3;
static const int32_t EXISTED = 4;

} // namespace status_code

namespace common {

static const std::string INTERNAL_NS("__internal__");

} // namespace common

} // namespace orion

#endif // ORION_COMMON_CONST_H

