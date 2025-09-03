import math
import os
import time
from concurrent.futures import ProcessPoolExecutor
import threading


# --- CPU-BOUND: check if number is prime ---
def is_prime(n: int) -> bool:
    if n < 2:
        return False
    if n == 2:
        return True
    if n % 2 == 0:
        return False
    sqrt_n = int(math.sqrt(n)) + 1
    for i in range(3, sqrt_n, 2):
        if n % i == 0:
            return False
    return True


# --- CPU-BOUND: find all primes in a given range ---
def find_primes_in_range(start: int, end: int) -> list[int]:
    primes = []
    for n in range(start, end):
        if is_prime(n):
            primes.append(n)
    return primes


# --- I/O-BOUND: save results to a file using a separate thread ---
def async_save_to_file(primes: list[int], filename: str) -> threading.Thread:
    def writer():
        print(f"Writing {len(primes)} primes to {filename}")
        with open(filename, "w") as f:
            for p in primes:
                f.write(f"{p}\n")
        print(f"Done writing {filename}")

    t = threading.Thread(target=writer)
    t.start()
    return t


# --- CPU + I/O: function executed in each "process" (thread) ---
def process_range_and_save(start: int, end: int, output_dir: str, process_id: int) -> str:
    print(f"Process {process_id} working on range {start}-{end}")
    primes = find_primes_in_range(start, end)
    filename = os.path.join(output_dir, f"primes_{start}_{end}.txt")
    writer_thread = async_save_to_file(primes, filename)
    writer_thread.join()  # Wait for the file to finish saving
    return f"Process {process_id} finished ({len(primes)} primes found)"


# --- Main runner function ---
def run_parallel_prime_finder(N: int, num_threads: int, output_dir: str):
    os.makedirs(output_dir, exist_ok=True)
    chunk_size = N // num_threads

    print(f"Finding prime numbers from 1 to {N} using {num_threads} threads...\n")
    start_time = time.perf_counter()

    with ProcessPoolExecutor(max_workers=num_threads) as executor:
        futures = []
        for i in range(num_threads):
            range_start = i * chunk_size
            range_end = N if i == num_threads - 1 else (i + 1) * chunk_size
            futures.append(executor.submit(process_range_and_save,
                                           range_start, range_end, output_dir, i))

        for f in futures:
            print(f.result())

    end_time = time.perf_counter()
    print(f"\nCompleted in {end_time - start_time:.2f} seconds")


if __name__ == "__main__":
    threads = os.cpu_count() or 4
    run_parallel_prime_finder(5_000_000, threads, "prime_output")
