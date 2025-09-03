import math
import os
import threading
import time
from concurrent.futures import ProcessPoolExecutor, ThreadPoolExecutor

# --- CPU-BOUND: check if number is prime ---
def is_prime(n):
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
def find_primes_in_range(start, end):
    primes = []
    for i in range(start, end):
        if is_prime(i):
            primes.append(i)
    return primes

# --- I/O-BOUND: save results to a file using a separate thread ---
def async_save_to_file(primes, filename):
    def worker():
        print(f"Writing {len(primes)} primes to {filename}")
        try:
            with open(filename, 'w') as writer:
                for p in primes:
                    writer.write(f"{p}\n")
        except IOError as e:
            print(f"Error writing to file: {e}")
        print(f"Done writing {filename}")

    thread = threading.Thread(target=worker)
    thread.start()
    return thread

# --- CPU + I/O: function executed in each process ---
def process_range_and_save(start, end, output_dir, process_id):
    print(f"Process {process_id} working on range {start}-{end}")
    primes = find_primes_in_range(start, end)
    filename = os.path.join(output_dir, f"primes_{start}_{end}.txt")
    
    # Use a thread pool for I/O-bound tasks
    with ThreadPoolExecutor(max_workers=1) as executor:
        future = executor.submit(async_save_to_file, primes, filename)
        future.result().join()  # Wait for the file to finish saving

    return f"Process {process_id} finished ({len(primes)} primes found)"

# --- Main runner function ---
def run_parallel_prime_finder(n, num_processes, output_dir):
    os.makedirs(output_dir, exist_ok=True)
    chunk_size = n // num_processes

    ranges = []
    for i in range(num_processes):
        start = i * chunk_size
        end = (i + 1) * chunk_size if i < num_processes - 1 else n
        ranges.append((start, end, i))

    print(f"Finding prime numbers from 1 to {n} using {num_processes} processes...\n")
    start_time = time.time()

    with ProcessPoolExecutor(max_workers=num_processes) as executor:
        futures = [executor.submit(process_range_and_save, *range_params) for range_params in ranges]
        results = [future.result() for future in futures]
    
    for result in results:
        print(result)

    duration = time.time() - start_time
    print(f"\nCompleted in {duration:.2f} seconds")

if __name__ == "__main__":
    num_cores = os.cpu_count()
    run_parallel_prime_finder(5_000_000, num_cores, "prime_output")