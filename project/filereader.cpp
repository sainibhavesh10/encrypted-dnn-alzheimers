#include "filereader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm> // For std::shuffle, std::replace
#include <random>    // For std::mt19937
#include <chrono>    // For std::chrono::system_clock

// Definition for the 1D vector reader
std::vector<double> readVector(const std::string& filename) {
    std::vector<double> vec;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return vec; // Return empty vector
    }

    std::string line;
    double value;
    
    // Read one line at a time
    while (std::getline(file, line)) {
        if (line.empty()) continue; // Skip empty lines
        
        std::stringstream ss(line);
        if (ss >> value) {
            vec.push_back(value);
        }
    }

    file.close();
    std::cout << "Successfully read vector " << filename << " (size: " << vec.size() << ")" << std::endl;
    return vec;
}


// Definition for the 2D matrix reader
std::vector<std::vector<double>> readMatrix(const std::string& filename) {
    std::vector<std::vector<double>> matrix;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return matrix; // Return empty matrix
    }

    std::string line;
    
    // Read one line at a time
    while (std::getline(file, line)) {
        if (line.empty()) continue; // Skip empty lines

        std::vector<double> row;
        std::stringstream ss(line);
        double value;

        // Parse all space-separated numbers on this line
        while (ss >> value) {
            row.push_back(value);
        }

        if (!row.empty()) {
            matrix.push_back(row);
        }
    }

    file.close();
    std::cout << "Successfully read matrix " << filename << " (rows: " << matrix.size() << ")" << std::endl;
    return matrix;
}

// Definition for the client data reader (reads all rows)
// THIS FUNCTION IS NOW MODIFIED TO HANDLE CSV (comma-separated)
std::vector<std::vector<double>> readClientData(const std::string& filename) {
    std::vector<std::vector<double>> matrix;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return matrix;
    }

    std::string line;
    
    // Read one line at a time
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        // --- THIS IS THE FIX ---
        // Replace all commas with spaces
        std::replace(line.begin(), line.end(), ',', ' ');
        // --- END FIX ---

        std::vector<double> row;
        std::stringstream ss(line);
        double value;

        // Parse all space-separated numbers on this line
        while (ss >> value) {
            row.push_back(value);
        }

        if (!row.empty()) {
            matrix.push_back(row);
        }
    }

    file.close();
    // No "Success" message here, readRandomClientData will print
    return matrix;
}

// Definition for the random client data reader (reads 'count' random rows)
// This function USES the corrected readClientData, so it doesn't need changes.
std::vector<std::vector<double>> readRandomClientData(const std::string& filename, size_t count) {
    std::cout << "Reading all client data to sample from " << filename << "..." << std::endl;
    
    // 1. Read all rows from the file (this now handles CSV)
    std::vector<std::vector<double>> all_data = readClientData(filename);

    if (all_data.empty()) {
        std::cerr << "Error: No data was read from " << filename << std::endl;
        return all_data;
    }

    // 2. Get a random number generator
    // We use the current time as a seed for true randomness
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 g(seed);

    // 3. Shuffle the entire vector
    std::shuffle(all_data.begin(), all_data.end(), g);

    // 4. Truncate the vector if it's larger than the requested count
    if (all_data.size() > count) {
        all_data.resize(count);
    }

    std::cout << "Successfully sampled " << all_data.size() << " random rows." << std::endl;
    return all_data;
}