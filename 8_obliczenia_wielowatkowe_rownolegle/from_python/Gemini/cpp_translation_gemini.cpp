#include <iostream>
#include <vector>
#include <cmath>
#include <numeric>
#include <thread>
#include <future>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <functional>

// Use a namespace alias for easier access to std::filesystem
namespace fs = std::filesystem;

// --- CPU-BOUND: check if number is prime ---
// This function checks if a given number `n` is prime.
// It uses a standard optimization by only checking for divisors up to the square root of `n`.
bool is_prime(long long n) {
    if (n < 2) {
        return false;
    }
    if (n == 2) {
        return true;
    }
    if (n % 2 == 0) {
        return false;
    }
    long long sqrt_n = static_cast<long long>(std::sqrt(n)) + 1;
    for (long long i = 3; i < sqrt_n; i += 2) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}

// --- CPU-BOUND: find all primes in a given range ---
// This function iterates through a range of numbers and uses `is_prime` to find all primes.
// The results are stored in a vector.
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
// This function launches a new thread to asynchronously write a list of primes to a file.
// It returns the `std::thread` object so the caller can wait for it to finish.
std::thread async_save_to_file(const std::vector<long long>& primes, const std::string& filename) {
    auto write_primes = [primes, filename]() {
        std::cout << "Writing " << primes.size() << " primes to " << filename << std::endl;
        std::ofstream file(filename);
        if (file.is_open()) {
            for (long long p : primes) {
                file << p << "\n";
            }
            std::cout << "Done writing " << filename << std::endl;
            file.close();
        }
        else {
            std::cerr << "Error opening file: " << filename << std::endl;
        }
        };
    return std::thread(write_primes);
}

// --- CPU + I/O: function executed in each parallel task ---
// This is the worker function for each parallel task. It finds primes in a sub-range
// and then saves them to a file using an I/O thread. It waits for the I/O thread to complete
// before returning its result string.
std::string process_range_and_save(long long start, long long end, const fs::path& output_dir, int process_id) {
    std::cout << "Task " << process_id << " working on range " << start << "-" << end << std::endl;

    std::vector<long long> primes = find_primes_in_range(start, end);

    std::stringstream filename_ss;
    filename_ss << "primes_" << start << "_" << end << ".txt";
    fs::path filepath = output_dir / filename_ss.str();

    // Launch a new thread to handle the I/O-bound task.
    std::thread io_thread = async_save_to_file(primes, filepath.string());

    // Wait for the I/O thread to finish.
    io_thread.join();

    std::stringstream result_ss;
    result_ss << "Task " << process_id << " finished (" << primes.size() << " primes found)";
    return result_ss.str();
}

// --- Main runner function ---
// This function orchestrates the parallel prime finding process.
void run_parallel_prime_finder(long long N = 5'000'000, unsigned int num_tasks = std::thread::hardware_concurrency()) {
    std::string output_dir_str = "prime_output_cpp";
    fs::path output_dir(output_dir_str);
    fs::create_directories(output_dir);

    // Calculate chunk size and define the ranges for each task.
    long long chunk_size = N / num_tasks;
    std::vector<std::pair<long long, long long>> ranges;
    for (unsigned int i = 0; i < num_tasks; ++i) {
        long long start = i * chunk_size;
        long long end = (i < num_tasks - 1) ? (i + 1) * chunk_size : N;
        ranges.push_back({ start, end });
    }

    std::cout << "Finding prime numbers from 1 to " << N << " using " << num_tasks << " tasks...\n" << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();

    // Use std::async to run the tasks in parallel. `std::async` provides a cleaner
    // way to manage a thread pool and retrieve results via `std::future`.
    std::vector<std::future<std::string>> futures;
    for (unsigned int i = 0; i < num_tasks; ++i) {
        futures.push_back(std::async(std::launch::async, process_range_and_save, ranges[i].first, ranges[i].second, output_dir, i));
    }

    // Wait for all tasks to complete and print their results.
    for (auto& f : futures) {
        std::cout << f.get() << std::endl;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    std::cout << "\nCompleted in " << elapsed.count() << " seconds" << std::endl;
}

int main() {
    // The C++ equivalent of multiprocessing.cpu_count() is std::thread::hardware_concurrency().
    run_parallel_prime_finder();
    return 0;
}
