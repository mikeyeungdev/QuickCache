#!/usr/bin/env python3
import argparse
import socket
import threading
import time


def send_command(sock_file, command):
    sock_file.write(command + "\n")
    sock_file.flush()
    return sock_file.readline().strip()


def worker(worker_id, host, port, operations, errors):
    try:
        with socket.create_connection((host, port), timeout=5) as sock:
            sock_file = sock.makefile("rw", encoding="utf-8", newline="\n")
            for index in range(operations):
                key = f"load:{worker_id}:{index}"
                expected = str(index)

                checks = [
                    (f"SET {key} {expected} EX 30", "OK"),
                    (f"GET {key}", f"VALUE {expected}"),
                    (f"TTL {key}", "TTL "),
                    (f"DELETE {key}", "OK"),
                    (f"GET {key}", "NOT_FOUND"),
                ]

                for command, expected_prefix in checks:
                    response = send_command(sock_file, command)
                    if not response.startswith(expected_prefix):
                        errors.append(f"{command} -> {response}, expected {expected_prefix}")
                        return
    except Exception as exc:
        errors.append(f"worker {worker_id}: {exc}")


def main():
    parser = argparse.ArgumentParser(description="QuickCache concurrent client smoke test")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=6379)
    parser.add_argument("--clients", type=int, default=8)
    parser.add_argument("--operations", type=int, default=100)
    args = parser.parse_args()

    errors = []
    threads = []
    start = time.perf_counter()

    for worker_id in range(args.clients):
        thread = threading.Thread(
            target=worker,
            args=(worker_id, args.host, args.port, args.operations, errors),
        )
        threads.append(thread)
        thread.start()

    for thread in threads:
        thread.join()

    elapsed = time.perf_counter() - start
    total_commands = args.clients * args.operations * 5

    if errors:
        print("FAILED")
        for error in errors[:10]:
            print(error)
        raise SystemExit(1)

    print("PASSED")
    print(f"clients={args.clients}")
    print(f"operations_per_client={args.operations}")
    print(f"commands={total_commands}")
    print(f"elapsed_seconds={elapsed:.3f}")
    print(f"commands_per_second={total_commands / elapsed:.2f}")


if __name__ == "__main__":
    main()
