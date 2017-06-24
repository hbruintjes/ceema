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

#include <iostream>
#include "logging.h"

namespace ceema {

    namespace logging {

        l3pp::LogPtr loggerRoot = l3pp::Logger::getLogger("ceema");
        l3pp::LogPtr loggerNetwork = l3pp::Logger::getLogger("ceema.network");
        l3pp::LogPtr loggerSession = l3pp::Logger::getLogger("ceema.session");
        l3pp::LogPtr loggerProtocol = l3pp::Logger::getLogger("ceema.protocol");

        l3pp::Initializer l3pp_init = l3pp::Initializer::get();

        void init() {
            l3pp::SinkPtr sink = l3pp::StreamSink::create(std::clog);
            l3pp::Logger::getRootLogger()->addSink(sink);
            l3pp::Logger::getRootLogger()->setLevel(l3pp::LogLevel::TRACE);

            loggerNetwork->setLevel(l3pp::LogLevel::WARN);
        }
    }
}
