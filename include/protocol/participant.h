#ifndef OTMPSI_PARTICIPANT_H
#define OTMPSI_PARTICIPANT_H

#include <chrono>
#include <vector>

#include "crypto/threshold_elgamal.h"
#include "network/tcp_endpoint.h"
#include "utils/bloom_filter.h"
#include "utils/common.h"

class Participant : KeyHolder {
public:
    // Constructor
    Participant(const Options &options, const std::vector<ElementType> &set)
            : KeyHolder(options.p, options.alpha, options.phi_p_prime_factor_list),
              endpoint_(new TcpEndpoint(options.port)),
              elements_(set),
              bf_(options.bloom_filter_size, options.murmurhash_seeds),
              options_(options) {
        endpoint_->Start();
    };

    // Deleted default constructor
    Participant() = delete;

    // Default destructor
    ~Participant() = default;

    // Get the role of the participant
    [[nodiscard]] Role role() const { return options_.role; };

    // Change the element set of the participant
    void ChangeElementSet(const std::vector<ElementType> &new_set) { elements_ = new_set; };

    // Initialize the participant
    void Initialize();

    // Get the ring latency
    void RingLatency(bool print);

    // Stop and shut down the participant
    inline void Stop();

    // Execute the protocol
    std::vector<long long> Execute(bool print);

    // Method to get the total amount of data sent in a more readable form
    inline uint64 GetTotalBytesSent() const;

    // Method to get the total amount of data received in a more readable form
    inline uint64 GetTotalBytesReceived() const;

private:
    // Network module
    Endpoint *endpoint_;

    // Element set of the participant
    std::vector<ElementType> elements_;

    // Bloom filter of the participant
    BloomFilter bf_;

    // Options for the protocol
    Options options_;

    // Perform distributed key generation
    void DistributedKeyGeneration();

    // Prepare for the protocol
    void Prepare(std::vector<Ciphertext> &encrypted_bases,
                 std::vector<Ciphertext> &rerand_array, std::vector<NTL::ZZ> &precomputed_table);

    // Pass the bases on the ring, each participant applies its operation
    void RingPass(std::vector<Ciphertext> &encrypted_bases,
                  const std::vector<Ciphertext> &rerand_array);


    // Find the intersection of the sets
    void FindIntersection(std::vector<std::pair<int, uint64>> &intersection,
                          const std::vector<Ciphertext> &encrypted_bases,
                          const std::vector<Ciphertext> &rerand_array, const std::vector<NTL::ZZ> &precomputed_table);

    // Send an NTL::ZZ to a remote participant
    inline void SendZz(const std::string &remote, const NTL::ZZ &n, int channel);

    // Receive an NTL::ZZ from a remote participant
    inline void ReceiveZz(const std::string &remote, NTL::ZZ &n, int channel);

    // Broadcast an NTL::ZZ to all remote participants
    void BroadcastZz(const NTL::ZZ &n, int channel);

    // Collect NTL::ZZs from all remote participants
    void CollectZz(std::vector<NTL::ZZ> &zz_array, int channel);

    // Send a ciphertext to a remote participant
    inline void SendCiphertext(const std::string &remote, const Ciphertext &ciphertext, int channel);

    // Receive a ciphertext from a remote participant
    inline void ReceiveCiphertext(const std::string &remote, Ciphertext &ciphertext, int channel);

    // Broadcast a ciphertext to all remote participants
    void BroadcastCiphertext(const Ciphertext &ciphertext, int channel);

    // Collect ciphertexts from all remote participants
    void CollectCiphertext(std::vector<Ciphertext> &ciphertext_array, int channel);

    // Initialize the client participant
    void InitializeClient();

    // Initialize the server participant
    void InitializeServer();

    // Perform distributed key generation for the server participant
    void DistributedKeyGenerationServer();

    // Perform distributed key generation for the client participant
    void DistributedKeyGenerationClient();

    // Prepare for the protocol for the server participant
    void PrepareServer(std::vector<Ciphertext> &encrypted_bases, std::vector<Ciphertext> &rerand_array,
                       std::vector<NTL::ZZ> &precomputed_table);

    // Prepare for the protocol for the client participant
    void PrepareClient(std::vector<Ciphertext> &rerand_array);

    // Pass the bases on the ring for the server participant
    void RingPassServer(std::vector<Ciphertext> &encrypted_bases);

    // Pass the bases on the ring for the client participant
    void RingPassClient(std::vector<Ciphertext> &encrypted_bases, const std::vector<Ciphertext> &rerand_array);

    // Membership test for server participant
    void MembershipTestServer(std::vector<Ciphertext> &encrypted_membership_test_results,
                              const std::vector<Ciphertext> &encrypted_bases);

    // Perform mutual decryption for the server participant
    void MutualDecryptServer(NTL::ZZ &result, const Ciphertext &c, int channel);

    // Perform mutual decryption for the client participant
    void MutualDecryptClient(int channel);

    // Extract the hidden count for server participant
    uint32 ExtractCountServer(NTL::ZZ &membership_test_result, const std::vector<NTL::ZZ> &precomputed_table);

    // Get the ring latency for the server participant
    void RingLatencyServer(std::chrono::high_resolution_clock::time_point start, bool print);

    // Get the ring latency for the client participant
    void RingLatencyClient();
};


void Participant::SendZz(const std::string &remote, const NTL::ZZ &n, int channel) {
    // unsigned char buf[options_.num_bytes_field_numbers];
    // BytesFromZZ(buf, n, options_.num_bytes_field_numbers);
    // endpoint_->AsyncWrite(remote, buf, options_.num_bytes_field_numbers);

    unsigned char buf[options_.num_bytes_field_numbers];
    BytesFromZZ(buf, n, options_.num_bytes_field_numbers);
    endpoint_->Write(remote + "_" + std::to_string(channel), buf, options_.num_bytes_field_numbers);
}

void Participant::ReceiveZz(const std::string &remote, NTL::ZZ &n, int channel) {
    unsigned char buf[options_.num_bytes_field_numbers];
    endpoint_->Read(remote + "_" + std::to_string(channel), buf, options_.num_bytes_field_numbers);
    ZZFromBytes(n, buf, options_.num_bytes_field_numbers);
}

void Participant::SendCiphertext(const std::string &remote, const Ciphertext &ciphertext, int channel) {
    SendZz(remote, ciphertext.first, channel);
    SendZz(remote, ciphertext.second, channel);
}

void Participant::ReceiveCiphertext(const std::string &remote, Ciphertext &ciphertext, int channel) {
    ReceiveZz(remote, ciphertext.first, channel);
    ReceiveZz(remote, ciphertext.second, channel);
}

void Participant::Stop() { endpoint_->Stop(); }

// Method to get the total amount of data sent in a more readable form
uint64 Participant::GetTotalBytesSent() const {
    return endpoint_->GetTotalBytesSent();
}

// Method to get the total amount of data received in a more readable form
uint64 Participant::GetTotalBytesReceived() const {
    return endpoint_->GetTotalBytesReceived();
}

#endif  // OTMPSI_PARTICIPANT_H