import os
import time
import threading
from concurrent.futures import ProcessPoolExecutor, ThreadPoolExecutor, as_completed
from typing import List
import math


def is_prime(n: int) -> bool:
    """CPU-BOUND: check if number is prime"""
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
    """CPU-BOUND: find all primes in a given range"""
    primes = []
    for i in range(start, end):
        if is_prime(i):
            primes.append(i)
    return primes


def async_save_to_file(primes: List[int], filename: str) -> threading.Thread:
    """I/O-BOUND: save results to a file using a separate thread"""
    def save_worker():
        print(f"Writing {len(primes)} primes to {filename}")
        try:
            with open(filename, 'w') as writer:
                for p in primes:
                    writer.write(f"{p}\n")
        except IOError as e:
            print(f"Error writing to file: {e}")
        print(f"Done writing {filename}")
    
    thread = threading.Thread(target=save_worker)
    thread.start()
    return thread


def process_range_and_save(args: tuple) -> str:
    """CPU + I/O: function executed in each process"""
    start, end, output_dir, process_id = args
    
    print(f"Process {process_id} working on range {start}-{end}")
    primes = find_primes_in_range(start, end)
    filename = os.path.join(output_dir, f"primes_{start}_{end}.txt")
    
    io_thread = async_save_to_file(primes, filename)
    io_thread.join()  # Wait for the file to finish saving
    
    return f"Process {process_id} finished ({len(primes)} primes found)"


def run_parallel_prime_finder(N: int, num_processes: int, output_dir: str):
    """Main runner function"""
    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    chunk_size = N // num_processes
    
    # Create ranges for each process
    ranges = []
    for i in range(num_processes):
        start = i * chunk_size
        end = (i + 1) * chunk_size if i < num_processes - 1 else N
        ranges.append((start, end, output_dir, i))
    
    print(f"Finding prime numbers from 1 to {N} using {num_processes} processes...\n")
    
    start_time = time.time()
    
    # Use ProcessPoolExecutor for CPU-bound tasks (similar to ForkJoinPool)
    with ProcessPoolExecutor(max_workers=num_processes) as executor:
        # Submit all tasks
        futures = [executor.submit(process_range_and_save, range_args) for range_args in ranges]
        
        # Collect results as they complete
        for future in as_completed(futures):
            try:
                result = future.result()
                print(result)
            except Exception as e:
                print(f"Error in process: {e}")
    
    duration = time.time() - start_time
    print(f"\nCompleted in {duration:.2f} seconds")


def main():
    """Main entry point"""
    num_cores = os.cpu_count()
    run_parallel_prime_finder(5_000_000, num_cores, "prime_output")


if __name__ == "__main__":
    main()