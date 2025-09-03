import math
import threading
import concurrent.futures
import os
import time
from typing import List

def is_prime(n: int) -> bool:
    """CPU-BOUND: check if number is prime"""
    if n < 2:
        return False
    if n == 2:
        return True
    if n % 2 == 0:
        return False
    
    sqrt_n = int(math.sqrt(n)) + 1
    for i in range(3, sqrt_n + 1, 2):
        if n % i == 0:
            return False
    return True

def find_primes_in_range(start: int, end: int) -> List[int]:
    """CPU-BOUND: find all primes in a given range"""
    primes = []
    # Reserve space estimate (Python lists grow dynamically)
    for n in range(start, end):
        if is_prime(n):
            primes.append(n)
    return primes

def async_save_to_file(primes: List[int], filename: str) -> threading.Thread:
    """I/O-BOUND: save results to a file using a separate thread"""
    def save_worker():
        print(f"Writing {len(primes)} primes to {filename}")
        with open(filename, 'w') as file:
            for p in primes:
                file.write(f"{p}\n")
        print(f"Done writing {filename}")
    
    thread = threading.Thread(target=save_worker)
    thread.start()
    return thread

def process_range_and_save(start: int, end: int, output_dir: str, process_id: int) -> str:
    """CPU + I/O: function executed in each "process" (thread)"""
    print(f"Process {process_id} working on range {start}-{end}")
    primes = find_primes_in_range(start, end)
    
    filename = f"{output_dir}/primes_{start}_{end}.txt"
    writer_thread = async_save_to_file(primes, filename)
    writer_thread.join()  # Wait for the file to finish saving
    
    return f"Process {process_id} finished ({len(primes)} primes found)"

def run_parallel_prime_finder(N: int, num_threads: int, output_dir: str):
    """Main runner function"""
    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    chunk_size = N // num_threads
    
    print(f"Finding prime numbers from 1 to {N} using {num_threads} threads...\n")
    
    start_time = time.time()
    
    # Use ThreadPoolExecutor for managing futures
    with concurrent.futures.ThreadPoolExecutor(max_workers=num_threads) as executor:
        futures = []
        
        for i in range(num_threads):
            range_start = i * chunk_size
            range_end = N if i == num_threads - 1 else (i + 1) * chunk_size
            
            future = executor.submit(process_range_and_save, 
                                   range_start, range_end, output_dir, i)
            futures.append(future)
        
        # Wait for all futures to complete and print results
        for future in concurrent.futures.as_completed(futures):
            print(future.result())
    
    end_time = time.time()
    seconds = end_time - start_time
    print(f"\nCompleted in {seconds:.6f} seconds")

def main():
    # Get number of CPU cores (equivalent to std::thread::hardware_concurrency())
    threads = os.cpu_count()
    if threads is None:
        threads = 4  # fallback
    
    run_parallel_prime_finder(5_000_000, threads, "prime_output")

if __name__ == "__main__":
    main()