// proj_root/tests/ChemEngineImplTest.cpp
#include "../src/engine/ChemEngine.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <future>
#include <chrono>

const int NUM_TESTS = 3;
const auto TIMEOUT_THRESHOLD = std::chrono::milliseconds(1500); // 1.5-second time limit per computation

// Global storage arrays for test data pools
std::vector<std::string> global_reactants[NUM_TESTS];
std::vector<std::string> global_products[NUM_TESTS];
std::vector<int> global_coefficients[NUM_TESTS];
bool global_success_states[NUM_TESTS];
bool global_timeout_states[NUM_TESTS]; // Tracks whether a calculation locked up the thread

std::mutex cout_mutex;

// Worker subroutine wrapped via standard promises to catch processing locks
void run_parallel_test(int test_index, std::promise<bool>&& completion_promise) {
    global_success_states[test_index] = ChemEngine::balance(
        global_reactants[test_index], 
        global_products[test_index], 
        global_coefficients[test_index]
    );
    
    // Notify the parent listener thread that calculations finished cleanly
    completion_promise.set_value(true);

    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "[Thread " << std::this_thread::get_id() << "] Test " << test_index << " logic completed processing.\n";
}

// Helper function to print individual assertion checks clearly
void print_assert_status(const std::string& check_name, bool condition) {
    std::cout << "  [ASSERT] " << check_name << ": " << (condition ? "PASSED" : "FAILED") << "\n";
    assert(condition);
}

int main() {
    // --- Populate Global Test Data Pools ---
    global_reactants[0] = {"C6H12O6", "O2"};
    global_products[0]  = {"CO2", "H2O"};

    global_reactants[1] = {"C", "O2"};
    global_products[1]  = {"CO2"};

    global_reactants[2] = {"Al", "H2SO4"};
    global_products[2]  = {"Al2(SO4)3", "H2"};

    std::cout << "Launching " << NUM_TESTS << " validation tests across independent threads with timeout monitoring...\n\n";

    std::thread workers[NUM_TESTS];
    std::future<bool> futures[NUM_TESTS];

    // --- Spawn Concurrent Thread Pool with Futures ---
    for (int i = 0; i < NUM_TESTS; ++i) {
        std::promise<bool> p;
        futures[i] = p.get_future();
        global_timeout_states[i] = false;
        
        workers[i] = std::thread(run_parallel_test, i, std::move(p));
    }

    // --- Process and Monitor Thread Futures for Timeouts ---
    for (int i = 0; i < NUM_TESTS; ++i) {
        // Wait on the future result up to our rigid time ceiling
        if (futures[i].wait_for(TIMEOUT_THRESHOLD) == std::future_status::timeout) {
            global_timeout_states[i] = true; // Flag structural computation lockup
            global_success_states[i] = false;
        }

        // Clean up the operating system thread loop safely
        if (workers[i].joinable()) {
            workers[i].join();
        }
    }

    std::cout << "\nExecuting and printing detailed validation checks:\n";

    // --- Test 0 Checks: Cellular Respiration ---
    std::cout << "\nEvaluating Test 0 (Cellular Respiration):\n";
    print_assert_status("Thread completed execution within time limit", global_timeout_states[0] == false);
    print_assert_status("Solver returned success status", global_success_states[0] == true);
    print_assert_status("Coefficient count matches formula parameters", global_coefficients[0].size() == 4);
    print_assert_status("C6H12O6 coefficient is exactly 1", global_coefficients[0][0] == 1);
    print_assert_status("O2 coefficient is exactly 6",       global_coefficients[0][1] == 6);
    print_assert_status("CO2 coefficient is exactly 6",      global_coefficients[0][2] == 6);
    print_assert_status("H2O coefficient is exactly 6",      global_coefficients[0][3] == 6);

    // --- Test 1 Checks: Basic Carbon Combustion ---
    std::cout << "\nEvaluating Test 1 (Carbon Combustion):\n";
    print_assert_status("Thread completed execution within time limit", global_timeout_states[1] == false);
    print_assert_status("Solver returned success status", global_success_states[1] == true);
    print_assert_status("Coefficient count matches formula parameters", global_coefficients[1].size() == 3);
    print_assert_status("C coefficient is exactly 1",   global_coefficients[1][0] == 1);
    print_assert_status("O2 coefficient is exactly 1",  global_coefficients[1][1] == 1);
    print_assert_status("CO2 coefficient is exactly 1", global_coefficients[1][2] == 1);

    // --- Test 2 Checks: Aluminum Sulfate Reaction ---
    std::cout << "\nEvaluating Test 2 (Aluminum Sulfate Reaction):\n";
    print_assert_status("Thread completed execution within time limit", global_timeout_states[2] == false);
    print_assert_status("Solver returned success status", global_success_states[2] == true);
    print_assert_status("Coefficient count matches formula parameters", global_coefficients[2].size() == 4);
    print_assert_status("Al coefficient is exactly 2",        global_coefficients[2][0] == 2);
    print_assert_status("H2SO4 coefficient is exactly 3",    global_coefficients[2][1] == 3);
    print_assert_status("Al2(SO4)3 coefficient is exactly 1", global_coefficients[2][2] == 1);
    print_assert_status("H2 coefficient is exactly 3",        global_coefficients[2][3] == 3);

    std::cout << "\nAll concurrent validation assertions printed, monitored, and passed completely!\n";
    return 0;
}
