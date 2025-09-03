#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <string>
#include <numeric>
#include <thread>
#include <future>
#include <chrono>
#include <filesystem>

// --- CPU-BOUND: check if number is prime ---
bool isPrime(int n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    int sqrtN = static_cast<int>(sqrt(n)) + 1;
    for (int i = 3; i < sqrtN; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

// --- CPU-BOUND: find all primes in a given range ---
std::vector<int> findPrimesInRange(int start, int end) {
    std::vector<int> primes;
    for (int i = start; i < end; i++) {
        if (isPrime(i)) {
            primes.push_back(i);
        }
    }
    return primes;
}

// --- I/O-BOUND: save results to a file using a separate thread ---
std::thread asyncSaveToFile(const std::vector<int>& primes, const std::string& filename) {
    return std::thread([primes, filename]() {
        std::cout << "Writing " << primes.size() << " primes to " << filename << std::endl;
        std::ofstream writer(filename);
        if (writer.is_open()) {
            for (int p : primes) {
                writer << p << std::endl;
            }
            writer.close();
        }
        else {
            std::cerr << "Error: Unable to open file " << filename << std::endl;
        }
        std::cout << "Done writing " << filename << std::endl;
        });
}

// --- CPU + I/O: function executed in each process/thread ---
std::string processRangeAndSave(int start, int end, const std::string& outputDir, int processId) {
    std::cout << "Thread " << processId << " working on range " << start << "-" << end << std::endl;
    std::vector<int> primes = findPrimesInRange(start, end);
    std::string filename = outputDir + "/primes_" + std::to_string(start) + "_" + std::to_string(end) + ".txt";

    std::thread ioThread = asyncSaveToFile(primes, filename);
    if (ioThread.joinable()) {
        ioThread.join(); // Wait for the file to finish saving
    }

    return "Thread " + std::to_string(processId) + " finished (" + std::to_string(primes.size()) + " primes found)";
}

// --- Main runner function ---
void runParallelPrimeFinder(int N, int numThreads, const std::string& outputDir) {
    std::filesystem::create_directories(outputDir);
    int chunkSize = N / numThreads;

    std::vector<std::tuple<int, int, int>> ranges;
    for (int i = 0; i < numThreads; ++i) {
        int start = i * chunkSize;
        int end = (i < numThreads - 1) ? (i + 1) * chunkSize : N;
        ranges.emplace_back(start, end, i);
    }

    std::cout << "Finding prime numbers from 1 to " << N << " using " << numThreads << " threads...\n\n";

    auto startTime = std::chrono::high_resolution_clock::now();

    std::vector<std::future<std::string>> futures;
    std::vector<std::thread> threads;

    for (const auto& range : ranges) {
        int start, end, processId;
        std::tie(start, end, processId) = range;

        // Use std::async for a higher-level approach to thread management
        // std::async will return a std::future which can be used to get the result
        futures.push_back(std::async(std::launch::async, processRangeAndSave, start, end, outputDir, processId));
    }

    for (auto& future : futures) {
        try {
            std::cout << future.get() << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }

    auto duration = std::chrono::high_resolution_clock::now() - startTime;
    double seconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() / 1000.0;
    std::cout << "\nCompleted in " << std::fixed << std::setprecision(2) << seconds << " seconds" << std::endl;
}

int main() {
    int numCores = std::thread::hardware_concurrency();
    if (numCores == 0) {
        numCores = 4; // Default to a reasonable number if hardware_concurrency() is not supported
    }
    runParallelPrimeFinder(5'000'000, numCores, "prime_output");
    return 0;
}