#include <iostream>
#include <vector>
#include <cmath>
#include <thread>
#include <future>
#include <fstream>
#include <string>
#include <filesystem>
#include <chrono>

// --- CPU-BOUND: check if number is prime ---
bool is_prime(unsigned long long n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    unsigned long long sqrt_n = static_cast<unsigned long long>(std::sqrt(n)) + 1;
    for (unsigned long long i = 3; i <= sqrt_n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

// --- CPU-BOUND: find all primes in a given range ---
std::vector<unsigned long long> find_primes_in_range(unsigned long long start, unsigned long long end) {
    std::vector<unsigned long long> primes;
    primes.reserve((end - start) / 10); // wstępny szacunek
    for (unsigned long long n = start; n < end; ++n) {
        if (is_prime(n)) {
            primes.push_back(n);
        }
    }
    return primes;
}

// --- I/O-BOUND: save results to a file using a separate thread ---
std::thread async_save_to_file(const std::vector<unsigned long long>& primes, const std::string& filename) {
    return std::thread([primes, filename]() {
        std::cout << "Writing " << primes.size() << " primes to " << filename << "\n";
        std::ofstream file(filename);
        for (auto p : primes) {
            file << p << "\n";
        }
        std::cout << "Done writing " << filename << "\n";
        });
}

// --- CPU + I/O: function executed in each "process" (thread) ---
std::string process_range_and_save(unsigned long long start, unsigned long long end,
    const std::string& output_dir, int process_id) {
    std::cout << "Process " << process_id << " working on range " << start << "-" << end << "\n";
    auto primes = find_primes_in_range(start, end);
    std::string filename = output_dir + "/primes_" + std::to_string(start) + "_" + std::to_string(end) + ".txt";
    std::thread writer = async_save_to_file(primes, filename);
    writer.join(); // Wait for the file to finish saving
    return "Process " + std::to_string(process_id) + " finished (" + std::to_string(primes.size()) + " primes found)";
}

// --- Main runner function ---
void run_parallel_prime_finder(unsigned long long N, unsigned int num_threads, const std::string& output_dir) {
    std::filesystem::create_directories(output_dir);
    unsigned long long chunk_size = N / num_threads;

    std::vector<std::future<std::string>> futures;
    futures.reserve(num_threads);

    std::cout << "Finding prime numbers from 1 to " << N
        << " using " << num_threads << " threads...\n\n";

    auto start_time = std::chrono::steady_clock::now();

    for (unsigned int i = 0; i < num_threads; ++i) {
        unsigned long long range_start = i * chunk_size;
        unsigned long long range_end = (i == num_threads - 1) ? N : (i + 1) * chunk_size;
        futures.push_back(std::async(std::launch::async, process_range_and_save,
            range_start, range_end, output_dir, i));
    }

    for (auto& f : futures) {
        std::cout << f.get() << "\n";
    }

    auto end_time = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(end_time - start_time).count();
    std::cout << "\nCompleted in " << seconds << " seconds\n";
}

int main() {
    unsigned int threads = std::thread::hardware_concurrency();
    if (threads == 0) threads = 4; // fallback
    run_parallel_prime_finder(5'000'000ULL, threads, "prime_output");
    return 0;
}
