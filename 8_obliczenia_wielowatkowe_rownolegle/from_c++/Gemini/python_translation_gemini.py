import math
import os
import threading
from concurrent.futures import ProcessPoolExecutor, ThreadPoolExecutor
import time
from typing import List

# --- CPU-BOUND: check if number is prime ---
def is_prime(n: int) -> bool:
    """
    Checks if a given number is prime.
    """
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
    """
    Finds all prime numbers within a specified range.
    """
    primes = []
    for n in range(start, end):
        if is_prime(n):
            primes.append(n)
    return primes

# --- I/O-BOUND: save results to a file using a separate thread ---
def save_to_file(primes: List[int], filename: str):
    """
    Saves a list of primes to a file.
    """
    print(f"Writing {len(primes)} primes to {filename}")
    with open(filename, 'w') as file:
        for p in primes:
            file.write(f"{p}\n")
    print(f"Done writing {filename}")

# --- CPU + I/O: function executed in each "process" (thread) ---
def process_range_and_save(start: int, end: int, output_dir: str, process_id: int) -> str:
    """
    Finds primes in a range and saves them to a file. This function is designed to be run
    in a separate process.
    """
    print(f"Process {process_id} working on range {start}-{end}")
    primes = find_primes_in_range(start, end)
    filename = os.path.join(output_dir, f"primes_{start}_{end}.txt")
    
    # Use a separate thread for I/O-bound task to avoid blocking the main process
    # while waiting for file operations.
    writer_thread = threading.Thread(target=save_to_file, args=(primes, filename))
    writer_thread.start()
    writer_thread.join()  # Wait for the file to finish saving
    
    return f"Process {process_id} finished ({len(primes)} primes found)"

# --- Main runner function ---
def run_parallel_prime_finder(N: int, num_processes: int, output_dir: str):
    """
    Main function to orchestrate the parallel prime finding process.
    """
    os.makedirs(output_dir, exist_ok=True)
    chunk_size = N // num_processes

    print(f"Finding prime numbers from 1 to {N} using {num_processes} processes...\n")
    
    start_time = time.time()
    
    # Use ProcessPoolExecutor to run CPU-bound tasks in parallel processes
    # to bypass the Global Interpreter Lock (GIL).
    with ProcessPoolExecutor(max_workers=num_processes) as executor:
        futures = []
        for i in range(num_processes):
            range_start = i * chunk_size
            range_end = N if i == num_processes - 1 else (i + 1) * chunk_size
            futures.append(executor.submit(process_range_and_save, range_start, range_end, output_dir, i))

        for future in futures:
            print(future.result())

    end_time = time.time()
    seconds = end_time - start_time
    print(f"\nCompleted in {seconds:.2f} seconds")

if __name__ == "__main__":
    # Get the number of CPU cores available
    num_cores = os.cpu_count()
    if num_cores is None or num_cores == 0:
        num_cores = 4  # Fallback value
        
    # Run the prime finder with a large number and the available cores
    run_parallel_prime_finder(5_000_000, num_cores, "prime_output")