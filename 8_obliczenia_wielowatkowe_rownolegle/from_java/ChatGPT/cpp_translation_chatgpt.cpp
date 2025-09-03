#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <future>
#include <cmath>
#include <chrono>
#include <filesystem>

// --- CPU-BOUND: check if number is prime ---
bool isPrime(int n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    int sqrtN = static_cast<int>(std::sqrt(n)) + 1;
    for (int i = 3; i < sqrtN; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

// --- CPU-BOUND: find all primes in a given range ---
std::vector<int> findPrimesInRange(int start, int end) {
    std::vector<int> primes;
    primes.reserve(end - start); // Reserve space to reduce reallocations
    for (int i = start; i < end; ++i) {
        if (isPrime(i)) {
            primes.push_back(i);
        }
    }
    return primes;
}

// --- I/O-BOUND: save results to a file using a separate thread ---
std::thread asyncSaveToFile(const std::vector<int>& primes, const std::string& filename) {
    return std::thread([primes, filename]() {
        std::cout << "Writing " << primes.size() << " primes to " << filename << "\n";
        try {
            std::ofstream out(filename);
            if (!out) {
                throw std::ios_base::failure("Failed to open file");
            }
            for (int p : primes) {
                out << p << "\n";
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error writing file " << filename << ": " << e.what() << "\n";
        }
        std::cout << "Done writing " << filename << "\n";
        });
}

// --- CPU + I/O: function executed in each thread ---
std::string processRangeAndSave(int start, int end, const std::string& outputDir, int processId) {
    std::cout << "Process " << processId << " working on range " << start << "-" << end << "\n";
    auto primes = findPrimesInRange(start, end);

    std::string filename = outputDir + "/primes_" + std::to_string(start) + "_" + std::to_string(end) + ".txt";
    std::thread ioThread = asyncSaveToFile(primes, filename);
    ioThread.join(); // Wait for I/O to complete

    return "Process " + std::to_string(processId) + " finished (" + std::to_string(primes.size()) + " primes found)";
}

// --- Main runner function ---
void runParallelPrimeFinder(int N, int numProcesses, const std::string& outputDir) {
    std::filesystem::create_directories(outputDir);
    int chunkSize = N / numProcesses;

    std::vector<std::tuple<int, int, int>> ranges;
    for (int i = 0; i < numProcesses; ++i) {
        int start = i * chunkSize;
        int end = (i < numProcesses - 1) ? (i + 1) * chunkSize : N;
        ranges.emplace_back(start, end, i);
    }

    std::cout << "Finding prime numbers from 1 to " << N
        << " using " << numProcesses << " threads...\n\n";

    auto startTime = std::chrono::high_resolution_clock::now();

    std::vector<std::future<std::string>> futures;
    for (auto& [start, end, processId] : ranges) {
        futures.push_back(std::async(std::launch::async,
            processRangeAndSave,
            start, end, outputDir, processId));
    }

    for (auto& f : futures) {
        try {
            std::cout << f.get() << "\n";
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = endTime - startTime;

    std::cout << "\nCompleted in " << duration.count() << " seconds\n";
}

int main() {
    int numCores = std::thread::hardware_concurrency();
    runParallelPrimeFinder(5'000'000, numCores, "prime_output");
    return 0;
}
