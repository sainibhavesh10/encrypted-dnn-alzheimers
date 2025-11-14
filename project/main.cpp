#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <stdexcept>

#include "seal/seal.h"
#include "filereader.h"
#include "ckks_helper.h"

using namespace seal;
using namespace std;


std::vector<double> min_vals = {
    60.0000000000, 0.0000000000, 15.0088511816, 0.0000000000, 0.0020030991, 0.0036160168, 0.0093847201, 4.0026286598, 0.0000000000, 0.0000000000, 0.0000000000, 0.0000000000,
    0.0000000000, 0.0000000000, 90.0000000000, 60.0000000000, 150.0933155941, 50.2307065598, 20.0034340150, 50.4071936198, 0.0053121464, 0.0004595936, 0.0000000000, 0.0000000000,
    0.0012879277, 0.0000000000, 0.0000000000, 0.0000000000, 0.0000000000, 0.0000000000
};

std::vector<double> max_vals = {
    90.0000000000, 3.0000000000, 39.9927674640, 1.0000000000, 19.9892933591, 9.9874294134, 9.9983456788, 9.9998403167, 1.0000000000, 1.0000000000, 1.0000000000, 1.0000000000,
    1.0000000000, 1.0000000000, 179.0000000000, 119.0000000000, 299.9933524743, 199.9656651014, 99.9803240780, 399.9418615941, 29.9913805605, 9.9964670726, 1.0000000000, 1.0000000000,
    9.9997471218, 1.0000000000, 1.0000000000, 1.0000000000, 1.0000000000, 1.0000000000
};

void print_decrypted_vector(
    seal::Ciphertext &ct,
    seal::Decryptor &decryptor,
    seal::CKKSEncoder &encoder,
    size_t limit = 5)
{
    seal::Plaintext pt;
    std::vector<double> result;

    decryptor.decrypt(ct, pt);
    encoder.decode(pt, result);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "[ ";

    for (size_t i = 0; i < std::min(limit, result.size()); ++i)
    {
        std::cout << result[i];
        if (i != std::min(limit, result.size()) - 1)
            std::cout << ", ";
    }

    if (result.size() > limit)
        std::cout << ", ...";

    std::cout << " ]" << std::endl;
}

std::vector<double> minmax_scale(const std::vector<double>& x) {
    if (x.size() != min_vals.size() || x.size() != max_vals.size()) {
        throw std::invalid_argument("Input and scaler vectors must have same size");
    }

    std::vector<double> x_scaled;
    x_scaled.reserve(x.size());

    for (size_t i = 0; i < x.size(); ++i) {
        double denom = max_vals[i] - min_vals[i];
        if (denom == 0.0) {
            x_scaled.push_back(-1.0);
        } else {
            x_scaled.push_back(
                -1.0 + (x[i] - min_vals[i]) * (1.0 - (-1.0)) / denom
            );
        }
    }
    return x_scaled;
}

bool checkStatus(
    vector<double> &tempData,
    vector<double> &l0_biases, vector<double> &l1_biases, vector<double> &l2_biases,
    vector<vector<double>>& l0_weights, vector<vector<double>>& l1_weights, vector<vector<double>>& l2_weights,
    CKKSEncoder &encoder, Encryptor &encryptor, Decryptor &decryptor, double &scale,
    CKKSHelper &helper)
{
    int diagnosis_dataset = tempData[30], diagnosis_model;
    vector<double> data = minmax_scale(vector<double>(tempData.begin(), tempData.begin() + 30));

    seal::Ciphertext ct_data;
    seal::Plaintext pt_data;

    // 2. Encode and encrypt data
    encoder.encode(data, scale, pt_data);
    encryptor.encrypt(pt_data, ct_data);

    seal::Ciphertext ct_data1 = helper.process_layer_linear(ct_data, l0_weights, l0_biases);
    helper.apply_poly_relu_inplace(ct_data1,128);

    seal::Ciphertext ct_data2 = helper.process_layer_linear(ct_data1,l1_weights,l1_biases);
    helper.apply_poly_relu_inplace(ct_data2,64);

    seal::Ciphertext ct_data3 = helper.process_layer_linear(ct_data2, l2_weights, l2_biases);
    helper.apply_poly_sigmoid_inplace(ct_data3,1);

    seal::Plaintext pt;
    std::vector<double> result;

    decryptor.decrypt(ct_data3, pt);
    encoder.decode(pt, result);

    if(result[0]>0.5) diagnosis_model = 1;
    else diagnosis_model = 0;

    if(diagnosis_dataset == diagnosis_model) return true;
    return false;
}

int main()
{
    // =========================================================================
    // 1. SET UP PARAMETERS
    // =========================================================================
    EncryptionParameters parms(scheme_type::ckks);
    size_t poly_modulus_degree = 32768;
    double scale = pow(2.0, 40);

    parms.set_poly_modulus_degree(poly_modulus_degree);
    vector<int> bit_sizes = { 
        60, 40, 40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40, 40, 60  
    };
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, bit_sizes));

    cout << "--- Initializing SEAL Context ---" << endl;
    SEALContext context(parms);
    if (!context.parameters_set()) {
        cout << "Error: Encryption parameters invalid: "
             << context.parameter_error_message() << endl;
        return 1;
    }

    // =========================================================================
    // 2. KEYS & TOOLS
    // =========================================================================
    KeyGenerator keygen(context);
    PublicKey public_key; keygen.create_public_key(public_key);
    SecretKey secret_key = keygen.secret_key();
    RelinKeys relin_keys; keygen.create_relin_keys(relin_keys);
    GaloisKeys galois_keys; keygen.create_galois_keys(galois_keys);

    Encryptor encryptor(context, public_key);
    Evaluator evaluator(context);
    Decryptor decryptor(context, secret_key);
    CKKSEncoder encoder(context);

    cout << "Keys and tools ready. Using scale 2^" << log2(scale) << endl;

    CKKSHelper helper(context, encryptor, evaluator, encoder, relin_keys, galois_keys, scale);

    // =========================================================================
    // 3. LOAD WEIGHTS AND BIASES AND DATA
    // =========================================================================
    vector<double> l0_biases, l1_biases, l2_biases;
    vector<vector<double>> l0_weights, l1_weights, l2_weights, test_data;

    try
    {
        test_data = readRandomClientData("../../inputs/data.csv",100);

        // Load Layer 0
        l0_biases = readVector("../../inputs/layer_0_biases.txt");
        l0_weights = readMatrix("../../inputs/layer_0_weights.txt");
        // Load Layer 1
        l1_biases = readVector("../../inputs/layer_1_biases.txt");
        l1_weights = readMatrix("../../inputs/layer_1_weights.txt");
        // Load Layer 2
        l2_biases = readVector("../../inputs/layer_2_biases.txt");
        l2_weights = readMatrix("../../inputs/layer_2_weights.txt");
    }
    catch (const std::exception &e)
    {
        cout << "FATAL FILE ERROR: " << e.what() << endl;
        cout << "Check file paths: ../../inputs/..." << endl;
        return 1;
    }

    int accuracy = 0, total_tests = 0;

    for(auto data : test_data){
        bool result = checkStatus(data, 
                                l0_biases, l1_biases, l2_biases,
                                l0_weights, l1_weights, l2_weights,
                                encoder, encryptor, decryptor, scale,
                                helper);
        if(result) accuracy++;
        total_tests++;
    }

    cout<<accuracy<<" "<<total_tests<<endl;

    return 0;
}
