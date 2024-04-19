#include "protocol/participant.h"

#include <fstream>
#include <thread>

const std::string serverName = "server";
const std::string rightNeighborName = "right";
const std::string leftNeighborName = "left";

// Initialize the participant
void Participant::Initialize() {
    if (role() == Role::client) {
        InitializeClient();
    } else if (role() == Role::server) {
        InitializeServer();
    }

    DistributedKeyGeneration();
    endpoint_->ResetCounters();
}

// Initialize the client participant
void Participant::InitializeClient() {
    // Connect to the server
    for(int i = 0; i < options_.concurrency_level; i++){
        endpoint_->Connect(serverName + "_" + std::to_string(i), options_.server_address, options_.local_name + "_" + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Connect to the right neighbor
    for(int i = 0; i < options_.concurrency_level; i++){
        endpoint_->Connect(rightNeighborName + "_" + std::to_string(i), options_.right_neighbor_address, leftNeighborName + "_" + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }


    // Wait for all connections to be established
    uint32 numConn = 3 * options_.concurrency_level;
    while (endpoint_->GetRemoteNames().size() < numConn) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    endpoint_->StopListen();
}

// Initialize the server participant
void Participant::InitializeServer() {

     while (endpoint_->GetRemoteNames().size() < options_.num_parties* options_.concurrency_level) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    // Connect to the right neighbor
    for(int i = 0; i < options_.concurrency_level; i++){
        endpoint_->Connect(rightNeighborName + "_" + std::to_string(i), options_.right_neighbor_address, leftNeighborName + "_" + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }


    // Wait for all connections to be established
    uint32 numConn = (options_.num_parties + 1) * options_.concurrency_level;
    while (endpoint_->GetRemoteNames().size() < numConn) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    endpoint_->StopListen();
}

// Perform distributed key generation
void Participant::DistributedKeyGeneration() {
    if (role() == Role::server) {
        DistributedKeyGenerationServer();
    } else if (role() == Role::client) {
        DistributedKeyGenerationClient();
    }
}

// Perform distributed key generation for the server participant
void Participant::DistributedKeyGenerationServer() {
    NTL::ZZ temp;
    std::vector<NTL::ZZ> zz_array;
    CollectZz(zz_array, 0);

    // Compute the product of all beta values
    for (const auto &z: zz_array) {
        NTL::MulMod(beta_, beta_, z, options_.p);
    }

    // Broadcast the final beta value to all participants
    BroadcastZz(beta_, 0);
}

// Perform distributed key generation for the client participant
void Participant::DistributedKeyGenerationClient() {
    // Send the local beta value to the server
    SendZz(serverName, beta_, 0);

    // Receive the final beta value from the server
    ReceiveZz(serverName, beta_, 0);
}

// Execute the protocol
std::vector<long long> Participant::Execute(bool print) {
    endpoint_->ResetCounters();
    bf_.Clear();

    std::vector<Ciphertext> encrypted_bases(
            options_.bloom_filter_size); // encrypted bases that will be passed along the ring for the purpose of voting
    std::vector<NTL::ZZ> decrypted_bases(options_.bloom_filter_size); // decryption outputs
    std::vector<std::pair<int, uint64>> result; // OTMPSI final result
    std::vector<Ciphertext> rerand_array(
            options_.bloom_filter_size); // probabilistic encryption of 1, used for ReRand Algorithm
    std::vector<NTL::ZZ> precomputed_table(options_.num_parties - options_.intersection_threshold + 1);

    auto start = std::chrono::high_resolution_clock::now();




  

    Prepare(encrypted_bases, rerand_array, precomputed_table);
    RingLatency(false);

    auto preparation_done = std::chrono::high_resolution_clock::now();



    RingPass(encrypted_bases, rerand_array);



    FindIntersection(result, encrypted_bases, rerand_array, precomputed_table);


    auto end = std::chrono::high_resolution_clock::now();

    auto preparation = std::chrono::duration_cast<std::chrono::milliseconds>(preparation_done - start).count();
    auto online = std::chrono::duration_cast<std::chrono::milliseconds>(end - preparation_done).count();
    std::vector<long long> durations = {preparation, online};
    if (role() == Role::server && print) {
        std::cout << "result size: " << result.size() << std::endl;
//        for(auto res : result){
//            std::cout << res.first << " " << res.second << std::endl;
//        }
    }

    return durations;
}

// Check if a number is a generator
bool is_generator(const NTL::ZZ &g, const NTL::ZZ &p, const std::vector<NTL::ZZ> &ppFactors) {
    NTL::ZZ temp;
    for (const auto &f: ppFactors) {
        PowerMod(temp, g, (p - 1) / f, p);
        if (temp == 1) {
            return false;
        }
    }
    return true;
}

// Prepare for the protocol
void Participant::Prepare(std::vector<Ciphertext> &encrypted_bases, std::vector<Ciphertext> &rerand_array,
                          std::vector<NTL::ZZ> &precomputed_table) {
    // Build the bloom filter
    for (const auto &e: elements_) {
        bf_.Insert(e);
    }

    // Invert the Bloom Filter
    bf_.Invert();

    // Finish the preparation
    if (role() == Role::server) {
        PrepareServer(encrypted_bases, rerand_array, precomputed_table);
    } else {
        PrepareClient(rerand_array);
    }
}

// Prepare for the protocol for the server participant
void Participant::PrepareServer(std::vector<Ciphertext> &encrypted_bases, std::vector<Ciphertext> &rerand_array,
                                std::vector<NTL::ZZ> &precomputed_table) {
    NTL::ZZ vote_base; // vote vote_base


    NTL::ZZ vote_base_power // vote vote_base power. vote vote_base = generator ^ ((p-1)/q^(t-l+1))
            = (options_.p - 1)
              / NTL::power(options_.q, (options_.num_parties - options_.intersection_threshold + 1));

    // first make vote_base a random generator for filed Fp
    NTL::RandomBnd(vote_base, options_.p);
    while (!is_generator(vote_base, options_.p, options_.phi_p_prime_factor_list)) {
        NTL::RandomBnd(vote_base, options_.p);
    }
    
    NTL::PowerMod(vote_base, vote_base, vote_base_power, options_.p);



    auto encrypt_range = [&](int start, int end) {
        NTL::ZZ temp;
        for (int i = start; i < end; ++i) {
            temp = vote_base;
            if (bf_.CheckPosition(i)) {
                NTL::PowerMod(temp, temp, options_.q, options_.p);
            }
            Encrypt(encrypted_bases[i], temp);
        }
    };

    std::vector<std::thread> threads;
    int total_elements = bf_.size();
    int elements_per_thread = total_elements / options_.concurrency_level;
    for (int i = 0; i < options_.concurrency_level; ++i) {
        int start = i * elements_per_thread;
        int end = (i == options_.concurrency_level - 1) ? total_elements : (start + elements_per_thread);
        threads.emplace_back(encrypt_range, start, end);
    }

    for (auto &th : threads) {
        th.join();
    }
    threads.clear();


    // Create an array of fresh encryptions of 1 to refresh the ciphertexts.
    // For each membership test result, precompute sqrRootTrail encryptions to refresh the ciphertext
    // in the hopes that the new ciphertext will have a square root.
    auto rerandomize_range = [&](int start, int end) {
        for (int i = start; i < end; ++i) {
            Encrypt(rerand_array[i], NTL::ZZ(1));
        }
    };

    int total_rerand = elements_.size();
    int rerand_per_thread = total_rerand / options_.concurrency_level;
    for (int i = 0; i < options_.concurrency_level; ++i) {
        int start = i * rerand_per_thread;
        int end = (i == options_.concurrency_level - 1) ? total_rerand : (start + rerand_per_thread);
        threads.emplace_back(rerandomize_range, start, end);
    }

    for (auto &th : threads) {
        th.join();
    }

    NTL::ZZ temp = vote_base;
    for (long i = options_.num_parties - options_.intersection_threshold; i >= 0; i--) {
        precomputed_table[i] = NTL::InvMod(temp, options_.p);
        NTL::PowerMod(temp, temp, options_.q, options_.p);
    }


}

// Prepare for the protocol for the client participant
void Participant::PrepareClient(std::vector<Ciphertext> &rerand_array) {
    // Create an array of fresh encryptions of 1 to refresh the ciphertexts.
    // Need to refresh all the ciphertexts passed on the ring.
    // For each membership test result, precompute 10 encryptions to refresh the ciphertext
    // in the hopes that the new ciphertext will have a square root.
     auto encrypt_range = [&](int start, int end) {
        for (int i = start; i < end; ++i) {
            Encrypt(rerand_array[i], NTL::ZZ(1));
        }
    };


    std::vector<std::thread> threads;
    int total_elements = bf_.size(); // Assume bf_ contains the number of elements to process
    int elements_per_thread = total_elements / options_.concurrency_level;

    for (int i = 0; i < options_.concurrency_level; ++i) {
        int start = i * elements_per_thread;
        int end = (i == options_.concurrency_level - 1) ? total_elements : (start + elements_per_thread);
        threads.emplace_back(encrypt_range, start, end);
    }

    for (auto &th : threads) {
        th.join();
    }



}

// Pass the bases on the ring
void Participant::RingPass(std::vector<Ciphertext> &encrypted_bases, const std::vector<Ciphertext> &rerand_array) {
    if (role() == Role::server) {
        RingPassServer(encrypted_bases);
    } else {
        RingPassClient(encrypted_bases, rerand_array);
    }
}

// Pass the bases on the ring for the server participant
void Participant::RingPassServer(std::vector<Ciphertext> &encrypted_bases) {
    auto range = [&](int start, int end, int thread) {
        for (int i = start; i < end; ++i) {
            SendCiphertext(rightNeighborName, encrypted_bases[i], thread);
        }

        for (int i = start; i < end; ++i) {
            ReceiveCiphertext(leftNeighborName, encrypted_bases[i], thread);
        }
    };

    std::vector<std::thread> threads;
    int total_elements = encrypted_bases.size();
    int elements_per_thread = total_elements / options_.concurrency_level;

    for (int i = 0; i < options_.concurrency_level; ++i) {
        int start = i * elements_per_thread;
        int end = (i == options_.concurrency_level - 1) ? total_elements : (start + elements_per_thread);
        threads.emplace_back(range, start, end, i);
    }

    for (auto &th : threads) {
        th.join();
    }
}

// Pass the bases on the ring for the client participant
void
Participant::RingPassClient(std::vector<Ciphertext> &encrypted_bases, const std::vector<Ciphertext> &rerand_array) {
    auto range = [&](int start, int end, int thread) {
        Ciphertext temp;
        for (auto i = start; i < end; i++) {
            // receive from left neighbor
            ReceiveCiphertext(leftNeighborName, temp, thread);

            // raise to Power of q if it is a 1 in node's rbf
            if (bf_.CheckPosition(i)) {
                Power(temp, temp, options_.q);
            }

            // ReRand c
            Mul(temp, temp, rerand_array[i]);

            // send to right neighbor
            if(options_.id != options_.num_parties-1){
                SendCiphertext(rightNeighborName, temp, thread);
            } else {
                encrypted_bases[i] = temp;
            }
        }

        if(options_.id == options_.num_parties-1){
            for (auto i = start; i < end; i++) {
                // if head, send ciphertexts to right neighbor to start
                SendCiphertext(rightNeighborName, encrypted_bases[i], thread);
            }
        }
    };

    std::vector<std::thread> threads;
    int total_elements = encrypted_bases.size();
    int elements_per_thread = total_elements / options_.concurrency_level;

    for (int i = 0; i < options_.concurrency_level; ++i) {
        int start = i * elements_per_thread;
        int end = (i == options_.concurrency_level - 1) ? total_elements : (start + elements_per_thread);
        threads.emplace_back(range, start, end, i);
    }

    for (auto &th : threads) {
        th.join();
    }
}


// Find the intersection of the sets
void Participant::FindIntersection(std::vector<std::pair<int, uint64>> &intersection,
                                   const std::vector<Ciphertext> &encrypted_bases,
                                   const std::vector<Ciphertext> &rerand_array,
                                   const std::vector<NTL::ZZ> &precomputed_table) {
    
    std::vector<Ciphertext> encrypted_membership_test_results(elements_.size());

    // Server does the membership tests
    if (role() == Role::server) {
        MembershipTestServer(encrypted_membership_test_results, encrypted_bases);
    }



    // Mutual decryption
    std::vector<NTL::ZZ> membership_test_results(elements_.size());
    auto range = [&](int start, int end, int thread) {
        for (auto i = start; i < end; i++) {
            if (role() == Role::server) {
                MutualDecryptServer(membership_test_results[i], encrypted_membership_test_results[i], thread);
            } else {
                MutualDecryptClient(thread);
            }
        }
    };

    std::vector<std::thread> threads;
    int total_elements = elements_.size();
    int elements_per_thread = total_elements / options_.concurrency_level;

    for (int i = 0; i < options_.concurrency_level; ++i) {
        int start = i * elements_per_thread;
        int end = (i == options_.concurrency_level - 1) ? total_elements : (start + elements_per_thread);
        threads.emplace_back(range, start, end, i);
    }

    for (auto &th : threads) {
        th.join();
    }

    // if (role() == Role::server) {
    //     for (int i = 0; i < elements_.size(); i++) {
    //         auto cnt = ExtractCountServer(membership_test_results[i], precomputed_table);
    //         if (cnt != 0) {
    //             intersection.emplace_back(cnt, elements_[i]);
    //         }
    //     }
    // }


   if (role() == Role::server) {
        std::vector<std::thread> threads;
        std::vector<std::vector<std::pair<int, uint64>>> local_intersections(options_.concurrency_level);
        int total_elements = elements_.size();
        int elements_per_thread = total_elements / options_.concurrency_level;

        auto range = [&](int start, int end, int thread_id) {
            for (int i = start; i < end; i++) {
                auto cnt = ExtractCountServer(membership_test_results[i], precomputed_table);
                if (cnt != 0) {
                    local_intersections[thread_id].emplace_back(cnt, elements_[i]);
                }
            }
        };

        for (int i = 0; i < options_.concurrency_level; ++i) {
            int start = i * elements_per_thread;
            int end = (i == options_.concurrency_level - 1) ? total_elements : (start + elements_per_thread);
            threads.emplace_back(range, start, end, i);
        }

        for (auto &th : threads) {
            th.join();
        }

        // Merge local intersections into the main intersection vector
        for (auto &local : local_intersections) {
            intersection.insert(intersection.end(), local.begin(), local.end());
        }

    }

}

// Perform membership tests for the server participant
void Participant::MembershipTestServer(std::vector<Ciphertext> &encrypted_membership_test_results,
                                       const std::vector<Ciphertext> &encrypted_bases) {
    auto range = [&](int start, int end) {
        Ciphertext test_result;
        for (auto i = start; i < end; i++) {
            auto positions = GetHashPositions(elements_[i], options_.bloom_filter_size, options_.murmurhash_seeds);
            test_result = encrypted_bases[positions[0]];
            for (int j = 1; j < positions.size(); j++) {
                Mul(test_result, test_result, encrypted_bases[positions[j]]);
            }
            encrypted_membership_test_results[i] = test_result;
        }
    };


    std::vector<std::thread> threads;
    int total_elements = elements_.size();
    int elements_per_thread = total_elements / options_.concurrency_level;

    for (int i = 0; i < options_.concurrency_level; ++i) {
        int start = i * elements_per_thread;
        int end = (i == options_.concurrency_level - 1) ? total_elements : (start + elements_per_thread);
        threads.emplace_back(range, start, end);
    }

    for (auto &th : threads) {
        th.join();
    }
}


// Perform mutual decryption for the server participant
void Participant::MutualDecryptServer(NTL::ZZ &result, const Ciphertext &c, int channel) {
    NTL::ZZ temp;
    // broadcast the first part of the ciphertext
    BroadcastZz(c.first, channel);


    // prepare server's own decryption share
    std::vector<NTL::ZZ> shares;
    shares.reserve(options_.party_list.size());
    PartialDecrypt(temp, c.first);
    shares.push_back(std::move(temp));

    // collect decryption shares from all other parties
    CollectZz(shares, channel);

    // fully decrypt using the decrpytion shares
    FullyDecrypt(result, shares, c.second);
}

// Perform mutual decryption for the client participant
void Participant::MutualDecryptClient(int channel) {
    NTL::ZZ d, temp;
    ReceiveZz(serverName, temp, channel);
    PartialDecrypt(d, temp);
    SendZz(serverName, d, channel);
}

// Extract the hidden count for server participant
uint32 Participant::ExtractCountServer(NTL::ZZ &membership_test_result, const std::vector<NTL::ZZ> &precomputed_table) {
    uint32 cnt;
    NTL::ZZ temp;
    for (auto i = 0; i < options_.num_hash_functions; i++) {
        cnt = 0;
        temp = membership_test_result;
        while (temp != 1) {  // keep raising to the power of q until it is a 1, and count the number of operations
            PowerMod(temp, temp, options_.q, options_.p);
            cnt++;
        }

        if (cnt == 0) {
            return 0;
        } else {
            NTL::MulMod(membership_test_result, membership_test_result, precomputed_table[cnt - 1], options_.p);
        }
    }

    return options_.intersection_threshold + cnt - 1;
}


// Get the ring latency
void Participant::RingLatency(bool print) {
    auto start = std::chrono::high_resolution_clock::now();
    if (role() == Role::server) {
        RingLatencyServer(start, print);
    } else {
        RingLatencyClient();
    }
}

// Get the ring latency, the server participant
void Participant::RingLatencyServer(std::chrono::high_resolution_clock::time_point start, bool print) {
    // dummy write and read
    uint8 dummy[2];
    endpoint_->Write(rightNeighborName + "_" + std::to_string(0), dummy, sizeof(dummy));
    endpoint_->Read(leftNeighborName + "_" + std::to_string(0), dummy, sizeof(dummy));

    auto end = std::chrono::high_resolution_clock::now();
    if (print) {
        std::cout << "Ring Latency: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                  << "ms" << std::endl;
    }
}

// Get the ring latency, the client participant
void Participant::RingLatencyClient() {
    // dummy read and write
    uint8 dummy[2];
    endpoint_->Read(leftNeighborName + "_" + std::to_string(0), dummy, sizeof(dummy));
    endpoint_->Write(rightNeighborName + "_" + std::to_string(0), dummy, sizeof(dummy));
}

// Broadcast an NTL::ZZ to all remote participants
void Participant::BroadcastZz(const NTL::ZZ &n, int channel) {
    for (const auto &remote: options_.party_list) {
        if (remote == options_.local_name) {
            continue;
        }
        SendZz(remote, n, channel);
    }
}

// Collect NTL::ZZs from all remote participants
void Participant::CollectZz(std::vector<NTL::ZZ> &zz_array, int channel) {
    NTL::ZZ temp;
    for (const auto &remote: options_.party_list) {
        if (remote == options_.local_name) {
            continue;
        }
        ReceiveZz(remote, temp, channel);

        zz_array.push_back(std::move(temp));
    }
}

// Broadcast a ciphertext to all remote participants
void Participant::BroadcastCiphertext(const Ciphertext &ciphertext, int channel) {
    for (const auto &remote: options_.party_list) {
        if (remote == options_.local_name) {
            continue;
        }
        SendCiphertext(remote, ciphertext, channel);
    }
}

// Collect ciphertexts from all remote participants
void Participant::CollectCiphertext(std::vector<Ciphertext> &ciphertext_array, int channel) {
    Ciphertext temp;
    for (const auto &remote: options_.party_list) {
        if (remote == options_.local_name) {
            continue;
        }
        ReceiveCiphertext(remote, temp, channel);
        ciphertext_array.push_back(std::move(temp));
    }
}

