# Copyright 2017 Harold Bruintjes
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# A self compiled CURL lib has to be installed in /3rdparty/curl, which
# is contained in .gitignore and is not trakced by git
if (USE_OWN_CURL_LIB)
    set (CURL_INCLUDE_DIRS  "${CMAKE_SOURCE_DIR}/3rdparty/curl/include")
    set (CURL_LIBRARIES     "${CMAKE_SOURCE_DIR}/3rdparty/curl/lib/libcurl.so")
endif ()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src/include/
    ${CMAKE_SOURCE_DIR}/src/
    ${CMAKE_SOURCE_DIR}/3rdparty/
    ${CMAKE_SOURCE_DIR}/3rdparty/json/src/
    ${sodium_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
)

# Write config.h
configure_file(
    ${CMAKE_SOURCE_DIR}/src/include/config.h.in
    ${CMAKE_SOURCE_DIR}/src/include/config.h
)

if (USE_OPENSSL)
    list(APPEND SSL_SOURCES api/HttpClientOpenSSL.cpp)
endif ()

if (USE_WOLFSSL)
    list(APPEND SSL_SOURCES api/HttpClientwolfSSL.cpp)
endif ()

if (USE_MBEDTLS)
    list(APPEND SSL_SOURCES api/HttpClientmbedTLS.cpp)
endif ()

add_library(ceema SHARED
        api/API.h api/API.cpp api/BlobAPI.h api/BlobAPI.cpp api/HttpClient.h api/HttpClient.cpp api/IdentAPI.h api/IdentAPI.cpp
        api/HttpManager.cpp api/HttpManager.h ${SSL_SOURCES}

        async/future.h

        contact/Account.h contact/Account.cpp contact/backup.h contact/backup.cpp contact/Contact.h contact/Contact.cpp

        encoding/base32.h encoding/base64.h encoding/crypto.h encoding/crypto.cpp encoding/hex.h
        encoding/pkcs7.h encoding/sha256.h encoding/sha256.cpp encoding/pbkdf2-sha256.h encoding/pbkdf2-sha256.c

        include/config.h

        logging/logging.h logging/logging.cpp

        protocol/protocol.h protocol/protocol.cpp protocol/session.h protocol/session.cpp
        protocol/data/Blob.h protocol/data/Client.h protocol/data/Group.h protocol/data/Poll.h
        protocol/packet/Acknowledgement.h protocol/packet/Acknowledgement.cpp protocol/packet/KeepAlive.h protocol/packet/KeepAlive.cpp
        protocol/packet/Message.h protocol/packet/Message.cpp protocol/packet/payloads/MessagePayload.h protocol/packet/payloads/MessagePayload.cpp
        protocol/packet/Packet.h protocol/packet/Packet.cpp protocol/packet/Status.h protocol/packet/Status.cpp

        socket/socket.h socket/socket.cpp

        types/bytes.h types/slice.h types/flags.h
        protocol/data/Crypto.h protocol/packet/payloads/PayloadGroupControl.cpp api/BlobTransfer.h protocol/packet/payloads/PayloadText.h protocol/packet/payloads/PayloadBlob.h protocol/packet/payloads/PayloadPoll.h protocol/packet/payloads/PayloadControl.h protocol/packet/payloads/PayloadGroupControl.h protocol/packet/payloads/PayloadGroup.h protocol/packet/payloads/PayloadGroup.cpp protocol/packet/payloads/PayloadText.cpp protocol/packet/payloads/PayloadBlob.cpp protocol/packet/payloads/PayloadPoll.cpp protocol/packet/payloads/PayloadControl.cpp types/iter.h protocol/packet/MessageFlag.h protocol/packet/MessageFlag.cpp types/formatstr.h)

set_target_properties(ceema PROPERTIES
    # create *nix style library versions + symbolic links
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION}"
    # allow creating static and shared libs without conflicts
    # CLEAN_DIRECT_OUTPUT 1
    # avoid conflicts between library and binary target names
    OUTPUT_NAME ceema
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    # we currently do not use public header option, as it doesnt support multi-directory headers
    CXX_STANDARD 14
)

target_link_libraries(ceema
    ${CURL_LIBRARIES}
    ${SSL_LIBS}
    ${sodium_LIBRARIES}
    mpark_variant
    )

if (${GLIB2_FOUND})
    include_directories(${GLIB2_INCLUDE_DIRS})

    add_library(threepl SHARED
            threepl/prpl/threepl.cpp threepl/prpl/threepl.h threepl/PrplHttpManager.cpp
            threepl/PrplHttpManager.h threepl/prpl/buddy.h threepl/prpl/buddy.cpp threepl/ThreeplConnection.cpp
            threepl/ThreeplConnection.h threepl/ThreeplImport.cpp threepl/ThreeplImport.h
            threepl/ContactStore.h threepl/MessageHandler.h threepl/MessageHandler.cpp threepl/ContactStore.cpp
            threepl/GroupStore.cpp threepl/GroupStore.h threepl/prpl/actions.h threepl/prpl/actions.cpp
            threepl/Transfer.h threepl/Transfer.cpp threepl/prpl/commands.h threepl/prpl/commands.cpp threepl/Logging.h threepl/Logging.cpp
        threepl/prpl/list.h threepl/prpl/list.cpp threepl/prpl/chat.h threepl/prpl/chat.cpp
            threepl/prpl/xfer.cpp threepl/prpl/xfer.h threepl/prpl/im.cpp threepl/prpl/im.h
            threepl/prpl/connection.h threepl/prpl/connection.cpp
            threepl/Buddy.cpp threepl/Buddy.h)
    target_link_libraries(threepl ceema zip ${GLIB2_LIBRARIES})
    set_property(TARGET threepl PROPERTY CXX_STANDARD 14)
    target_compile_definitions(threepl PRIVATE PURPLE_DISABLE_DEPRECATED=1)
endif()