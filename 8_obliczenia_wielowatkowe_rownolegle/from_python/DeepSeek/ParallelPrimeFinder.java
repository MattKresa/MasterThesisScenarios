// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/ParallelPrimeFinder.java) ---
package org.example;

import java.io.*;
import java.util.*;
import java.util.concurrent.*;
import java.lang.Math;

public class ParallelPrimeFinder {

    // --- CPU-BOUND: check if number is prime ---
    public static boolean isPrime(int n) {
        if (n < 2) return false;
        if (n == 2) return true;
        if (n % 2 == 0) return false;
        int sqrt_n = (int) Math.sqrt(n) + 1;
        for (int i = 3; i < sqrt_n; i += 2) {
            if (n % i == 0) return false;
        }
        return true;
    }

    // --- CPU-BOUND: find all primes in a given range ---
    public static List<Integer> findPrimesInRange(int start, int end) {
        List<Integer> primes = new ArrayList<>();
        for (int n = start; n < end; n++) {
            if (isPrime(n)) {
                primes.add(n);
            }
        }
        return primes;
    }

    // --- I/O-BOUND: save results to a file using a separate thread ---
    public static Future<?> asyncSaveToFile(List<Integer> primes, String filename) {
        return CompletableFuture.runAsync(() -> {
            try {
                System.out.println("Writing " + primes.size() + " primes to " + filename);
                BufferedWriter writer = new BufferedWriter(new FileWriter(filename));
                for (int p : primes) {
                    writer.write(p + "\n");
                }
                writer.close();
                System.out.println("Done writing " + filename);
            } catch (IOException e) {
                e.printStackTrace();
            }
        });
    }

    // --- CPU + I/O: function executed in each process ---
    public static String processRangeAndSave(int start, int end, String outputDir, int processId)
            throws InterruptedException, ExecutionException {
        System.out.println("Process " + processId + " working on range " + start + "-" + end);
        List<Integer> primes = findPrimesInRange(start, end);
        String filename = outputDir + "/primes_" + start + "_" + end + ".txt";
        Future<?> future = asyncSaveToFile(primes, filename);
        future.get();  // Wait for the file to finish saving
        return "Process " + processId + " finished (" + primes.size() + " primes found)";
    }

    // --- Main runner function ---
    public static void runParallelPrimeFinder(int N, int numProcesses, String outputDir)
            throws InterruptedException, ExecutionException {
        new File(outputDir).mkdirs();
        int chunkSize = N / numProcesses;
        ExecutorService executor = Executors.newFixedThreadPool(numProcesses);
        List<Future<String>> futures = new ArrayList<>();

        System.out.println("Finding prime numbers from 1 to " + N +
                " using " + numProcesses + " processes...\n");

        long startTime = System.currentTimeMillis();

        for (int i = 0; i < numProcesses; i++) {
            final int start = i * chunkSize;
            final int end = (i < numProcesses - 1) ? (i + 1) * chunkSize : N;
            final int processId = i;
            futures.add(executor.submit(() ->
                    processRangeAndSave(start, end, outputDir, processId)));
        }

        for (Future<String> future : futures) {
            System.out.println(future.get());
        }

        executor.shutdown();

        long endTime = System.currentTimeMillis();
        System.out.println("\nCompleted in " + (endTime - startTime) / 1000.0 + " seconds");
    }

    public static void main(String[] args) throws InterruptedException, ExecutionException {
        int numProcesses = Runtime.getRuntime().availableProcessors();
        runParallelPrimeFinder(5_000_000, numProcesses, "prime_output");
    }
}