// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/ParallelPrimeFinder.java) ---
package org.example;

import java.io.*;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;
import java.util.concurrent.*;
import java.util.stream.IntStream;

public class ParallelPrimeFinder {

    // --- CPU-BOUND: check if number is prime ---
    public static boolean isPrime(int n) {
        if (n < 2) return false;
        if (n == 2) return true;
        if (n % 2 == 0) return false;

        int sqrtN = (int) Math.sqrt(n) + 1;
        for (int i = 3; i < sqrtN; i += 2) {
            if (n % i == 0) return false;
        }
        return true;
    }

    // --- CPU-BOUND: find all primes in a given range ---
    public static List<Integer> findPrimesInRange(int start, int end) {
        return IntStream.range(start, end)
                .filter(ParallelPrimeFinder::isPrime)
                .boxed()
                .collect(ArrayList::new, List::add, List::addAll);
    }

    // --- I/O-BOUND: save results to a file using a separate thread ---
    public static Thread asyncSaveToFile(List<Integer> primes, String filename) {
        Thread thread = new Thread(() -> {
            System.out.println("Writing " + primes.size() + " primes to " + filename);

            try (PrintWriter writer = new PrintWriter(new FileWriter(filename))) {
                for (Integer prime : primes) {
                    writer.println(prime);
                }
                System.out.println("Done writing " + filename);
            } catch (IOException e) {
                System.err.println("Error writing to file " + filename + ": " + e.getMessage());
            }
        });

        thread.start();
        return thread;
    }

    // --- CPU + I/O: function executed in each thread ---
    public static String processRangeAndSave(int start, int end, String outputDir, int processId) {
        System.out.println("Process " + processId + " working on range " + start + "-" + end);

        List<Integer> primes = findPrimesInRange(start, end);

        String filename = Paths.get(outputDir, "primes_" + start + "_" + end + ".txt").toString();
        Thread saveThread = asyncSaveToFile(primes, filename);

        try {
            saveThread.join(); // Wait for the file to finish saving
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            return "Process " + processId + " interrupted";
        }

        return "Process " + processId + " finished (" + primes.size() + " primes found)";
    }

    // --- Main runner function ---
    public static void runParallelPrimeFinder(int N, int numProcesses, String outputDir)
            throws IOException, InterruptedException, ExecutionException {

        // Create output directory
        Path outputPath = Paths.get(outputDir);
        Files.createDirectories(outputPath);

        int chunkSize = N / numProcesses;
        List<CompletableFuture<String>> futures = new ArrayList<>();

        System.out.println("Finding prime numbers from 1 to " + N +
                " using " + numProcesses + " processes...\n");

        long startTime = System.currentTimeMillis();

        // Create executor service for parallel processing
        ExecutorService executor = Executors.newFixedThreadPool(numProcesses);

        try {
            // Submit tasks
            for (int i = 0; i < numProcesses; i++) {
                final int start = i * chunkSize;
                final int end = (i < numProcesses - 1) ? (i + 1) * chunkSize : N;
                final int processId = i;

                CompletableFuture<String> future = CompletableFuture.supplyAsync(() ->
                        processRangeAndSave(start, end, outputDir, processId), executor);

                futures.add(future);
            }

            // Wait for all tasks to complete and collect results
            for (CompletableFuture<String> future : futures) {
                String result = future.get();
                System.out.println(result);
            }

        } finally {
            executor.shutdown();
            if (!executor.awaitTermination(60, TimeUnit.SECONDS)) {
                executor.shutdownNow();
            }
        }

        long endTime = System.currentTimeMillis();
        double duration = (endTime - startTime) / 1000.0;

        System.out.printf("\nCompleted in %.2f seconds\n", duration);
    }

    // Overloaded method with default parameters
    public static void runParallelPrimeFinder()
            throws IOException, InterruptedException, ExecutionException {
        runParallelPrimeFinder(1_000_000, 4, "prime_output");
    }

    public static void main(String[] args) {
        try {
            // Get number of available processors (equivalent to multiprocessing.cpu_count())
            int numProcessors = Runtime.getRuntime().availableProcessors();
            runParallelPrimeFinder(5_000_000, numProcessors, "prime_output");

        } catch (IOException | InterruptedException | ExecutionException e) {
            System.err.println("Error during execution: " + e.getMessage());
            e.printStackTrace();
        }
    }
}