import math
import os
import threading
import concurrent.futures
import time
from typing import List, Tuple

def is_prime(n: int) -> bool:
    """Check if a number is prime (CPU-bound)."""
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

def find_primes_in_range(start: int, end: int) -> List[int]:
    """Find all primes in a given range (CPU-bound)."""
    primes = []
    for i in range(start, end):
        if is_prime(i):
            primes.append(i)
    return primes

def async_save_to_file(primes: List[int], filename: str) -> threading.Thread:
    """Save results to a file in a separate thread (I/O-bound)."""
    def save_task():
        print(f"Writing {len(primes)} primes to {filename}")
        with open(filename, 'w') as f:
            for p in primes:
                f.write(f"{p}\n")
        print(f"Done writing {filename}")
    
    thread = threading.Thread(target=save_task)
    thread.start()
    return thread

def process_range_and_save(start: int, end: int, output_dir: str, process_id: int) -> str:
    """Process a range and save results (CPU + I/O)."""
    print(f"Process {process_id} working on range {start}-{end}")
    primes = find_primes_in_range(start, end)
    filename = os.path.join(output_dir, f"primes_{start}_{end}.txt")
    io_thread = async_save_to_file(primes, filename)
    io_thread.join()  # Wait for the file to finish saving
    return f"Process {process_id} finished ({len(primes)} primes found)"

def run_parallel_prime_finder(N: int, num_processes: int, output_dir: str) -> None:
    """Main runner function."""
    os.makedirs(output_dir, exist_ok=True)
    chunk_size = N // num_processes

    ranges = []
    for i in range(num_processes):
        start = i * chunk_size
        end = (i + 1) * chunk_size if i < num_processes - 1 else N
        ranges.append((start, end, i))

    print(f"Finding prime numbers from 1 to {N} using {num_processes} processes...\n")

    start_time = time.time()

    with concurrent.futures.ProcessPoolExecutor(max_workers=num_processes) as executor:
        futures = []
        for start, end, process_id in ranges:
            futures.append(executor.submit(process_range_and_save, start, end, output_dir, process_id))
        
        for future in concurrent.futures.as_completed(futures):
            try:
                print(future.result())
            except Exception as e:
                print(f"Error: {e}")

    duration = time.time() - start_time
    print(f"\nCompleted in {duration:.2f} seconds")

if __name__ == "__main__":
    num_cores = os.cpu_count()
    run_parallel_prime_finder(5_000_000, num_cores, "prime_output")