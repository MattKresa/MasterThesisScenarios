import math
import os
import threading
import concurrent.futures
import time
from typing import List

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
def find_primes_in_range(start: int, end: int) -> List[int]:
    primes = []
    for n in range(start, end):
        if is_prime(n):
            primes.append(n)
    return primes

# --- I/O-BOUND: save results to a file using a separate thread ---
def async_save_to_file(primes: List[int], filename: str) -> None:
    print(f"Writing {len(primes)} primes to {filename}")
    with open(filename, 'w') as file:
        for p in primes:
            file.write(f"{p}\n")
    print(f"Done writing {filename}")

# --- CPU + I/O: function executed in each "process" (thread) ---
def process_range_and_save(start: int, end: int, output_dir: str, process_id: int) -> str:
    print(f"Process {process_id} working on range {start}-{end}")
    primes = find_primes_in_range(start, end)
    filename = os.path.join(output_dir, f"primes_{start}_{end}.txt")
    
    # Create and start a thread for file writing
    writer = threading.Thread(target=async_save_to_file, args=(primes, filename))
    writer.start()
    writer.join()  # Wait for the file to finish saving
    
    return f"Process {process_id} finished ({len(primes)} primes found)"

# --- Main runner function ---
def run_parallel_prime_finder(N: int, num_threads: int, output_dir: str) -> None:
    os.makedirs(output_dir, exist_ok=True)
    chunk_size = N // num_threads

    print(f"Finding prime numbers from 1 to {N} using {num_threads} threads...\n")

    start_time = time.time()

    with concurrent.futures.ThreadPoolExecutor(max_workers=num_threads) as executor:
        futures = []
        for i in range(num_threads):
            range_start = i * chunk_size
            range_end = N if (i == num_threads - 1) else (i + 1) * chunk_size
            futures.append(
                executor.submit(
                    process_range_and_save,
                    range_start,
                    range_end,
                    output_dir,
                    i
                )
            )
        
        for future in concurrent.futures.as_completed(futures):
            print(future.result())

    end_time = time.time()
    seconds = end_time - start_time
    print(f"\nCompleted in {seconds:.2f} seconds")

if __name__ == "__main__":
    threads = os.cpu_count() or 4  # fallback to 4 if os.cpu_count() returns None
    run_parallel_prime_finder(5_000_000, threads, "prime_output")