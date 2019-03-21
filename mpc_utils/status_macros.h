// Copyright 2019 Google LLC. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This file is a custom fork of util/task/status_macros.h. This will become
// obsolete and will be replaced once Abseil releases absl::Status.

#ifndef MPC_UTILS_STATUS_MACROS_H_
#define MPC_UTILS_STATUS_MACROS_H_

#include "mpc_utils/status.h"

namespace mpc_utils {

// Internal helper for concatenating macro values.
#define MACROS_IMPL_CONCAT_INNER_(x, y) x##y
#define MACROS_IMPL_CONCAT(x, y) MACROS_IMPL_CONCAT_INNER_(x, y)

#define RETURN_IF_ERROR(expr)          \
  do {                                      \
    const auto status = (expr);             \
    if (ABSL_PREDICT_FALSE(!status.ok())) { \
      return status;                        \
    }                                       \
  } while (0);

#define ASSIGN_OR_RETURN(lhs, rexpr) \
  ASSIGN_OR_RETURN_IMPL(             \
      MACROS_IMPL_CONCAT(_statusor, __LINE__), lhs, rexpr)

#define ASSIGN_OR_RETURN_IMPL(statusor, lhs, rexpr) \
  auto statusor = (rexpr);                               \
  if (ABSL_PREDICT_FALSE(!statusor.ok())) {              \
    return statusor.status();                            \
  }                                                      \
  lhs = std::move(statusor).ValueOrDie();

}  // namespace mpc_utils

#endif  // MPC_UTILS_STATUS_MACROS_H_
