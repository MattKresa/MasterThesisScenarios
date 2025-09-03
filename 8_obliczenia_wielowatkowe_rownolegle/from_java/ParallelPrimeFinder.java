// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/ParallelPrimeFinder.java) ---
package org.example;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
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
        List<Integer> primes = new ArrayList<>();
        for (int i = start; i < end; i++) {
            if (isPrime(i)) {
                primes.add(i);
            }
        }
        return primes;
    }

    // --- I/O-BOUND: save results to a file using a separate thread ---
    public static Thread asyncSaveToFile(List<Integer> primes, String filename) {
        Thread thread = new Thread(() -> {
            System.out.printf("Writing %d primes to %s%n", primes.size(), filename);
            try (BufferedWriter writer = new BufferedWriter(new FileWriter(filename))) {
                for (int p : primes) {
                    writer.write(Integer.toString(p));
                    writer.newLine();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
            System.out.printf("Done writing %s%n", filename);
        });
        thread.start();
        return thread;
    }

    // --- CPU + I/O: function executed in each process ---
    public static String processRangeAndSave(int start, int end, String outputDir, int processId) {
        System.out.printf("Process %d working on range %d-%d%n", processId, start, end);
        List<Integer> primes = findPrimesInRange(start, end);
        String filename = outputDir + File.separator + String.format("primes_%d_%d.txt", start, end);
        Thread ioThread = asyncSaveToFile(primes, filename);
        try {
            ioThread.join(); // Wait for the file to finish saving
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
        return String.format("Process %d finished (%d primes found)", processId, primes.size());
    }

    // --- Main runner function ---
    public static void runParallelPrimeFinder(int N, int numProcesses, String outputDir) throws InterruptedException {
        new File(outputDir).mkdirs();
        int chunkSize = N / numProcesses;

        List<int[]> ranges = new ArrayList<>();
        for (int i = 0; i < numProcesses; i++) {
            int start = i * chunkSize;
            int end = (i < numProcesses - 1) ? (i + 1) * chunkSize : N;
            ranges.add(new int[]{start, end, i});
        }

        System.out.printf("Finding prime numbers from 1 to %d using %d processes...%n%n", N, numProcesses);

        long startTime = System.currentTimeMillis();

        ForkJoinPool pool = new ForkJoinPool(numProcesses);
        List<Callable<String>> tasks = new ArrayList<>();
        for (int[] range : ranges) {
            int start = range[0];
            int end = range[1];
            int processId = range[2];
            tasks.add(() -> processRangeAndSave(start, end, outputDir, processId));
        }

        List<Future<String>> results = pool.invokeAll(tasks);

        for (Future<String> result : results) {
            try {
                System.out.println(result.get());
            } catch (ExecutionException e) {
                e.printStackTrace();
            }
        }

        pool.shutdown();
        pool.awaitTermination(1, TimeUnit.HOURS);

        long duration = System.currentTimeMillis() - startTime;
        System.out.printf("%nCompleted in %.2f seconds%n", duration / 1000.0);
    }

    public static void main(String[] args) throws InterruptedException {
        int numCores = Runtime.getRuntime().availableProcessors();
        runParallelPrimeFinder(5_000_000, numCores, "prime_output");
    }
}
