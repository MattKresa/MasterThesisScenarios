#include <iostream>
#include <vector>
#include <thread>
#include <future>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <chrono>
#include <string>
#include <sstream>

// --- CPU-BOUND: check if number is prime ---
bool is_prime(int n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;

    int sqrt_n = static_cast<int>(std::sqrt(n)) + 1;
    for (int i = 3; i < sqrt_n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

// --- CPU-BOUND: find all primes in a given range ---
std::vector<int> find_primes_in_range(int start, int end) {
    std::vector<int> primes;
    for (int n = start; n < end; ++n) {
        if (is_prime(n)) {
            primes.push_back(n);
        }
    }
    return primes;
}

// --- I/O-BOUND: save results to a file using a separate thread ---
std::thread async_save_to_file(const std::vector<int>& primes, const std::string& filename) {
    return std::thread([primes, filename]() {
        std::cout << "Writing " << primes.size() << " primes to " << filename << std::endl;

        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return;
        }

        for (int prime : primes) {
            file << prime << "\n";
        }
        file.close();

        std::cout << "Done writing " << filename << std::endl;
        });
}

// --- CPU + I/O: function executed in each thread ---
std::string process_range_and_save(int start, int end, const std::string& output_dir, int process_id) {
    std::cout << "Process " << process_id << " working on range " << start << "-" << end << std::endl;

    std::vector<int> primes = find_primes_in_range(start, end);

    std::string filename = output_dir + "/primes_" + std::to_string(start) + "_" + std::to_string(end) + ".txt";
    std::thread save_thread = async_save_to_file(primes, filename);
    save_thread.join();  // Wait for the file to finish saving

    std::stringstream result;
    result << "Process " << process_id << " finished (" << primes.size() << " primes found)";
    return result.str();
}

// --- Main runner function ---
void run_parallel_prime_finder(int N = 1000000, int num_processes = 4, const std::string& output_dir = "prime_output") {
    // Create output directory
    std::filesystem::create_directories(output_dir);

    int chunk_size = N / num_processes;
    std::vector<std::future<std::string>> futures;

    std::cout << "Finding prime numbers from 1 to " << N << " using " << num_processes << " processes...\n" << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    // Launch threads
    for (int i = 0; i < num_processes; ++i) {
        int start = i * chunk_size;
        int end = (i < num_processes - 1) ? (i + 1) * chunk_size : N;

        futures.push_back(std::async(std::launch::async, process_range_and_save, start, end, output_dir, i));
    }

    // Wait for all threads to complete and collect results
    for (auto& future : futures) {
        std::string result = future.get();
        std::cout << result << std::endl;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "\nCompleted in " << duration.count() / 1000.0 << " seconds" << std::endl;
}

int main() {
    // Get number of hardware threads (equivalent to multiprocessing.cpu_count())
    int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4; // fallback if detection fails

    run_parallel_prime_finder(5000000, num_threads);

    return 0;
}

// Compile with: g++ -std=c++17 -O2 -pthread parallel_prime_finder.cpp -o parallel_prime_finder