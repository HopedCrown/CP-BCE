#include <string>
#include <vector>
#include <map>
#include <cmath>

namespace ChemEngine {

// --- 1. Pure Integer Math Core ---
inline long long engine_gcd(long long a, long long b) {
    a = (a < 0) ? -a : a; b = (b < 0) ? -b : b;
    while (b) {
        a %= b;
        long long tmp = a; a = b; b = tmp;
    }
    return a;
}

inline long long engine_lcm(long long a, long long b) {
    if (a == 0 || b == 0) return 0;
    return ((a < 0 ? -a : a) * (b < 0 ? -b : b)) / engine_gcd(a, b);
}

struct EngineFraction {
    long long num = 0, den = 1;
    EngineFraction() {}
    EngineFraction(long long n, long long d = 1) : num(n), den(d) { reduce(); }

    void reduce() {
        if (den == 0) return;
        long long g = engine_gcd(num, den);
        num /= g; den /= g;
        if (den < 0) { num = -num; den = -den; }
    }

    EngineFraction operator+(const EngineFraction& o) const { return EngineFraction(num * o.den + o.num * den, den * o.den); }
    EngineFraction operator-(const EngineFraction& o) const { return EngineFraction(num * o.den - o.num * den, den * o.den); }
    EngineFraction operator*(const EngineFraction& o) const { return EngineFraction(num * o.num, den * o.den); }
    EngineFraction operator/(const EngineFraction& o) const { return EngineFraction(num * o.den, den * o.num); }
    bool isZero() const { return num == 0; }
};

// --- 2. Isolated Molecule Parser ---
// Handles elements, counts, and brackets safely (e.g., "Al2(SO4)3")
std::map<std::string, int> parse_molecule_string(const std::string& formula) {
    std::map<std::string, int> element_counts;
    std::vector<std::map<std::string, int>> stack;
    stack.push_back(std::map<std::string, int>()); // Base layer

    size_t i = 0;
    size_t n = formula.length();

    while (i < n) {
        if (formula[i] == '(' || formula[i] == '[' || formula[i] == '{') {
            stack.push_back(std::map<std::string, int>());
            i++;
        } 
        else if (formula[i] == ')' || formula[i] == ']' || formula[i] == '}') {
            i++;
            int multiplier = 0;
            while (i < n && formula[i] >= '0' && formula[i] <= '9') {
                multiplier = multiplier * 10 + (formula[i] - '0');
                i++;
            }
            if (multiplier == 0) multiplier = 1;

            if (stack.size() > 1) {
                auto completed_layer = stack.back();
                stack.pop_back();
                for (const auto& [element, count] : completed_layer) {
                    stack.back()[element] += count * multiplier;
                }
            }
        } 
        else if (formula[i] >= 'A' && formula[i] <= 'Z') {
            std::string element = "";
            element += formula[i++];
            while (i < n && formula[i] >= 'a' && formula[i] <= 'z') {
                element += formula[i++];
            }
            int count = 0;
            while (i < n && formula[i] >= '0' && formula[i] <= '9') {
                count = count * 10 + (formula[i] - '0');
                i++;
            }
            if (count == 0) count = 1;
            stack.back()[element] += count;
        } 
        else {
            i++; // Skip unparseable formatting characters cleanly
        }
    }

    // Collapse remaining stack fragments if unclosed brackets existed
    while (stack.size() > 1) {
        auto top = stack.back();
        stack.pop_back();
        for (const auto& [element, count] : top) {
            stack.back()[element] += count;
        }
    }

    return stack.back();
}

// --- 3. Pure Algorithmic Matrix Solver Backend ---
// Takes direct vector string parameters. Populates target array. Returns status.
bool balance(const std::vector<std::string>& reactants, 
             const std::vector<std::string>& products, 
             std::vector<int>& out_coefficients) 
{
    out_coefficients.clear();
    size_t total_species = reactants.size() + products.size();
    if (total_species == 0) return false;

    std::vector<std::map<std::string, int>> parsed_compounds;
    std::vector<std::string> unique_elements;

    // Parse all structures sequentially 
    for (const auto& formula : reactants) {
        parsed_compounds.push_back(parse_molecule_string(formula));
        for (const auto& [element, count] : parsed_compounds.back()) {
            if (std::find(unique_elements.begin(), unique_elements.end(), element) == unique_elements.end()) {
                unique_elements.push_back(element);
            }
        }
    }
    for (const auto& formula : products) {
        parsed_compounds.push_back(parse_molecule_string(formula));
        for (const auto& [element, count] : parsed_compounds.back()) {
            if (std::find(unique_elements.begin(), unique_elements.end(), element) == unique_elements.end()) {
                unique_elements.push_back(element);
            }
        }
    }

    size_t num_elements = unique_elements.size();
    if (num_elements == 0) return false;

    // Allocate matrix mapping via hijacked allocator context
    std::vector<std::vector<EngineFraction>> matrix(num_elements, std::vector<EngineFraction>(total_species, EngineFraction(0, 1)));

    // Populate rows (Elements) across columns (Compounds)
    for (size_t r = 0; r < num_elements; ++r) {
        const std::string& target_element = unique_elements[r];
        
        // Reactants (Positive side)
        for (size_t c = 0; c < reactants.size(); ++c) {
            if (parsed_compounds[c].count(target_element)) {
                matrix[r][c] = EngineFraction(parsed_compounds[c][target_element], 1);
            }
        }
        // Products (Negative side)
        for (size_t p = 0; p < products.size(); ++p) {
            size_t c = reactants.size() + p;
            if (parsed_compounds[c].count(target_element)) {
                matrix[r][c] = EngineFraction(-parsed_compounds[c][target_element], 1);
            }
        }
    }

    // Run exact Gauss-Jordan Elimination RREF
    size_t lead = 0;
    for (size_t r = 0; r < num_elements; ++r) {
        if (lead >= total_species) break;
        size_t i = r;
        while (matrix[i][lead].isZero()) {
            i++;
            if (i == num_elements) {
                i = r;
                lead++;
                if (lead == total_species) break;
            }
        }
        if (lead == total_species) break;

        std::swap(matrix[i], matrix[r]);
        EngineFraction lv = matrix[r][lead];
        
        for (size_t j = 0; j < total_species; ++j) matrix[r][j] = matrix[r][j] / lv;
        
        for (size_t i_sub = 0; i_sub < num_elements; ++i_sub) {
            if (i_sub != r) {
                EngineFraction lv2 = matrix[i_sub][lead];
                for (size_t j = 0; j < total_species; ++j) {
                    matrix[i_sub][j] = matrix[i_sub][j] - (matrix[r][j] * lv2);
                }
            }
        }
        lead++;
    }

    // Extract Nullspace vectors
    std::vector<EngineFraction> coeff_fracs(total_species);
    coeff_fracs[total_species - 1] = EngineFraction(1, 1);
    for (size_t r = 0; r < total_species - 1; ++r) {
        coeff_fracs[r] = EngineFraction(0, 1) - matrix[r][total_species - 1];
    }

    // Find Lowest Common Multiple across fractional outcomes
    long long lcm_den = 1;
    for (size_t i = 0; i < total_species; ++i) {
        lcm_den = engine_lcm(lcm_den, coeff_fracs[i].den);
    }

    // Scale final integers into execution array
    out_coefficients.resize(total_species);
    for (size_t i = 0; i < total_species; ++i) {
        long long final_val = coeff_fracs[i].num * (lcm_den / coeff_fracs[i].den);
        if (final_val <= 0 || final_val > 10000) return false; // Verify equilibrium validity
        out_coefficients[i] = static_cast<int>(final_val);
    }

    return true;
}

} // namespace ChemEngine
