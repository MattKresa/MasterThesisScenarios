#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <cmath>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <future>
#include <algorithm>

namespace fs = std::filesystem;

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
std::future<void> async_save_to_file(const std::vector<int>& primes, const std::string& filename) {
    return std::async(std::launch::async, [primes, filename]() {
        std::cout << "Writing " << primes.size() << " primes to " << filename << std::endl;
        std::ofstream f(filename);
        for (const auto& p : primes) {
            f << p << "\n";
        }
        std::cout << "Done writing " << filename << std::endl;
        });
}

// --- CPU + I/O: function executed in each process ---
std::string process_range_and_save(int start, int end, const std::string& output_dir, int process_id) {
    std::cout << "Process " << process_id << " working on range " << start << "-" << end << std::endl;
    auto primes = find_primes_in_range(start, end);
    std::string filename = output_dir + "/primes_" + std::to_string(start) + "_" + std::to_string(end) + ".txt";
    auto future = async_save_to_file(primes, filename);
    future.wait();  // Wait for the file to finish saving
    return "Process " + std::to_string(process_id) + " finished (" + std::to_string(primes.size()) + " primes found)";
}

// --- Main runner function ---
void run_parallel_prime_finder(int N = 1'000'000, int num_processes = 4, const std::string& output_dir = "prime_output") {
    fs::create_directories(output_dir);
    int chunk_size = N / num_processes;
    std::vector<std::future<std::string>> futures;

    std::cout << "Finding prime numbers from 1 to " << N << " using " << num_processes << " processes...\n" << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_processes; ++i) {
        int start = i * chunk_size;
        int end = (i < num_processes - 1) ? (i + 1) * chunk_size : N;
        futures.push_back(std::async(std::launch::async, process_range_and_save, start, end, output_dir, i));
    }

    for (auto& future : futures) {
        std::cout << future.get() << std::endl;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    std::cout << "\nCompleted in " << elapsed.count() << " seconds" << std::endl;
}

int main() {
    unsigned int num_processes = std::thread::hardware_concurrency();
    run_parallel_prime_finder(5'000'000, num_processes);
    return 0;
}