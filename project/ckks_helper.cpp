#include "ckks_helper.h"
#include <stdexcept>

// --- Constructor ---
CKKSHelper::CKKSHelper(
    seal::SEALContext &context,
    seal::Encryptor &encryptor,
    seal::Evaluator &evaluator,
    seal::CKKSEncoder &encoder,
    seal::RelinKeys &relin_keys,
    seal::GaloisKeys &galois_keys,
    double scale)
    : context(context),
      encryptor(encryptor),
      evaluator(evaluator),
      encoder(encoder),
      relin_keys(relin_keys),
      galois_keys(galois_keys),
      scale(scale) {
    this->slot_count = encoder.slot_count();
}

// --- Function Implementations ---

void CKKSHelper::add_plain_inplace(seal::Ciphertext &ct, const std::vector<double> &vec) {
    seal::Plaintext pt;
    // Encode at the ciphertext's current level and scale
    encoder.encode(vec, ct.parms_id(), ct.scale(), pt);
    evaluator.add_plain_inplace(ct, pt);
}

void CKKSHelper::multiply_plain_inplace(seal::Ciphertext &ct, const std::vector<double> &vec) {
    seal::Plaintext pt;
    // Encode at the ciphertext's current level and scale
    encoder.encode(vec, ct.parms_id(), ct.scale(), pt);
    
    evaluator.multiply_plain_inplace(ct, pt);
    evaluator.rescale_to_next_inplace(ct);
}

void CKKSHelper::sum_all_slots_inplace(seal::Ciphertext &ct) {
    // We only need to rotate up to slot_count / 2
    for (size_t i = 1; i <= slot_count / 2; i *= 2) {
        seal::Ciphertext rotated_ct = ct;
        evaluator.rotate_vector_inplace(rotated_ct, i, galois_keys);
        evaluator.add_inplace(ct, rotated_ct);
    }
}

void CKKSHelper::mask_slot_inplace(seal::Ciphertext &ct, size_t index) {
    // Check if the index is valid
    if (index >= slot_count) {
        throw std::out_of_range("Mask index is out of range for this slot count.");
    }
    
    // Create a vector of all zeros
    std::vector<double> mask(slot_count, 0.0);
    
    // Set the one slot we want to keep to 1.0
    mask[index] = 1.0;
    
    // This is just a multiply_plain operation
    multiply_plain_inplace(ct, mask);
}

void CKKSHelper::apply_poly_relu_inplace(seal::Ciphertext &ct_in,size_t size) {
    // Implements: 0.25x² + 0.5x
    //optimization: 0.5x * (0.5x + 1)
    // (2 depth cost)
    
    // 1. Calculate ct_lin = 0.5x (at level N-1)
    seal::Ciphertext ct_lin = ct_in;
    // multiply_plain_inplace costs 1 level, ct_lin is now at N-1
    multiply_plain_inplace(ct_lin, std::vector<double> (size,0.5)); 

    // 2. Calculate ct_lin_plus_1 = (0.5x + 1) (at level N-1)
    seal::Ciphertext ct_lin_plus_1 = ct_lin;
    // add_plain is "free" (0 depth cost), ct_lin_plus_1 is still at N-1
    add_plain_inplace(ct_lin_plus_1, std::vector<double> (size,1.0));

    // 3. Multiply them: 0.5x * (0.5x + 1)
    // This is a Ctxt-Ctxt multiplication, costs 1 level
    evaluator.multiply_inplace(ct_lin, ct_lin_plus_1); // Result at N-1
    evaluator.relinearize_inplace(ct_lin, relin_keys);
    evaluator.rescale_to_next_inplace(ct_lin); // Final result at N-2

    // 4. Assign back
    ct_in = ct_lin;
}

void CKKSHelper::apply_poly_sigmoid_inplace(seal::Ciphertext &ct_in, size_t size) {
    // Approximation: sigmoid(x) ≈ 0.5 + 0.197x - 0.004x³
    // Depth cost: 2 (x² and x³)

    // 1. Compute x²
    seal::Ciphertext ct_x2;
    evaluator.multiply(ct_in, ct_in, ct_x2);
    evaluator.relinearize_inplace(ct_x2, relin_keys);
    evaluator.rescale_to_next_inplace(ct_x2);

    // Align ct_in scale and parms_id with ct_x2 before multiplying
    evaluator.mod_switch_to_inplace(ct_in, ct_x2.parms_id());
    ct_in.scale() = ct_x2.scale();

    // 2. Compute x³ = x² * x
    seal::Ciphertext ct_x3;
    evaluator.multiply(ct_x2, ct_in, ct_x3);
    evaluator.relinearize_inplace(ct_x3, relin_keys);
    evaluator.rescale_to_next_inplace(ct_x3);

    // Align everything to ct_x3's level
    evaluator.mod_switch_to_inplace(ct_in, ct_x3.parms_id());
    evaluator.mod_switch_to_inplace(ct_x2, ct_x3.parms_id());
    ct_in.scale() = ct_x3.scale();
    ct_x2.scale() = ct_x3.scale();

    // 3. Combine: 0.5 + 0.197x - 0.004x³
    seal::Ciphertext result = ct_x3;
    multiply_plain_inplace(result, std::vector<double>(size, -0.004));

    // Add 0.197x
    seal::Ciphertext ct_lin = ct_in;
    multiply_plain_inplace(ct_lin, std::vector<double>(size, 0.197));

    // Make sure both are on same level
    evaluator.mod_switch_to_inplace(result, ct_lin.parms_id());

    evaluator.add_inplace(result, ct_lin);

    // Add 0.5 (plain)
    add_plain_inplace(result, std::vector<double>(size, 0.5));

    // Return result in ct_in
    ct_in = result;
}

seal::Ciphertext CKKSHelper::process_layer_linear(seal::Ciphertext &ct_in, std::vector<std::vector<double>> &weights, std::vector<double> &biases){
    // Initializing ct_final with 0
    std::vector<double> zero_vector(slot_count, 0.0);
    seal::Plaintext pt_zero;
    encoder.encode(zero_vector, scale, pt_zero);
    
    seal::Ciphertext ct_final;
    encryptor.encrypt(pt_zero, ct_final);

    //making final layer values without adding biases one feature by one
    for(int i = 0 ; i < weights.size() ; i++){
        seal::Ciphertext ct_curr = ct_in;
        multiply_plain_inplace(ct_curr,weights[i]); //1 depth cost
        sum_all_slots_inplace(ct_curr);
        mask_slot_inplace(ct_curr,i);
        if(ct_final.parms_id()!=ct_curr.parms_id()){
            evaluator.mod_switch_to_inplace(ct_final, ct_curr.parms_id());
            ct_final.scale() = ct_curr.scale();
        }
        evaluator.add_inplace(ct_final, ct_curr);
    }

    //finally adding features
    add_plain_inplace(ct_final,biases);
    return ct_final;
}



