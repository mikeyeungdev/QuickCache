#!/usr/bin/env python3
import argparse
import random
import socket
import statistics
import threading
import time
from dataclasses import dataclass


@dataclass
class BenchmarkResult:
    latencies_ms: list
    successful_requests: int
    errors: int


def command_for(workload, request_id):
    user_id = request_id % 100_000

    if workload == "verification_codes":
        if request_id % 2 == 0:
            code = 100_000 + (request_id % 900_000)
            return f"SET code:user{user_id} {code} EX 300"
        return f"GET code:user{user_id}"

    if workload == "ticket_queue":
        if request_id % 4 == 0:
            position = 1 + (request_id % 50_000)
            return f"SET queue:user{user_id} {position} EX 1800"
        if request_id % 4 == 1:
            position = 1 + random.randint(0, 50_000)
            return f"SET queue:user{user_id} {position} EX 1800"
        return f"GET queue:user{user_id}"

    if workload == "inventory_holds":
        item_id = request_id % 10_000
        key = f"hold:item{item_id}:user{user_id}"
        operation = request_id % 3
        if operation == 0:
            return f"SET {key} reserved EX 600"
        if operation == 1:
            return f"GET {key}"
        return f"DELETE {key}"

    raise ValueError(f"unknown workload: {workload}")


def is_success(response):
    return (
        response == "OK"
        or response == "PONG"
        or response == "NOT_FOUND"
        or response.startswith("VALUE ")
        or response.startswith("TTL ")
    )


def run_worker(host, port, workload, start_id, count):
    latencies = []
    successful = 0
    errors = 0

    try:
        with socket.create_connection((host, port), timeout=10) as sock:
            sock_file = sock.makefile("rw", encoding="utf-8", newline="\n")

            for offset in range(count):
                command = command_for(workload, start_id + offset)
                started = time.perf_counter()
                sock_file.write(command + "\n")
                sock_file.flush()
                response = sock_file.readline().strip()
                elapsed_ms = (time.perf_counter() - started) * 1000

                latencies.append(elapsed_ms)
                if is_success(response):
                    successful += 1
                else:
                    errors += 1
    except Exception:
        errors += count

    return BenchmarkResult(latencies, successful, errors)


def percentile(sorted_values, percentile_value):
    if not sorted_values:
        return 0.0

    index = int((percentile_value / 100) * (len(sorted_values) - 1))
    return sorted_values[index]


def main():
    parser = argparse.ArgumentParser(description="QuickCache TCP benchmark client")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=6379)
    parser.add_argument("--clients", type=int, default=50)
    parser.add_argument("--requests", type=int, default=10_000)
    parser.add_argument(
        "--workload",
        choices=["verification_codes", "ticket_queue", "inventory_holds"],
        default="verification_codes",
    )
    args = parser.parse_args()

    if args.clients <= 0 or args.requests <= 0:
        raise SystemExit("--clients and --requests must be positive")

    requests_per_client = args.requests // args.clients
    remainder = args.requests % args.clients
    results = []
    threads = []
    lock = threading.Lock()
    next_request_id = 0
    started = time.perf_counter()

    def thread_main(client_index, request_start, request_count):
        result = run_worker(args.host, args.port, args.workload, request_start, request_count)
        with lock:
            results.append(result)

    for client_index in range(args.clients):
        request_count = requests_per_client + (1 if client_index < remainder else 0)
        request_start = next_request_id
        next_request_id += request_count
        thread = threading.Thread(target=thread_main, args=(client_index, request_start, request_count))
        threads.append(thread)
        thread.start()

    for thread in threads:
        thread.join()

    elapsed_seconds = time.perf_counter() - started
    latencies = [latency for result in results for latency in result.latencies_ms]
    latencies.sort()

    successful_requests = sum(result.successful_requests for result in results)
    errors = sum(result.errors for result in results)
    rps = args.requests / elapsed_seconds if elapsed_seconds > 0 else 0.0

    p50 = percentile(latencies, 50)
    p95 = percentile(latencies, 95)
    p99 = percentile(latencies, 99)
    average = statistics.mean(latencies) if latencies else 0.0

    print("QuickCache Benchmark Results")
    print("============================")
    print(f"workload: {args.workload}")
    print(f"host: {args.host}")
    print(f"port: {args.port}")
    print(f"clients: {args.clients}")
    print(f"total_requests: {args.requests}")
    print(f"successful_requests: {successful_requests}")
    print(f"errors: {errors}")
    print(f"elapsed_seconds: {elapsed_seconds:.3f}")
    print(f"requests_per_second: {rps:.2f}")
    print(f"latency_avg_ms: {average:.3f}")
    print(f"latency_p50_ms: {p50:.3f}")
    print(f"latency_p95_ms: {p95:.3f}")
    print(f"latency_p99_ms: {p99:.3f}")


if __name__ == "__main__":
    main()
