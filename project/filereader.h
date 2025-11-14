#pragma once

#include <string>
#include <vector>

// Reads a 1D vector (for bias files)
// Each line is assumed to be a single number
std::vector<double> readVector(const std::string& filename);

// Reads a 2D matrix (for weight files)
// Each line is a row, with numbers separated by spaces
std::vector<std::vector<double>> readMatrix(const std::string& filename);

// Definition for the client data reader (reads all rows)
// THIS FUNCTION IS NOW MODIFIED TO HANDLE CSV (comma-separated)
std::vector<std::vector<double>> readClientData(const std::string& filename);

// Definition for the random client data reader (reads 'count' random rows)
// This function USES the corrected readClientData, so it doesn't need changes.
std::vector<std::vector<double>> readRandomClientData(const std::string& filename, size_t count);