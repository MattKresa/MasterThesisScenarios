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
import java.util.stream.Collectors;

// This class represents the arguments for a single processing task.
// Using a record simplifies the creation of an immutable data carrier.
record TaskArgs(long start, long end, Path outputDir, int taskId) {}

public class PrimeFinder {

    // --- CPU-BOUND: check if number is prime ---
    // This method checks if a given number `n` is prime.
    // It's a direct translation of the Python logic, using the same optimizations.
    public static boolean isPrime(long n) {
        if (n < 2) {
            return false;
        }
        if (n == 2) {
            return true;
        }
        if (n % 2 == 0) {
            return false;
        }
        long sqrtN = (long) Math.sqrt(n) + 1;
        for (long i = 3; i < sqrtN; i += 2) {
            if (n % i == 0) {
                return false;
            }
        }
        return true;
    }

    // --- CPU-BOUND: find all primes in a given range ---
    // This method finds all prime numbers within a specified range [start, end).
    // It returns the list of primes as a `List<Long>`.
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
    // This method saves a list of primes to a file asynchronously using a `CompletableFuture`.
    // It returns the `CompletableFuture` so the caller can wait for it to complete.
    public static CompletableFuture<Void> asyncSaveToFile(List<Long> primes, Path filename) {
        return CompletableFuture.runAsync(() -> {
            System.out.printf("Writing %d primes to %s%n", primes.size(), filename);
            try (BufferedWriter writer = new BufferedWriter(new FileWriter(filename.toFile()))) {
                for (Long p : primes) {
                    writer.write(p.toString());
                    writer.newLine();
                }
                System.out.printf("Done writing %s%n", filename);
            } catch (IOException e) {
                System.err.printf("Error writing to file %s: %s%n", filename, e.getMessage());
            }
        });
    }

    // --- CPU + I/O: function executed in each process ---
    // This is the worker task that finds primes in its assigned range and saves them to a file.
    // It implements `Callable` so its result can be returned via a `Future`.
    static class ProcessTask implements Callable<String> {
        private final TaskArgs args;

        public ProcessTask(TaskArgs args) {
            this.args = args;
        }

        @Override
        public String call() throws Exception {
            System.out.printf("Task %d working on range %d-%d%n", args.taskId(), args.start(), args.end());

            List<Long> primes = findPrimesInRange(args.start(), args.end());

            String filename = String.format("primes_%d_%d.txt", args.start(), args.end());
            Path filepath = args.outputDir().resolve(filename);

            CompletableFuture<Void> saveFuture = asyncSaveToFile(primes, filepath);

            // Wait for the asynchronous save to complete before returning.
            saveFuture.join();

            return String.format("Task %d finished (%d primes found)", args.taskId(), primes.size());
        }
    }

    // --- Main runner function ---
    // This method orchestrates the entire parallel prime finding process.
    public static void runParallelPrimeFinder(long N, int numTasks, String outputDirStr) {
        Path outputDir = Paths.get(outputDirStr);
        try {
            Files.createDirectories(outputDir);
        } catch (IOException e) {
            System.err.printf("Could not create directory %s: %s%n", outputDir, e.getMessage());
            return;
        }

        ExecutorService executor = Executors.newFixedThreadPool(numTasks);
        long chunkSize = N / numTasks;

        List<TaskArgs> ranges = new ArrayList<>();
        for (int i = 0; i < numTasks; ++i) {
            long start = (long) i * chunkSize;
            long end = (i < numTasks - 1) ? (i + 1) * chunkSize : N;
            ranges.add(new TaskArgs(start, end, outputDir, i));
        }

        System.out.printf("Finding prime numbers from 1 to %d using %d tasks...%n%n", N, numTasks);
        long startTime = System.nanoTime();

        List<Future<String>> futures;
        try {
            // Submit all tasks and get a list of Future objects.
            futures = ranges.stream()
                    .map(ProcessTask::new)
                    .map(executor::submit)
                    .collect(Collectors.toList());

            // Wait for all futures to complete and print their results.
            for (Future<String> future : futures) {
                System.out.println(future.get());
            }

        } catch (InterruptedException | ExecutionException e) {
            e.printStackTrace();
        } finally {
            executor.shutdown();
        }

        long endTime = System.nanoTime();
        double elapsedTimeSeconds = (endTime - startTime) / 1_000_000_000.0;
        System.out.printf("%nCompleted in %.2f seconds%n", elapsedTimeSeconds);
    }

    public static void main(String[] args) {
        long N = 5_000_000;
        int numProcesses = Runtime.getRuntime().availableProcessors();
        String outputDir = "prime_output_java";

        runParallelPrimeFinder(N, numProcesses, outputDir);
    }
}
