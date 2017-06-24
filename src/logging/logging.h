/**
 * Copyright 2017 Harold Bruintjes
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <l3pp/l3pp.h>

#define LOG_TRACE(a, b) L3PP_LOG_TRACE(a, b)
#define LOG_DEBUG(a, b) L3PP_LOG_DEBUG(a, b)
#define LOG_INFO(a, b) L3PP_LOG_INFO(a, b)
#define LOG_WARN(a, b) L3PP_LOG_WARN(a, b)
#define LOG_ERROR(a, b) L3PP_LOG_ERROR(a, b)

#define LOG_DBG(m) L3PP_LOG_DEBUG(::ceema::logging::loggerRoot, m)

namespace ceema {
    namespace logging {
        /**
         * Setup logging, call before using the library
         */
        void init();

        extern l3pp::LogPtr loggerRoot;
        extern l3pp::LogPtr loggerNetwork;
        extern l3pp::LogPtr loggerSession;
        extern l3pp::LogPtr loggerProtocol;
    }
}
