#include <iostream>
#include <vector>
#include <cmath>
#include <thread>
#include <future>
#include <fstream>
#include <filesystem>
#include <string>
#include <chrono>

namespace fs = std::filesystem;

// --- CPU-BOUND: check if number is prime ---
bool is_prime(long long n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    long long sqrt_n = static_cast<long long>(std::sqrt(n)) + 1;
    for (long long i = 3; i <= sqrt_n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

// --- CPU-BOUND: find all primes in a given range ---
std::vector<long long> find_primes_in_range(long long start, long long end) {
    std::vector<long long> primes;
    for (long long n = start; n < end; ++n) {
        if (is_prime(n)) {
            primes.push_back(n);
        }
    }
    return primes;
}

// --- I/O-BOUND: save results to a file using a separate thread ---
std::thread async_save_to_file(const std::vector<long long>& primes, const fs::path& filename) {
    return std::thread([primes, filename]() {
        std::cout << "Writing " << primes.size() << " primes to " << filename << "\n";
        std::ofstream out(filename);
        for (auto p : primes) {
            out << p << "\n";
        }
        std::cout << "Done writing " << filename << "\n";
        });
}

// --- CPU + I/O: function executed in each thread/task ---
std::string process_range_and_save(long long start, long long end, const fs::path& output_dir, int process_id) {
    std::cout << "Process " << process_id << " working on range " << start << "-" << end << "\n";
    auto primes = find_primes_in_range(start, end);

    fs::path filename = output_dir / ("primes_" + std::to_string(start) + "_" + std::to_string(end) + ".txt");
    auto t = async_save_to_file(primes, filename);
    t.join(); // Wait for the file to finish saving

    return "Process " + std::to_string(process_id) + " finished (" + std::to_string(primes.size()) + " primes found)";
}

// --- Main runner function ---
void run_parallel_prime_finder(long long N, int num_processes, const std::string& output_dir) {
    fs::create_directories(output_dir);
    long long chunk_size = N / num_processes;

    std::vector<std::future<std::string>> futures;

    auto start_time = std::chrono::steady_clock::now();
    std::cout << "Finding prime numbers from 1 to " << N
        << " using " << num_processes << " threads...\n\n";

    for (int i = 0; i < num_processes; ++i) {
        long long range_start = i * chunk_size;
        long long range_end = (i < num_processes - 1) ? (i + 1) * chunk_size : N;
        futures.push_back(std::async(std::launch::async, process_range_and_save,
            range_start, range_end, fs::path(output_dir), i));
    }

    for (auto& f : futures) {
        std::cout << f.get() << "\n";
    }

    auto end_time = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end_time - start_time).count();
    std::cout << "\nCompleted in " << elapsed << " seconds\n";
}

int main() {
    int num_threads = std::thread::hardware_concurrency(); // CPU core count
    run_parallel_prime_finder(5'000'000, num_threads, "prime_output");
    return 0;
}
