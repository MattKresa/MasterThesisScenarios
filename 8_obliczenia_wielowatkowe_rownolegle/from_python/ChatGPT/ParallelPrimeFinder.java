// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/ParallelPrimeFinder.java) ---
package org.example;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.stream.Collectors;

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
        List<Long> primes = new ArrayList<>();
        for (long n = start; n < end; n++) {
            if (isPrime(n)) {
                primes.add(n);
            }
        }
        return primes;
    }

    // --- I/O-BOUND: save results to a file using a separate thread ---
    public static Thread asyncSaveToFile(List<Long> primes, Path filename) {
        Thread thread = new Thread(() -> {
            System.out.println("Writing " + primes.size() + " primes to " + filename);
            try (BufferedWriter writer = new BufferedWriter(new FileWriter(filename.toFile()))) {
                for (Long p : primes) {
                    writer.write(p.toString());
                    writer.newLine();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
            System.out.println("Done writing " + filename);
        });
        thread.start();
        return thread;
    }

    // --- CPU + I/O: function executed in each thread ---
    public static String processRangeAndSave(long start, long end, Path outputDir, int processId) {
        System.out.println("Process " + processId + " working on range " + start + "-" + end);
        List<Long> primes = findPrimesInRange(start, end);

        Path filename = outputDir.resolve("primes_" + start + "_" + end + ".txt");
        Thread t = asyncSaveToFile(primes, filename);
        try {
            t.join(); // wait for file to finish saving
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
        return "Process " + processId + " finished (" + primes.size() + " primes found)";
    }

    // --- Main runner function ---
    public static void runParallelPrimeFinder(long N, int numProcesses, String outputDirStr) {
        Path outputDir = Paths.get(outputDirStr);
        try {
            Files.createDirectories(outputDir);
        } catch (IOException e) {
            e.printStackTrace();
            return;
        }

        long chunkSize = N / numProcesses;
        ExecutorService executor = Executors.newFixedThreadPool(numProcesses);
        List<Future<String>> futures = new ArrayList<>();

        System.out.println("Finding prime numbers from 1 to " + N + " using " + numProcesses + " threads...\n");
        long startTime = System.nanoTime();

        for (int i = 0; i < numProcesses; i++) {
            long rangeStart = i * chunkSize;
            long rangeEnd = (i < numProcesses - 1) ? (i + 1) * chunkSize : N;
            final int processId = i;

            futures.add(executor.submit(() -> processRangeAndSave(rangeStart, rangeEnd, outputDir, processId)));
        }

        executor.shutdown();

        for (Future<String> f : futures) {
            try {
                System.out.println(f.get());
            } catch (InterruptedException | ExecutionException e) {
                e.printStackTrace();
            }
        }

        long endTime = System.nanoTime();
        double elapsedSec = (endTime - startTime) / 1_000_000_000.0;
        System.out.printf("%nCompleted in %.2f seconds%n", elapsedSec);
    }

    public static void main(String[] args) {
        int numCores = Runtime.getRuntime().availableProcessors();
        runParallelPrimeFinder(5_000_000L, numCores, "prime_output");
    }
}
