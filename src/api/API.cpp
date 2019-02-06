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

#include "API.h"

#include "logging/logging.h"

namespace ceema {

    API::API(HttpManager& manager) : m_manager(manager) {
        m_manager.set_cert(api_cert);
    }

    future<byte_vector> API::get(std::string const &url) {
        auto& client = m_manager.getFreeClient();
        return client.get(url);
    }

    future<void> API::get(std::string const &url, IHttpTransfer* transfer) {
        auto& client = m_manager.getFreeClient();
        return client.get(url, transfer);
    }

    future<byte_vector> API::post(std::string const &url, byte_vector const& data) {
        auto& client = m_manager.getFreeClient();
        return client.post(url, data);
    }

    void API::postFile(std::string url, IHttpTransfer* transfer, std::string const& filename) {
        auto& client = m_manager.getFreeClient();
        client.postFile(url, transfer, filename);
    }

    future<byte_vector> API::postFile(std::string url, byte_vector const& data, std::string const& filename) {
        auto& client = m_manager.getFreeClient();
        return client.postFile(url, data, filename);
    }

    future<json> API::jsonGet(std::string const& url) {
        auto& client = m_manager.getFreeClient();
        auto request_fut = client.get(url);
        auto json_fut = request_fut.next([](future<byte_vector> fut) {
            byte_vector data = fut.get();
            return checkJSONResult(json::parse(data.begin(), data.end()));
        });

        return json_fut;
    }

    future<json> API::jsonPost(std::string const& url, json request) {
        auto& client = m_manager.getFreeClient();
        std::string jsonString = request.dump();

        LOG_DBG("Submitting API request: " << request);

        auto request_fut = client.post(url, byte_vector(jsonString.begin(), jsonString.end()));
        auto json_fut = request_fut.next([](future<byte_vector> fut) {
            byte_vector data = fut.get();
            return checkJSONResult(json::parse(data.begin(), data.end()));
        });

        return json_fut;
    }

    json API::checkJSONResult(json data) {
        LOG_DBG("Got JSON data for API: " << data);
        if (data.count("success") && !data["success"].get<bool>()) {
            throw std::runtime_error(data["error"].get<std::string>().c_str());
        }

        return data;
    }

    const char* API::api_cert =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIEYTCCA0mgAwIBAgIJAM1DR/DBRFpQMA0GCSqGSIb3DQEBBQUAMH0xCzAJBgNV\n"
        "BAYTAkNIMQswCQYDVQQIEwJaSDEPMA0GA1UEBxMGWnVyaWNoMRAwDgYDVQQKEwdU\n"
        "aHJlZW1hMQswCQYDVQQLEwJDQTETMBEGA1UEAxMKVGhyZWVtYSBDQTEcMBoGCSqG\n"
        "SIb3DQEJARYNY2FAdGhyZWVtYS5jaDAeFw0xMjExMTMxMTU4NThaFw0zMjExMDgx\n"
        "MTU4NThaMH0xCzAJBgNVBAYTAkNIMQswCQYDVQQIEwJaSDEPMA0GA1UEBxMGWnVy\n"
        "aWNoMRAwDgYDVQQKEwdUaHJlZW1hMQswCQYDVQQLEwJDQTETMBEGA1UEAxMKVGhy\n"
        "ZWVtYSBDQTEcMBoGCSqGSIb3DQEJARYNY2FAdGhyZWVtYS5jaDCCASIwDQYJKoZI\n"
        "hvcNAQEBBQADggEPADCCAQoCggEBAK8GdoT7IpNC3Dz7IUGYW9pOBwx+9EnDZrkN\n"
        "VD8l3KfBHjGTdi9gQ6Nh+mQ9/yQ8254T2big9p0hcn8kjgEQgJWHpNhYnOhy3i0j\n"
        "cmlzb1MF/deFjJVtuMP3tqTwiMavpweoa20lGDn/CLZodu0Ra8oL78b6FVztNkWg\n"
        "PdiWClMk0JPPMlfLEiK8hfHE+6mRVXmi12itK1semmwyHKdj9fG4X9+rQ2sKuLfe\n"
        "jx7uFxnAF+GivCuCo8xfOesLw72vx+W7mmdYshg/lXOcqvszQQ/LmFEVQYxNaeeV\n"
        "nPSAs+ht8vUPW4sX9IkXKVgBJd1R1isUpoF6dKlUexmvLxEyf5cCAwEAAaOB4zCB\n"
        "4DAdBgNVHQ4EFgQUw6LaC7+J62rKdaTA37kAYYUbrkgwgbAGA1UdIwSBqDCBpYAU\n"
        "w6LaC7+J62rKdaTA37kAYYUbrkihgYGkfzB9MQswCQYDVQQGEwJDSDELMAkGA1UE\n"
        "CBMCWkgxDzANBgNVBAcTBlp1cmljaDEQMA4GA1UEChMHVGhyZWVtYTELMAkGA1UE\n"
        "CxMCQ0ExEzARBgNVBAMTClRocmVlbWEgQ0ExHDAaBgkqhkiG9w0BCQEWDWNhQHRo\n"
        "cmVlbWEuY2iCCQDNQ0fwwURaUDAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBBQUA\n"
        "A4IBAQARHMyIHBDFul+hvjACt6r0EAHYwR9GQSghIQsfHt8cyVczmEnJH9hrvh9Q\n"
        "Vivm7mrfveihmNXAn4WlGwQ+ACuVtTLxw8ErbST7IMAOx9npHf/kngnZ4nSwURF9\n"
        "rCEyHq179pNXpOzZ257E5r0avMNNXXDwulw03iBE21ebd00pG11GVq/I26s+8Bjn\n"
        "DKRPquKrSO4/luEDvL4ngiQjZp32S9Z1K9sVOzqtQ7I9zzeUADm3aVa/Bpaw4iMR\n"
        "1SI7o9aJYiRi1gxYP2BUA1IFqr8NzyfGD7tRHdq7bZOxXAluv81dcbz0SBX8SgV1\n"
        "4HEKc6xMANnYs/aYKjvmP0VpOvRU\n"
        "-----END CERTIFICATE-----";

}