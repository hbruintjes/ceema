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

#include "json.hpp"
#include <api/HttpManager.h>

using json = nlohmann::json;

namespace ceema {

    class API {
        HttpManager& m_manager;

        // Builtin certificate for the api server
        static const char* api_cert;
    protected:
        API(HttpManager& manager);

        future<byte_vector> get(std::string const &url);

        future<void> get(std::string const &url, IHttpTransfer* transfer);

        future<byte_vector> post(std::string const &url, byte_vector const& data);

        future<byte_vector> postFile(std::string url, byte_vector const& data, std::string const& filename);

        void postFile(std::string url, IHttpTransfer* transfer, std::string const& filename);

        future<json> jsonGet(std::string const &url);

        future<json> jsonPost(std::string const &url, json request);

        static json checkJSONResult(json data);
    };

}
