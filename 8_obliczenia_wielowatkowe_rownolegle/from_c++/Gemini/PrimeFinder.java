// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/PrimeFinder.java) ---
package org.example;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.*;

public class PrimeFinder {

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
        List<Long> primes = new ArrayList<>();
        for (long n = start; n < end; ++n) {
            if (isPrime(n)) {
                primes.add(n);
            }
        }
        return primes;
    }

    // --- I/O-BOUND: save results to a file using a separate thread ---
    public static void saveToFile(List<Long> primes, String filename) {
        System.out.println("Writing " + primes.size() + " primes to " + filename);
        try (BufferedWriter writer = new BufferedWriter(new FileWriter(filename))) {
            for (Long p : primes) {
                writer.write(p.toString());
                writer.newLine();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        System.out.println("Done writing " + filename);
    }

    // --- CPU + I/O: function executed in each thread ---
    public static String processRangeAndSave(long start, long end, String outputDir, int processId) {
        System.out.println("Process " + processId + " working on range " + start + "-" + end);
        List<Long> primes = findPrimesInRange(start, end);
        String filename = outputDir + "/primes_" + start + "_" + end + ".txt";

        // Create a new thread for the I/O operation
        Thread writerThread = new Thread(() -> saveToFile(primes, filename));
        writerThread.start();
        try {
            writerThread.join(); // Wait for the file to finish saving
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }

        return "Process " + processId + " finished (" + primes.size() + " primes found)";
    }

    // --- Main runner function ---
    public static void runParallelPrimeFinder(long N, int numThreads, String outputDir) {
        try {
            Path outputDirPath = Paths.get(outputDir);
            Files.createDirectories(outputDirPath);
        } catch (IOException e) {
            e.printStackTrace();
            return;
        }

        long chunkSize = N / numThreads;

        System.out.println("Finding prime numbers from 1 to " + N
                + " using " + numThreads + " threads...\n");

        long startTime = System.nanoTime();

        ExecutorService executor = Executors.newFixedThreadPool(numThreads);
        List<Future<String>> futures = new ArrayList<>();

        for (int i = 0; i < numThreads; ++i) {
            long rangeStart = i * chunkSize;
            long rangeEnd = (i == numThreads - 1) ? N : (i + 1) * chunkSize;

            // Submit the task to the executor
            Callable<String> task = () -> processRangeAndSave(rangeStart, rangeEnd, outputDir, i);
            futures.add(executor.submit(task));
        }

        for (Future<String> future : futures) {
            try {
                System.out.println(future.get());
            } catch (InterruptedException | ExecutionException e) {
                e.printStackTrace();
            }
        }

        executor.shutdown();

        long endTime = System.nanoTime();
        double seconds = (endTime - startTime) / 1_000_000_000.0;
        System.out.printf("\nCompleted in %.2f seconds%n", seconds);
    }

    public static void main(String[] args) {
        int threads = Runtime.getRuntime().availableProcessors();
        if (threads == 0) threads = 4; // fallback
        runParallelPrimeFinder(5_000_000L, threads, "prime_output");
    }
}