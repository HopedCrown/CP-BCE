#pragma once
#include <string>
#include <vector>

namespace ChemEngine {
    // Expose only the pure data interface to your test and UI suites
    bool balance(const std::vector<std::string>& reactants, 
                 const std::vector<std::string>& products, 
                 std::vector<int>& out_coefficients);
}
// This is a very heavily dependent structure. however, it might not work.
// Test via root/tests/EngineImplTest.cpp!
