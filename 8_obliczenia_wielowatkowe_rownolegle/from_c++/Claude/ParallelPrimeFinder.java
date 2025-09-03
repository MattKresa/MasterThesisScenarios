// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/ParallelPrimeFinder.java) ---
package org.example;

import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.concurrent.*;
import java.time.Instant;
import java.time.Duration;

public class ParallelPrimeFinder {

    // --- CPU-BOUND: check if number is prime ---
    public static boolean isPrime(long n) {
        if (n < 2) return false;
        if (n == 2) return true;
        if (n % 2 == 0) return false;

        long sqrtN = (long) Math.sqrt(n) + 1;
        for (long i = 3; i <= sqrtN; i += 2) {
            if (n % i == 0) return false;
        }
        return true;
    }

    // --- CPU-BOUND: find all primes in a given range ---
    public static List<Long> findPrimesInRange(long start, long end) {
        List<Long> primes = new ArrayList<>((int) ((end - start) / 10)); // initial capacity estimate
        for (long n = start; n < end; n++) {
            if (isPrime(n)) {
                primes.add(n);
            }
        }
        return primes;
    }

    // --- I/O-BOUND: save results to a file using a separate thread ---
    public static CompletableFuture<Void> asyncSaveToFile(List<Long> primes, String filename) {
        return CompletableFuture.runAsync(() -> {
            System.out.println("Writing " + primes.size() + " primes to " + filename);
            try (PrintWriter writer = new PrintWriter(new FileWriter(filename))) {
                for (Long p : primes) {
                    writer.println(p);
                }
                System.out.println("Done writing " + filename);
            } catch (IOException e) {
                System.err.println("Error writing to file " + filename + ": " + e.getMessage());
            }
        });
    }

    // --- CPU + I/O: function executed in each "process" (thread) ---
    public static String processRangeAndSave(long start, long end, String outputDir, int processId) {
        System.out.println("Process " + processId + " working on range " + start + "-" + end);

        List<Long> primes = findPrimesInRange(start, end);
        String filename = outputDir + "/primes_" + start + "_" + end + ".txt";

        CompletableFuture<Void> writer = asyncSaveToFile(primes, filename);
        try {
            writer.get(); // Wait for the file to finish saving
        } catch (InterruptedException | ExecutionException e) {
            System.err.println("Error in file writing: " + e.getMessage());
        }

        return "Process " + processId + " finished (" + primes.size() + " primes found)";
    }

    // --- Main runner function ---
    public static void runParallelPrimeFinder(long N, int numThreads, String outputDir) {
        try {
            Files.createDirectories(Paths.get(outputDir));
        } catch (IOException e) {
            System.err.println("Error creating directory: " + e.getMessage());
            return;
        }

        long chunkSize = N / numThreads;

        System.out.println("Finding prime numbers from 1 to " + N +
                " using " + numThreads + " threads...\n");

        Instant startTime = Instant.now();

        // Use ExecutorService for managing threads
        try (ExecutorService executor = Executors.newFixedThreadPool(numThreads)) {
            List<Future<String>> futures = new ArrayList<>(numThreads);

            for (int i = 0; i < numThreads; i++) {
                long rangeStart = (long) i * chunkSize;
                long rangeEnd = (i == numThreads - 1) ? N : (long) (i + 1) * chunkSize;

                Future<String> future = executor.submit(() ->
                        processRangeAndSave(rangeStart, rangeEnd, outputDir, i));
                futures.add(future);
            }

            // Wait for all futures to complete and print results
            for (Future<String> future : futures) {
                try {
                    System.out.println(future.get());
                } catch (InterruptedException | ExecutionException e) {
                    System.err.println("Error in thread execution: " + e.getMessage());
                }
            }
        }

        Instant endTime = Instant.now();
        double seconds = Duration.between(startTime, endTime).toNanos() / 1_000_000_000.0;
        System.out.println("\nCompleted in " + seconds + " seconds");
    }

    public static void main(String[] args) {
        int threads = Runtime.getRuntime().availableProcessors();
        if (threads == 0) threads = 4; // fallback

        runParallelPrimeFinder(5_000_000L, threads, "prime_output");
    }
}