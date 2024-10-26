import socket
import threading
import numpy as np
import cv2
import time
import struct
import sys
import math

# Define header format: big endian, uint32, uint32, uint16, uint16, uint8
HEADER_FORMAT = '>I I H H B'
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)

# Magic number as defined in the server
MAGIC_NUMBER = 299792458

def format_bytes(bytes_num):
    """Converts bytes to a human-readable format."""
    for unit in ['B/s', 'KB/s', 'MB/s', 'GB/s']:
        if bytes_num < 1024.0:
            return f"{bytes_num:.2f} {unit}"
        bytes_num /= 1024.0
    return f"{bytes_num:.2f} TB/s"

class OCTClient:
    def __init__(self, host='127.0.0.1', port=1234):
        self.host = host
        self.port = port
        self.latest_frame = None
        self.frame_lock = threading.Lock()
        self.bytes_received = 0
        self.bytes_lock = threading.Lock()
        self.frames_received = 0
        self.frames_received_lock = threading.Lock()
        self.running = True
        self.cmd_sock = None
        self.data_sock = None

    def connect(self):
        try:
            self.cmd_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.cmd_sock.settimeout(600.0)
            self.cmd_sock.connect((self.host, self.port))
            print(f"[Command Connection] Established to {self.host}:{self.port}")

            identification_message = 'enable_command_only_mode\n'
            self.cmd_sock.sendall(identification_message.encode('utf-8'))
            response = self.cmd_sock.recv(4096)
            if response:
                print(f"[Command Connection] Server response: {response.decode('utf-8').strip()}")
            else:
                print("[Command Connection] No response received for command mode activation.")

            self.data_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.data_sock.settimeout(5.0)
            self.data_sock.connect((self.host, self.port))
            print(f"[Data Connection] Established to {self.host}:{self.port}")

            data_thread = threading.Thread(target=self.receive_data, args=(self.data_sock,), daemon=True)
            data_thread.start()

            user_input_thread = threading.Thread(target=self.handle_user_input, daemon=True)
            user_input_thread.start()

            window_name = 'OCT Image'
            cv2.namedWindow(window_name, cv2.WINDOW_NORMAL | cv2.WINDOW_KEEPRATIO)
            cv2.resizeWindow(window_name, 800, 600)

            fps_displayed = 0.0
            fps_received = 0.0
            data_rate = 0.0
            display_interval = 5.0
            frame_display_count = 0
            frame_received_count = 0
            bytes_count = 0
            display_time = time.time()

            while self.running:
                try:
                    with self.frame_lock:
                        frame = self.latest_frame.copy() if self.latest_frame is not None else None

                    if frame is not None:
                        frame_display_count += 1
                        with self.frames_received_lock:
                            frame_received_count += self.frames_received
                            self.frames_received = 0

                        with self.bytes_lock:
                            current_bytes = self.bytes_received
                            bytes_count += current_bytes
                            self.bytes_received = 0

                        current_time = time.time()
                        elapsed_time = current_time - display_time
                        if elapsed_time >= display_interval:
                            fps_displayed = frame_display_count / elapsed_time
                            fps_received = frame_received_count / elapsed_time
                            data_rate = bytes_count / elapsed_time
                            frame_display_count = 0
                            frame_received_count = 0
                            bytes_count = 0
                            display_time = current_time

                        if isinstance(frame, np.ndarray) and frame.size > 0:
                            cv2.imshow(window_name, frame)
                            formatted_data_rate = format_bytes(data_rate)
                            cv2.setWindowTitle(
                                window_name, 
                                f'OCT Image - Data Rate: {formatted_data_rate} - FPS Received: {fps_received:.2f} - FPS Displayed: {fps_displayed:.2f}'
                            )
                        else:
                            print("[Main Thread] Received invalid frame data.")

                    if cv2.waitKey(1) & 0xFF == ord('q'):
                        print("[Main Thread] 'q' pressed. Exiting.")
                        self.running = False
                        break

                except KeyboardInterrupt:
                    print("\n[Main Thread] KeyboardInterrupt received. Exiting.")
                    self.running = False
                    break
                except Exception as e:
                    print(f"[Main Thread] An error occurred: {e}")
                    self.running = False
                    break

            cv2.destroyAllWindows()
            self.close_sockets()
            print("[Main Thread] Client has exited.")

        except Exception as e:
            print(f"[Connection] An error occurred: {e}")
            self.close_sockets()

    def receive_data(self, sock):
        buffer = bytearray()
        try:
            while self.running:
                try:
                    data = sock.recv(65536)
                    if not data:
                        print("[Data Connection] Server closed the data connection.")
                        self.running = False
                        break
                    buffer.extend(data)

                    with self.bytes_lock:
                        self.bytes_received += len(data)

                    while True:
                        if len(buffer) < HEADER_SIZE:
                            break

                        header = buffer[:HEADER_SIZE]
                        try:
                            startIdentifier, bufferSizeInBytes, frameWidth, frameHeight, bitDepth = struct.unpack(HEADER_FORMAT, header)
                        except struct.error as e:
                            print(f"[Data Connection] Header unpacking failed: {e}")
                            buffer = self.resync_buffer(buffer)
                            continue

                        if startIdentifier != MAGIC_NUMBER:
                            print("[Data Connection] Magic number mismatch. Resynchronizing...")
                            buffer = self.resync_buffer(buffer)
                            continue

                        single_frame_size = frameWidth * frameHeight * math.ceil(bitDepth / 8)

                        if len(buffer) < HEADER_SIZE + single_frame_size:
                            break

                        frame_start_index = HEADER_SIZE
                        frame_end_index = HEADER_SIZE + single_frame_size
                        image_data = buffer[frame_start_index:frame_end_index]
                        buffer = buffer[frame_end_index:]

                        if bitDepth <= 8:
                            dtype = np.uint8
                        elif bitDepth <= 16:
                            dtype = np.uint16
                        else:
                            print(f"[Data Connection] Unsupported bit depth: {bitDepth}")
                            continue

                        image_array = np.frombuffer(image_data, dtype=dtype)
                        expected_size = frameWidth * frameHeight
                        if image_array.size != expected_size:
                            print(f"[Data Connection] Unexpected image size: expected {expected_size}, got {image_array.size}")
                            continue

                        image_array = image_array.reshape((frameHeight, frameWidth))

                        if bitDepth > 8:
                            image_display = (image_array.astype(np.float32) / 65535 * 255).astype(np.uint8)
                        else:
                            image_display = image_array

                        with self.frame_lock:
                            self.latest_frame = image_display.copy()

                        with self.frames_received_lock:
                            self.frames_received += 1

                        break

                except socket.timeout:
                    continue
                except Exception as e:
                    print(f"[Data Connection] An error occurred while receiving data: {e}")
                    self.running = False
                    break

        except Exception as e:
            print(f"[Data Connection] An unexpected error occurred: {e}")
            self.running = False
        finally:
            try:
                sock.close()
                print("[Data Connection] Data socket closed.")
            except Exception as e:
                print(f"[Data Connection] Error closing data socket: {e}")

    def resync_buffer(self, buffer):
        magic_bytes = struct.pack('>I', MAGIC_NUMBER)
        magic_index = buffer.find(magic_bytes, 1)
        if magic_index != -1:
            return buffer[magic_index:]
        else:
            buffer.clear()
            return bytearray()

    def handle_user_input(self):
        try:
            while self.running:
                command = input("Enter command 'remote_start' or 'remote_stop'. (Enter 'exit' to quit): ").strip()
                if not command:
                    continue
                if command.lower() == 'exit':
                    print("[Command Connection] Exiting.")
                    try:
                        self.cmd_sock.sendall(('exit\n').encode('utf-8'))
                    except Exception as e:
                        print(f"[Command Connection] Failed to send 'exit' command: {e}")
                    self.running = False
                    break

                try:
                    self.cmd_sock.sendall((command + '\n').encode('utf-8'))
                    print(f"[Command Connection] Sent command: {command}")
                except Exception as e:
                    print(f"[Command Connection] Failed to send command: {e}")
                    self.running = False
                    break

                if command.lower() == 'ping':
                    try:
                        response = self.cmd_sock.recv(4096)
                        if response:
                            print(f"[Command Connection] Server response: {response.decode('utf-8').strip()}")
                        else:
                            print("[Command Connection] No response received for ping.")
                    except socket.timeout:
                        print("[Command Connection] Ping timed out. No response received.")
                    except Exception as e:
                        print(f"[Command Connection] An error occurred while receiving ping response: {e}")

        except Exception as e:
            print(f"[Command Connection] An error occurred in user input thread: {e}")
            self.running = False
        finally:
            self.close_sockets()
            print("[Command Connection] Command socket closed.")

    def close_sockets(self):
        try:
            if self.data_sock:
                self.data_sock.close()
                print("[Data Connection] Data socket closed.")
        except Exception as e:
            print(f"[Data Connection] Error closing data socket: {e}")

        try:
            if self.cmd_sock:
                self.cmd_sock.close()
                print("[Command Connection] Command socket closed.")
        except Exception as e:
            print(f"[Command Connection] Error closing command socket: {e}")

    def run(self):
        self.connect()

def main():
    client = OCTClient(host='127.0.0.1', port=1234)
    client.run()

if __name__ == '__main__':
    main()