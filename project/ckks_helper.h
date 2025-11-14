#pragma once

#include "seal/seal.h"
#include <vector>
#include <stdexcept>

/**
 * @brief A helper class to manage common CKKS operations for a neural network.
 * BASED ON CORRECTED CKKS LOGIC.
 */
class CKKSHelper {
private:
    seal::SEALContext &context;
    seal::Encryptor &encryptor;
    seal::Evaluator &evaluator;
    seal::CKKSEncoder &encoder;
    seal::RelinKeys &relin_keys;
    seal::GaloisKeys &galois_keys;
    double scale;
    size_t slot_count;

public:
    CKKSHelper(
        seal::SEALContext &context,
        seal::Encryptor &encryptor,
        seal::Evaluator &evaluator,
        seal::CKKSEncoder &encoder,
        seal::RelinKeys &relin_keys,
        seal::GaloisKeys &galois_keys,
        double scale);

    /**
     * @brief Adds a plaintext vector to a ciphertext. (0 depth cost)
     */
    void add_plain_inplace(seal::Ciphertext &ct, const std::vector<double> &vec);

    /**
     * @brief Multiplies a ciphertext by a plaintext vector. (1 depth cost)
     * This operation consume a level and change the scale.
     */
    void multiply_plain_inplace(seal::Ciphertext &ct, const std::vector<double> &vec);

    /**
     * @brief Sums all slots of a ciphertext into slot 0 (and duplicates). (0 depth cost)
     */
    void sum_all_slots_inplace(seal::Ciphertext &ct);

    /**
     * @brief Masks the ciphertext by [0,..,1,..0] at a specific index. (0 depth cost)
     * @param ct The ciphertext to modify.
     * @param index The 0-based index of the slot to keep (e.g., 0 for the first slot).
     */
    void mask_slot_inplace(seal::Ciphertext &ct, size_t index);

    /**
     * @brief Applies the polynomial ReLU: 0.25x² + 0.5x. (1 depth cost)
     */
    void apply_poly_relu_inplace(seal::Ciphertext &ct_in, size_t size);
    
    /**
     * @brief Applies the polynomial Sigmoid: 0.5 + 0.25x - x³/48. (2 depth cost)
     */
    void apply_poly_sigmoid_inplace(seal::Ciphertext &ct_in,size_t size);

    seal::Ciphertext process_layer_linear(seal::Ciphertext &ct_in, std::vector<std::vector<double>> &weights, std::vector<double> &biases);
};

