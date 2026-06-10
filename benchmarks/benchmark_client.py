#!/usr/bin/env python3
import argparse
import concurrent.futures
import socket
import statistics
import time
from pathlib import Path


def send_command(host, port, command):
    start = time.perf_counter()
    with socket.create_connection((host, port), timeout=5) as sock:
        sock.sendall((command.rstrip() + "\n").encode("utf-8"))
        response = sock.recv(4096)
    elapsed_ms = (time.perf_counter() - start) * 1000
    return elapsed_ms, response.decode("utf-8", errors="replace").strip()


def load_workload(path):
    commands = []
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            commands.append(line)
    if not commands:
        raise ValueError("workload is empty")
    return commands


def percentile(values, pct):
    if not values:
        return 0.0
    ordered = sorted(values)
    index = int(round((pct / 100) * (len(ordered) - 1)))
    return ordered[index]


def main():
    parser = argparse.ArgumentParser(description="QuickCache benchmark client")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=6379)
    parser.add_argument("--clients", type=int, default=32)
    parser.add_argument("--requests", type=int, default=10000)
    parser.add_argument("--workload", default="benchmarks/workloads/verification_codes.txt")
    args = parser.parse_args()

    commands = load_workload(args.workload)
    latencies = []
    errors = 0
    start = time.perf_counter()

    def run_one(i):
        command = commands[i % len(commands)]
        return send_command(args.host, args.port, command)

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.clients) as executor:
        futures = [executor.submit(run_one, i) for i in range(args.requests)]
        for future in concurrent.futures.as_completed(futures):
            try:
                latency, response = future.result()
                latencies.append(latency)
                if response.startswith("ERR"):
                    errors += 1
            except Exception:
                errors += 1

    elapsed = time.perf_counter() - start
    rps = args.requests / elapsed if elapsed else 0

    _, stats = send_command(args.host, args.port, "STATS")
    print(f"requests={args.requests}")
    print(f"clients={args.clients}")
    print(f"elapsed_seconds={elapsed:.3f}")
    print(f"requests_per_second={rps:.2f}")
    print(f"latency_p50_ms={statistics.median(latencies):.3f}" if latencies else "latency_p50_ms=0.000")
    print(f"latency_p95_ms={percentile(latencies, 95):.3f}")
    print(f"latency_p99_ms={percentile(latencies, 99):.3f}")
    print(f"errors={errors}")
    print(f"server_stats={stats}")


if __name__ == "__main__":
    main()
