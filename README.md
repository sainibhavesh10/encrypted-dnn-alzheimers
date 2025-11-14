# C++ Implementation of an Encrypted DNN for Alzheimer's Prediction

![C++](https://img.shields.io/badge/language-C%2B%2B-blue.svg)
![Python](https://img.shields.io/badge/language-Python-yellow.svg)
![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)

This repository contains the C++ implementation of a Deep Neural Network (DNN) designed to predict Alzheimer's disease on homomorphically encrypted data. This project (a minor project) demonstrates a privacy-preserving machine learning workflow where inference is performed *directly on encrypted data*, ensuring patient privacy as the server never sees the plaintext information.

The core of this project is a custom C++ inference engine that runs a pre-trained neural network using the **Microsoft SEAL** library for homomorphic encryption.

## The Problem

Standard machine learning models require access to plaintext data, which is a major privacy concern for sensitive medical records. This project solves that problem by implementing a system where:
1.  A patient can encrypt their medical data (30 features).
2.  The server can run a predictive DNN model directly on this encrypted data.
3.  The server returns an encrypted prediction, which only the patient can decrypt.

## Key Technical Features

* **Custom C++ Inference Engine:** The DNN (128-64-1 architecture) is implemented from scratch in C++ to be compatible with the Microsoft SEAL library.
* **Encrypted Activation Functions:** Includes custom C++ implementations of **ReLU** and **Sigmoid** activation functions that operate on RNS-CKKS encrypted data.
* **Python-to-C++ Workflow:** The model is trained in Python (using Keras/TensorFlow/PyTorch) on the plaintext dataset. The trained weights are then extracted and loaded by the C++ engine for encrypted inference.
* **Homomorphic Encryption:** Utilizes the **RNS-CKKS** scheme from Microsoft SEAL, which allows for arithmetic operations (addition and multiplication) on encrypted real numbers.

## Project Workflow

1.  **Python Model Training:**
    * A 3-layer DNN (`128-64-1`) is trained in Python on the plaintext dataset (containing 30 features).
    * The weights and biases from the trained model are saved/exported.
2.  **C++ Encrypted Inference:**
    * The client sets up encryption parameters (poly_modulus_degree, coefficient modulus) using Microsoft SEAL.
    * The client encrypts their 30-feature vector using the RNS-CKKS scheme.
    * The server loads the pre-trained weights into the custom C++ DNN.
    * The server executes the DNN inference, performing encrypted matrix multiplications and applying the custom encrypted ReLU/Sigmoid functions.
    * The server returns the final encrypted prediction (a single encrypted value).
3.  **Decryption:**
    * The client decrypts the result using their secret key to get the final prediction.

## Core Technologies

* **C++:** Used for the high-performance, secure inference engine.
* **Python:** Used for model training and data preprocessing.
    * *Libraries:* NumPy, Pandas, Scikit-learn, TensorFlow/Keras
* **[Microsoft SEAL](https://github.com/microsoft/SEAL):** The open-source homomorphic encryption library from Microsoft.

## Results & Performance

* **Model Accuracy:** **83%** (achieved on the encrypted test dataset, matching the plaintext model's accuracy).
* **Inference Time:** Approximately **3 minutes per sample**, demonstrating the computational trade-off required for privacy-preserving computation.

## How to Run

### Prerequisites

* A C++ compiler (g++, Clang, MSVC)
* [CMake](https://cmake.org/)
* [Microsoft SEAL](https://github.com/microsoft/SEAL) (Follow their guide to build and install the library)
* Python 3.x (for the training scripts)

### 1. Training (Python)

Navigate to the `/python_training` folder:

```bash
# Install dependencies
pip install -r requirements.txt

# Run the training script
python train_model.py
