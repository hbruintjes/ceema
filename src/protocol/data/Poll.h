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

#include <types/bytes.h>
#include <json/src/json.hpp>
#include <encoding/hex.h>
#include <protocol/data/Client.h>

using json = nlohmann::json;

namespace ceema {

    /**
     * ID used by poll owner
     */
    struct poll_id : byte_array<8> {
        using byte_array::byte_array;
    };

    /**
     * Unique poll ID, both the owner ID and poll ID
     */
    struct poll_uid : byte_array<client_id::array_size + poll_id::array_size> {
        using byte_array::byte_array;

        poll_id pid() const {
            return poll_id(slice_array<client_id::array_size, poll_id::array_size>(*this));
        }

        client_id cid() const {
            return client_id(slice_array<0, client_id::array_size>(*this));
        }
    };

    enum class PollSelection {
        SINGLE = 0x00,
        MULTIPLE = 0x01
    };

    enum class PollType {
        TEXT = 0x00
    };

    enum class PollDisclose {
        ON_CLOSE = 0x00,
        INTERMEDIATE = 0x01
    };

    enum class PollStatus {
        OPEN = 0x00,
        CLOSED = 0x01
    };

    struct PollChoice {
        std::string name;
        unsigned id;
        unsigned order;
        // Contains the votes from each of the respondees
        std::vector<bool> votes;
    };

    inline void to_json(json& j, PollChoice const& c) {
        json votes;
        for(bool v: c.votes) {
            votes.push_back(v?1:0);
        }
        j = json{{"i", c.id}, {"n", c.name}, {"o", c.order}, {"r", votes}};
    }

    inline void from_json(json const& j, PollChoice& c) {
        c.name = j["n"].get<std::string>();
        c.id = j["i"].get<unsigned>();
        c.order = j["o"].get<unsigned>();
        for(auto const& vote: j["r"]) {
            c.votes.push_back(vote.get<unsigned>()?true:false);
        }
    }

    struct Poll {
        std::string description;
        PollSelection selection;
        PollType type;
        PollDisclose disclose;
        PollStatus status;

        std::vector<PollChoice> choices;
        std::vector<client_id> respondees;
    };

    inline void to_json(json& j, Poll const& p) {
        j = json::object({
            {"d", p.description},
            {"s", static_cast<unsigned>(p.status)},
            {"a", static_cast<unsigned>(p.selection)},
            {"t", static_cast<unsigned>(p.disclose)},
            {"o", static_cast<unsigned>(p.type)},
            {"c", json::array()},
            {"p", json::array()}
        });
        json& jchoices = j["c"];
        for(auto const& choice: p.choices) {
            jchoices.push_back(json{choice});
        }
        json& participants = j["p"];
        for(auto const& responder: p.respondees) {
            participants.push_back(responder.toString());
        }
    }

    inline void from_json(json const& j, Poll& p) {
        p.description = j["d"].get<std::string>();
        p.status = static_cast<PollStatus>(j["s"].get<unsigned>());
        p.selection = static_cast<PollSelection>(j["a"].get<unsigned>());
        p.disclose = static_cast<PollDisclose>(j["t"].get<unsigned>());
        p.type = static_cast<PollType>(j["o"].get<unsigned>());

        for(json const& choice: j["c"]) {
            p.choices.push_back(choice.get<PollChoice>());
        }
        for(auto const& participant: j["p"]) {
            p.respondees.push_back(client_id::fromString(participant));
        }
    }

    struct PollVoteChoice {
        unsigned id;
        bool chosen;
    };

    inline void to_json(json& j, PollVoteChoice const& c) {
        j = json::array({c.id, c.chosen?1:0});
    }

    inline void from_json(json const& j, PollVoteChoice& c) {
        c.id = j[0].get<unsigned>();
        c.chosen = j[1].get<unsigned>()?true:false;
    }

}
