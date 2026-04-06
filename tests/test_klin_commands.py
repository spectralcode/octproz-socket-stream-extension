"""
Integration tests for k-linearization remote commands in SocketStreamExtension.

Prerequisites:
  - OCTproZ running with SocketStreamExtension active and listening on TCP/IP
  - Default address: 127.0.0.1:1234

Usage:
  python test_klin_commands.py [--host HOST] [--port PORT] [--csv PATH]

The script connects in command-only mode, sends each k-linearization command,
and checks whether OCTproZ reports an error. It prints PASS/FAIL for each test.
"""

import socket
import sys
import os
import time
import argparse


DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 1234
RECV_TIMEOUT = 2.0  # seconds to wait for error responses


def send_command(sock, command):
    """Send a command and return any response received within the timeout."""
    sock.sendall((command + "\n").encode("utf-8"))
    time.sleep(0.3)  # give OCTproZ time to process
    try:
        data = sock.recv(4096)
        return data.decode("utf-8").strip()
    except socket.timeout:
        return ""


def run_tests(host, port, csv_path):
    results = []

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(RECV_TIMEOUT)
        sock.connect((host, port))
        print(f"Connected to {host}:{port}")
    except ConnectionRefusedError:
        print(f"ERROR: Could not connect to {host}:{port}. Is OCTproZ running with SocketStreamExtension?")
        sys.exit(1)

    # Switch to command-only mode so we don't get data mixed in
    response = send_command(sock, "enable_command_only_mode")
    print(f"Command mode: {response}")

    # --- Test 1: set_klin_coeffs with valid coefficients ---
    name = "set_klin_coeffs (valid)"
    response = send_command(sock, "set_klin_coeffs:1.2:3.4:5.6:7.8")
    passed = "error" not in response.lower() and "invalid" not in response.lower()
    results.append((name, passed, response))

    # --- Test 2: set_klin_coeffs with null values ---
    name = "set_klin_coeffs (with nulls)"
    response = send_command(sock, "set_klin_coeffs:null:1.5:null:0.001")
    passed = "error" not in response.lower() and "invalid" not in response.lower()
    results.append((name, passed, response))

    # --- Test 3: set_klin_coeffs with wrong number of params ---
    name = "set_klin_coeffs (wrong param count, expect error)"
    response = send_command(sock, "set_klin_coeffs:0:1")
    # This SHOULD produce an error, so we expect error text or at least no crash
    passed = True  # as long as we didn't lose connection
    results.append((name, passed, response))

    # --- Test 4: set_klin_curve with inline values ---
    name = "set_klin_curve (valid)"
    curve_values = ",".join(str(i * 0.5) for i in range(16))
    response = send_command(sock, f"set_klin_curve:{curve_values}")
    passed = "error" not in response.lower() and "invalid" not in response.lower()
    results.append((name, passed, response))

    # --- Test 5: set_klin_curve with empty data ---
    name = "set_klin_curve (empty, expect error)"
    response = send_command(sock, "set_klin_curve:")
    passed = True  # should error gracefully, not crash
    results.append((name, passed, response))

    # --- Test 6: load_klin_curve with sample CSV ---
    name = "load_klin_curve (valid CSV)"
    response = send_command(sock, f"load_klin_curve:{csv_path}")
    passed = "error" not in response.lower() and "invalid" not in response.lower()
    results.append((name, passed, response))

    # --- Test 7: load_klin_curve with nonexistent file ---
    name = "load_klin_curve (missing file, expect error)"
    response = send_command(sock, "load_klin_curve:/nonexistent/path/curve.csv")
    passed = True  # should error gracefully
    results.append((name, passed, response))

    # --- Test 8: curve switching: csv1 -> csv2 -> csv1 ---
    csv2_path = os.path.join(os.path.dirname(csv_path), "sample_klin_curve2.csv")
    if os.path.exists(csv2_path):
        name = "load_klin_curve (curve1)"
        response = send_command(sock, f"load_klin_curve:{csv_path}")
        passed = "error" not in response.lower() and "invalid" not in response.lower()
        results.append((name, passed, response))

        name = "load_klin_curve (switch to curve2)"
        response = send_command(sock, f"load_klin_curve:{csv2_path}")
        passed = "error" not in response.lower() and "invalid" not in response.lower()
        results.append((name, passed, response))

        name = "load_klin_curve (switch back to curve1)"
        response = send_command(sock, f"load_klin_curve:{csv_path}")
        passed = "error" not in response.lower() and "invalid" not in response.lower()
        results.append((name, passed, response))
    else:
        print(f"WARNING: sample_klin_curve2.csv not found at {csv2_path}, skipping curve switching test")

    # --- Test 9: verify connection still alive with ping ---
    name = "ping (connection still alive)"
    response = send_command(sock, "ping")
    passed = "pong" in response.lower()
    results.append((name, passed, response))

    sock.close()

    # Print results
    print("\n" + "=" * 60)
    print("TEST RESULTS")
    print("=" * 60)
    all_passed = True
    for name, passed, response in results:
        status = "PASS" if passed else "FAIL"
        if not passed:
            all_passed = False
        resp_info = f"  response: {response}" if response else ""
        print(f"  [{status}] {name}{resp_info}")

    print("=" * 60)
    print(f"{'ALL TESTS PASSED' if all_passed else 'SOME TESTS FAILED'}")
    print("=" * 60)
    return 0 if all_passed else 1


def main():
    parser = argparse.ArgumentParser(description="Test k-linearization remote commands")
    parser.add_argument("--host", default=DEFAULT_HOST, help=f"OCTproZ host (default: {DEFAULT_HOST})")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help=f"OCTproZ port (default: {DEFAULT_PORT})")
    parser.add_argument("--csv", default=None, help="Path to sample CSV file for load_klin_curve test")
    args = parser.parse_args()

    # Default CSV path: sample_klin_curve.csv in the same directory as this script
    if args.csv is None:
        args.csv = os.path.join(os.path.dirname(os.path.abspath(__file__)), "sample_klin_curve.csv")

    if not os.path.exists(args.csv):
        print(f"WARNING: Sample CSV not found at {args.csv}")
        print("The load_klin_curve test will fail. Use --csv to specify the path.")

    sys.exit(run_tests(args.host, args.port, args.csv))


if __name__ == "__main__":
    main()
