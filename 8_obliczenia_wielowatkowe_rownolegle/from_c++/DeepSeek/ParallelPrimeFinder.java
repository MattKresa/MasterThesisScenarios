// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/ParallelPrimeFinder.java) ---
package org.example;

import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.concurrent.*;

public class ParallelPrimeFinder {

    // --- CPU-BOUND: check if number is prime ---
    private static boolean isPrime(long n) {
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
    private static List<Long> findPrimesInRange(long start, long end) {
        List<Long> primes = new ArrayList<>((int) ((end - start) / 10)); // initial estimate
        for (long n = start; n < end; ++n) {
            if (isPrime(n)) {
                primes.add(n);
            }
        }
        return primes;
    }

    // --- I/O-BOUND: save results to a file using a separate thread ---
    private static Thread asyncSaveToFile(List<Long> primes, String filename) {
        Runnable task = () -> {
            System.out.println("Writing " + primes.size() + " primes to " + filename);
            try (PrintWriter writer = new PrintWriter(new FileWriter(filename))) {
                for (long p : primes) {
                    writer.println(p);
                }
                System.out.println("Done writing " + filename);
            } catch (IOException e) {
                System.err.println("Error writing to file: " + e.getMessage());
            }
        };
        Thread thread = new Thread(task);
        thread.start();
        return thread;
    }

    // --- CPU + I/O: function executed in each "process" (thread) ---
    private static String processRangeAndSave(long start, long end, String outputDir, int processId) {
        System.out.printf("Process %d working on range %d-%d%n", processId, start, end);
        List<Long> primes = findPrimesInRange(start, end);
        String filename = outputDir + "/primes_" + start + "_" + end + ".txt";
        Thread writer = asyncSaveToFile(primes, filename);
        try {
            writer.join(); // Wait for the file to finish saving
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            return "Process " + processId + " interrupted";
        }
        return "Process " + processId + " finished (" + primes.size() + " primes found)";
    }

    // --- Main runner function ---
    private static void runParallelPrimeFinder(long N, int numThreads, String outputDir) throws IOException {
        Files.createDirectories(Paths.get(outputDir));
        long chunkSize = N / numThreads;

        System.out.printf("Finding prime numbers from 1 to %d using %d threads...%n%n", N, numThreads);

        long startTime = System.nanoTime();

        ExecutorService executor = Executors.newFixedThreadPool(numThreads);
        List<Future<String>> futures = new ArrayList<>(numThreads);

        for (int i = 0; i < numThreads; i++) {
            final long rangeStart = i * chunkSize;
            final long rangeEnd = (i == numThreads - 1) ? N : (i + 1) * chunkSize;
            final int processId = i;
            futures.add(executor.submit(() ->
                    processRangeAndSave(rangeStart, rangeEnd, outputDir, processId)));
        }

        for (Future<String> future : futures) {
            try {
                System.out.println(future.get());
            } catch (InterruptedException | ExecutionException e) {
                System.err.println("Error in task execution: " + e.getMessage());
            }
        }

        executor.shutdown();

        long endTime = System.nanoTime();
        double seconds = (endTime - startTime) / 1_000_000_000.0;
        System.out.printf("%nCompleted in %.3f seconds%n", seconds);
    }

    public static void main(String[] args) {
        int threads = Runtime.getRuntime().availableProcessors();
        if (threads == 0) threads = 4; // fallback
        try {
            runParallelPrimeFinder(5_000_000L, threads, "prime_output");
        } catch (IOException e) {
            System.err.println("Error in prime finder: " + e.getMessage());
        }
    }
}