"""Measure OCTproZ socket-stream send->recv latency.

Reports mean and median of the time between the broadcaster emitting a
frame and Python finishing recv of the frame's last byte.

Requires, in the Socket Stream Extension settings:
  * 'Include header to data transfer'             enabled
  * 'Append send timestamp (for latency ...)'     enabled

Usage:
    python measure_delay.py                      # TCP on 127.0.0.1:1234
    python measure_delay.py --pipe <pipe_name>   # IPC (Windows named pipe /
                                                 # Unix domain socket)
"""

import socket, struct, sys, time, statistics

HOST, PORT   = '127.0.0.1', 1234
HEADER_FMT   = '>I I H H B Q'   # magic, size, w, h, bitDepth, send_ms
HEADER_SIZE  = struct.calcsize(HEADER_FMT)
MAGIC        = 299792458
N_FRAMES     = 1000

# Transport selection.
if '--pipe' in sys.argv:
    idx = sys.argv.index('--pipe')
    if idx + 1 >= len(sys.argv):
        raise SystemExit("usage: --pipe <pipe_name>")
    pipe_name = sys.argv[idx + 1]
    pipe_path = rf'\\.\pipe\{pipe_name}' if sys.platform == 'win32' else pipe_name
    transport = open(pipe_path, 'rb', buffering=0)
    def recv_chunk(n):
        return transport.read(n) or b''
    label = f"pipe {pipe_path}"
else:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((HOST, PORT))
    transport = sock
    def recv_chunk(n):
        return sock.recv(n)
    label = f"{HOST}:{PORT}"

print(f"connected to {label}")

# Offset so perf_counter can be compared to QDateTime::currentMSecsSinceEpoch.
wall_minus_perf_ms = time.time() * 1000.0 - time.perf_counter() * 1000.0

buf = bytearray()

def recv_exact(n):
    while len(buf) < n:
        chunk = recv_chunk(65536)
        if not chunk:
            raise RuntimeError("connection closed")
        buf.extend(chunk)
    out = bytes(buf[:n])
    del buf[:n]
    return out

print(f"measuring {N_FRAMES} frames...")
lags = []
for i in range(N_FRAMES):
    header = recv_exact(HEADER_SIZE)
    magic, size, w, h, bd, send_ms = struct.unpack(HEADER_FMT, header)
    if magic != MAGIC:
        raise RuntimeError(
            f"magic mismatch (got {magic}). Enable both 'Include header' "
            "and 'Append send timestamp' in the extension settings.")
    if i == 0:
        print(f"  frame payload: {size} B ({size/1048576:.2f} MB), {w}x{h}, {bd}-bit")
    recv_exact(size)
    recv_ms = time.perf_counter() * 1000.0 + wall_minus_perf_ms
    lags.append(recv_ms - float(send_ms))

print()
print(f"send->recv over {N_FRAMES} frames:")
print(f"  mean   = {statistics.fmean(lags):7.2f} ms")
print(f"  median = {statistics.median(lags):7.2f} ms")

transport.close()
