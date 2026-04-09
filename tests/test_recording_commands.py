"""
Test script for recording remote commands in OCTproZ Socket Stream Extension.

Tests: set_rec_path, set_rec_name, set_buffers_to_record, remote_record with params,
       set_rec_options, set_preallocation.

Usage:
    python test_recording_commands.py [--host HOST] [--port PORT] [--rec-path PATH]

Defaults: host=127.0.0.1, port=1234, rec-path=<system temp dir>
Requires: OCTproZ running with Virtual OCT System active and Socket Stream Extension connected.
"""

import socket
import time
import argparse
import tempfile
import os


def send_command(sock, command, expect_response=False, timeout=2.0):
    """Send a command and optionally wait for a response."""
    sock.sendall((command + '\n').encode('utf-8'))
    print(f"  Sent: {command}")
    if expect_response:
        sock.settimeout(timeout)
        try:
            response = sock.recv(4096).decode('utf-8').strip()
            print(f"  Response: {response}")
            return response
        except socket.timeout:
            print("  No response (timeout)")
            return None
        finally:
            sock.settimeout(None)
    time.sleep(0.1)
    return None


def ping(sock):
    """Verify connection is alive."""
    response = send_command(sock, "ping", expect_response=True)
    assert response == "pong", f"Expected 'pong', got '{response}'"
    print("  Connection OK")


def main():
    parser = argparse.ArgumentParser(description="Test recording commands for OCTproZ")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=1234)
    parser.add_argument("--rec-path", default=None, help="Recording path to use for tests (must exist)")
    args = parser.parse_args()

    rec_path = args.rec_path or tempfile.gettempdir()
    rec_path = rec_path.replace("\\", "/")

    print(f"Using recording path: {rec_path}")
    assert os.path.isdir(rec_path), f"Recording path does not exist: {rec_path}"

    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            sock.connect((args.host, args.port))
            print(f"Connected to {args.host}:{args.port}\n")

            send_command(sock, "enable_command_only_mode", expect_response=True)

            # Test 1: set_rec_path
            print("Test 1: set_rec_path")
            send_command(sock, f"set_rec_path:{rec_path}")
            ping(sock)

            # Test 2: set_rec_name
            print("\nTest 2: set_rec_name")
            send_command(sock, "set_rec_name:test_recording")
            ping(sock)

            # Test 3: set_buffers_to_record
            print("\nTest 3: set_buffers_to_record")
            send_command(sock, "set_buffers_to_record:2")
            ping(sock)

            # Test 4: remote_record (no params, uses settings from tests 1-3)
            print("\nTest 4: remote_record (no params)")
            send_command(sock, "remote_record")
            ping(sock)
            time.sleep(3)

            # Test 5: remote_record with path only
            print("\nTest 5: remote_record:<path>")
            send_command(sock, f"remote_record:{rec_path}")
            ping(sock)
            time.sleep(3)

            # Test 6: remote_record with path and name
            print("\nTest 6: remote_record:<path>|<name>")
            send_command(sock, f"remote_record:{rec_path}|param_test")
            ping(sock)
            time.sleep(3)

            # Test 7: set_rec_path with invalid path
            print("\nTest 7: set_rec_path with invalid path (expect error)")
            send_command(sock, "set_rec_path:/nonexistent/path/xyz")
            ping(sock)

            # Test 8: set_buffers_to_record with invalid value
            print("\nTest 8: set_buffers_to_record with invalid value (expect error)")
            send_command(sock, "set_buffers_to_record:0")
            ping(sock)

            # Test 9: set_buffers_to_record with non-numeric value
            print("\nTest 9: set_buffers_to_record with non-numeric value (expect error)")
            send_command(sock, "set_buffers_to_record:abc")
            ping(sock)

            # Test 10: set_rec_options (multiple flags)
            print("\nTest 10: set_rec_options (raw=1, processed=1, meta=0, stop_after=1)")
            send_command(sock, "set_rec_options:raw=1:processed=1:meta=0:stop_after=1")
            ping(sock)

            # Test 11: set_rec_options (single flag)
            print("\nTest 11: set_rec_options (screenshot=0)")
            send_command(sock, "set_rec_options:screenshot=0")
            ping(sock)

            # Test 12: set_rec_options with invalid format (expect error)
            print("\nTest 12: set_rec_options with invalid format (expect error)")
            send_command(sock, "set_rec_options:invalidpair")
            ping(sock)

            # Test 13: set_preallocation enable
            print("\nTest 13: set_preallocation:1 (enable)")
            send_command(sock, "set_preallocation:1")
            ping(sock)
            time.sleep(1)

            # Test 14: set_preallocation disable
            print("\nTest 14: set_preallocation:0 (disable)")
            send_command(sock, "set_preallocation:0")
            ping(sock)

            print("\nAll tests completed. Check OCTproZ log for info/error messages.")

    except ConnectionRefusedError:
        print(f"Could not connect to {args.host}:{args.port}. Is OCTproZ running with Socket Stream Extension?")
    except Exception as e:
        print(f"Error: {e}")


if __name__ == "__main__":
    main()
