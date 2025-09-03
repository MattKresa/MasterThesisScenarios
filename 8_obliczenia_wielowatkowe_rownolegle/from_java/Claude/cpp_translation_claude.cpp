#include <iostream>
#include <vector>
#include <thread>
#include <future>
#include <fstream>
#include <string>
#include <chrono>
#include <filesystem>
#include <cmath>
#include <algorithm>

class ParallelPrimeFinder {
public:
    // --- CPU-BOUND: check if number is prime ---
    static bool isPrime(int n) {
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
    static std::vector<int> findPrimesInRange(int start, int end) {
        std::vector<int> primes;
        for (int i = start; i < end; i++) {
            if (isPrime(i)) {
                primes.push_back(i);
            }
        }
        return primes;
    }

    // --- I/O-BOUND: save results to a file using a separate thread ---
    static std::thread asyncSaveToFile(const std::vector<int>& primes, const std::string& filename) {
        return std::thread([primes, filename]() {
            std::printf("Writing %zu primes to %s\n", primes.size(), filename.c_str());

            try {
                std::ofstream writer(filename);
                if (!writer.is_open()) {
                    throw std::runtime_error("Cannot open file: " + filename);
                }

                for (int p : primes) {
                    writer << p << '\n';
                }
                writer.close();
            }
            catch (const std::exception& e) {
                std::cerr << "Error writing file: " << e.what() << std::endl;
            }

            std::printf("Done writing %s\n", filename.c_str());
            });
    }

    // --- CPU + I/O: function executed in each process ---
    static std::string processRangeAndSave(int start, int end, const std::string& outputDir, int processId) {
        std::printf("Process %d working on range %d-%d\n", processId, start, end);

        std::vector<int> primes = findPrimesInRange(start, end);
        std::string filename = outputDir + "/" + "primes_" + std::to_string(start) + "_" + std::to_string(end) + ".txt";

        std::thread ioThread = asyncSaveToFile(primes, filename);
        ioThread.join(); // Wait for the file to finish saving

        return "Process " + std::to_string(processId) + " finished (" + std::to_string(primes.size()) + " primes found)";
    }

    // --- Main runner function ---
    static void runParallelPrimeFinder(int N, int numProcesses, const std::string& outputDir) {
        // Create output directory if it doesn't exist
        std::filesystem::create_directories(outputDir);

        int chunkSize = N / numProcesses;

        // Create ranges for each process
        std::vector<std::tuple<int, int, std::string, int>> ranges;
        for (int i = 0; i < numProcesses; i++) {
            int start = i * chunkSize;
            int end = (i < numProcesses - 1) ? (i + 1) * chunkSize : N;
            ranges.emplace_back(start, end, outputDir, i);
        }

        std::printf("Finding prime numbers from 1 to %d using %d processes...\n\n", N, numProcesses);

        auto startTime = std::chrono::high_resolution_clock::now();

        // Create futures for parallel execution
        std::vector<std::future<std::string>> futures;

        for (const auto& range : ranges) {
            auto future = std::async(std::launch::async, [range]() {
                return processRangeAndSave(
                    std::get<0>(range),  // start
                    std::get<1>(range),  // end
                    std::get<2>(range),  // outputDir
                    std::get<3>(range)   // processId
                );
                });
            futures.push_back(std::move(future));
        }

        // Collect results
        for (auto& future : futures) {
            try {
                std::string result = future.get();
                std::cout << result << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "Error in process: " << e.what() << std::endl;
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        std::printf("\nCompleted in %.2f seconds\n", duration.count() / 1000.0);
    }
};

int main() {
    unsigned int numCores = std::thread::hardware_concurrency();
    if (numCores == 0) numCores = 4; // Fallback if detection fails

    ParallelPrimeFinder::runParallelPrimeFinder(5'000'000, numCores, "prime_output");

    return 0;
}