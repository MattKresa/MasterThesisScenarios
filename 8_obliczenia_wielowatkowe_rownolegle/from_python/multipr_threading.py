import multiprocessing
import threading
import os
import math
import time

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
    return [n for n in range(start, end) if is_prime(n)]

# --- I/O-BOUND: save results to a file using a separate thread ---
def async_save_to_file(primes, filename):
    def write():
        print(f"Writing {len(primes)} primes to {filename}")
        with open(filename, 'w') as f:
            for p in primes:
                f.write(f"{p}\n")
        print(f"Done writing {filename}")
    thread = threading.Thread(target=write)
    thread.start()
    return thread

# --- CPU + I/O: function executed in each process ---
def process_range_and_save(args):
    start, end, output_dir, process_id = args
    print(f"Process {process_id} working on range {start}-{end}")
    primes = find_primes_in_range(start, end)
    filename = os.path.join(output_dir, f"primes_{start}_{end}.txt")
    thread = async_save_to_file(primes, filename)
    thread.join()  # Wait for the file to finish saving
    return f"Process {process_id} finished ({len(primes)} primes found)"

# --- Main runner function ---
def run_parallel_prime_finder(N=1_000_000, num_processes=4, output_dir="prime_output"):
    os.makedirs(output_dir, exist_ok=True)
    chunk_size = N // num_processes
    ranges = [
        (i * chunk_size, (i + 1) * chunk_size if i < num_processes - 1 else N, output_dir, i)
        for i in range(num_processes)
    ]

    print(f"Finding prime numbers from 1 to {N} using {num_processes} processes...\n")

    start_time = time.time()

    with multiprocessing.Pool(processes=num_processes) as pool:
        results = pool.map(process_range_and_save, ranges)

    for res in results:
        print(res)

    print(f"\nCompleted in {time.time() - start_time:.2f} seconds")

if __name__ == "__main__":
    run_parallel_prime_finder(N=5_000_000, num_processes=multiprocessing.cpu_count())
