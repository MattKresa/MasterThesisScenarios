import math
import os
import threading
from concurrent.futures import ProcessPoolExecutor
from time import time


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
    for i in range(start, end):
        if is_prime(i):
            primes.append(i)
    return primes


# --- I/O-BOUND: save results to a file using a separate thread ---
def async_save_to_file(primes: list[int], filename: str) -> threading.Thread:
    def writer_task():
        print(f"Writing {len(primes)} primes to {filename}")
        try:
            with open(filename, "w") as f:
                for p in primes:
                    f.write(f"{p}\n")
        except IOError as e:
            print(f"Error writing file {filename}: {e}")
        print(f"Done writing {filename}")

    thread = threading.Thread(target=writer_task)
    thread.start()
    return thread


# --- CPU + I/O: function executed in each process ---
def process_range_and_save(start: int, end: int, output_dir: str, process_id: int) -> str:
    print(f"Process {process_id} working on range {start}-{end}")
    primes = find_primes_in_range(start, end)
    filename = os.path.join(output_dir, f"primes_{start}_{end}.txt")
    io_thread = async_save_to_file(primes, filename)
    io_thread.join()  # Wait for file saving to finish
    return f"Process {process_id} finished ({len(primes)} primes found)"


# --- Main runner function ---
def run_parallel_prime_finder(N: int, num_processes: int, output_dir: str):
    os.makedirs(output_dir, exist_ok=True)
    chunk_size = N // num_processes

    ranges = []
    for i in range(num_processes):
        start = i * chunk_size
        end = (i + 1) * chunk_size if i < num_processes - 1 else N
        ranges.append((start, end, i))

    print(f"Finding prime numbers from 1 to {N} using {num_processes} processes...\n")

    start_time = time()

    with ProcessPoolExecutor(max_workers=num_processes) as pool:
        futures = [pool.submit(process_range_and_save, start, end, output_dir, pid)
                   for start, end, pid in ranges]

        for future in futures:
            try:
                print(future.result())
            except Exception as e:
                print(f"Error: {e}")

    duration = time() - start_time
    print(f"\nCompleted in {duration:.2f} seconds")


if __name__ == "__main__":
    import multiprocessing
    num_cores = multiprocessing.cpu_count()
    run_parallel_prime_finder(5_000_000, num_cores, "prime_output")
