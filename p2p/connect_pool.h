// Copyright 2018 The Beam Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#include "types.h"
#include "roulette.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "utility/io/errorhandling.h"

namespace beam {

class ConnectPool {
public:
    explicit ConnectPool(RandomGen& rdGen);

    void setup(PeerId thisId, io::Address thisServer, const std::vector<io::Address>& priorityPeers);

    void set_priority_peer(io::Address addr);

    void add_connect_candidate(io::Address address, uint32_t weight);

    /// Chooses peers to connect
    const std::vector<io::Address>& get_connect_candidates();

    void schedule_reconnect(io::Address address, io::ErrorCode whatHappened);

    bool is_ip_allowed(uint32_t ip);

    enum PeerReserveResult { success, peer_banned, peer_exists };

    /// returns false if id is already there
    PeerReserveResult reserve_peer_id(PeerId id, io::Address address);

    void release_peer_id(io::Address address);

private:
    io::Address _thisServer;
    std::vector<io::Address> _connectCandidates;
    std::unordered_set<io::Address> _priorityPeers;
    std::vector<io::Address> _priorityPeersUnconnected;
    std::unordered_map<io::Address, uint32_t> _otherPeers;
    std::unordered_map<PeerId, io::Address> _peersReservedById;
    std::unordered_map<io::Address, PeerId> _peersReservedByAddress;
    Roulette _connectRoulette;
};

} //namespace
