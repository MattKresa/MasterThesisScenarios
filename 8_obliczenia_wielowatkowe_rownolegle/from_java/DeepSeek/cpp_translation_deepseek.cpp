#include <iostream>
#include <vector>
#include <thread>
#include <future>
#include <cmath>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <atomic>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

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
    for (int i = start; i < end; ++i) {
        if (isPrime(i)) {
            primes.push_back(i);
        }
    }
    return primes;
}

// --- I/O-BOUND: save results to a file using a separate thread ---
void asyncSaveToFile(const std::vector<int>& primes, const std::string& filename) {
    std::thread([primes, filename]() {
        std::cout << "Writing " << primes.size() << " primes to " << filename << std::endl;
        std::ofstream outFile(filename);
        if (outFile.is_open()) {
            for (int p : primes) {
                outFile << p << '\n';
            }
            outFile.close();
            std::cout << "Done writing " << filename << std::endl;
        }
        else {
            std::cerr << "Failed to open file: " << filename << std::endl;
        }
        }).detach();
}

// --- CPU + I/O: function executed in each process ---
std::string processRangeAndSave(int start, int end, const std::string& outputDir, int processId) {
    std::ostringstream oss;
    oss << "Process " << processId << " working on range " << start << "-" << end;
    std::cout << oss.str() << std::endl;
    oss.str("");

    auto primes = findPrimesInRange(start, end);

    std::ostringstream filenameStream;
    filenameStream << outputDir << "/primes_" << start << "_" << end << ".txt";
    std::string filename = filenameStream.str();

    std::thread ioThread = std::thread(asyncSaveToFile, primes, filename);
    ioThread.join(); // Wait for the file to finish saving

    oss << "Process " << processId << " finished (" << primes.size() << " primes found)";
    return oss.str();
}

// --- Main runner function ---
void runParallelPrimeFinder(int N, int numProcesses, const std::string& outputDir) {
    fs::create_directories(outputDir);
    int chunkSize = N / numProcesses;

    std::vector<std::tuple<int, int, int>> ranges;
    for (int i = 0; i < numProcesses; ++i) {
        int start = i * chunkSize;
        int end = (i < numProcesses - 1) ? (i + 1) * chunkSize : N;
        ranges.emplace_back(start, end, i);
    }

    std::cout << "Finding prime numbers from 1 to " << N
        << " using " << numProcesses << " processes..." << std::endl << std::endl;

    auto startTime = std::chrono::high_resolution_clock::now();

    std::vector<std::future<std::string>> futures;
    for (const auto& range : ranges) {
        auto [start, end, processId] = range;
        futures.push_back(std::async(std::launch::async, processRangeAndSave,
            start, end, outputDir, processId));
    }

    for (auto& future : futures) {
        try {
            std::cout << future.get() << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    std::cout << "\nCompleted in " << std::fixed << std::setprecision(2)
        << duration / 1000.0 << " seconds" << std::endl;
}

int main() {
    unsigned int numCores = std::thread::hardware_concurrency();
    runParallelPrimeFinder(5'000'000, numCores, "prime_output");
    return 0;
}