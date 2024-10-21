import socket
import threading
import numpy as np
import matplotlib.pyplot as plt
from queue import Queue
import time

def receive_data(sock, image_queue, image_size, bytes_per_pixel):
    """
    Function to receive data from the server asynchronously.
    Accumulates data until a full image is received, then puts it into a queue.
    """
    buffer = bytearray()
    total_bytes = image_size * bytes_per_pixel

    try:
        while True:
            data = sock.recv(4096)
            if data:
                buffer.extend(data)
                # Process all complete images in the buffer
                while len(buffer) >= total_bytes:
                    # Extract image data from buffer
                    image_data = buffer[:total_bytes]
                    buffer = buffer[total_bytes:]

                    # Put the image data into the queue for the main thread
                    image_queue.put(image_data)
            else:
                # Connection closed by the server
                print("Server closed the data connection.")
                break
    except Exception as e:
        print(f"An error occurred while receiving data: {e}")

def handle_user_input(cmd_sock):
    """
    Function to handle user input in a separate thread.
    """
    try:
        while True:
            command = input("Enter command (or 'exit' to quit): ").strip()
            if command.lower() == 'exit':
                print("Exiting.")
                cmd_sock.close()
                break

            # Send the command to the command server
            cmd_sock.sendall((command + '\n').encode('utf-8'))

            # Handle 'ping' response
            if command.lower() == 'ping':
                try:
                    response = cmd_sock.recv(4096)
                    if response:
                        print(f"Server response: {response.decode('utf-8').strip()}")
                    else:
                        print("No response received for ping.")
                except socket.timeout:
                    print("Ping timed out. No response received.")
                except Exception as e:
                    print(f"An error occurred while receiving ping response: {e}")

    except Exception as e:
        print(f"An error occurred in user input thread: {e}")

def main():
    host = '127.0.0.1'  # Server address
    port = 1234         # Server port

    # Image parameters
    width = 1664 // 2   # 832 pixels
    height = 512        # 512 pixels
    bit_depth = 8       # Can be 8, 12, 16, etc.
    bytes_per_pixel = (bit_depth + 7) // 8  # Calculate bytes per pixel
    image_size = width * height

    # Available commands
    available_commands = [
        'remote_start',
        'remote_stop',
        'remote_record',
        'load_settings:<file_path>',
        'save_settings:<file_path>',
        'ping',
    ]

    try:
        # Create the command socket
        cmd_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        cmd_sock.connect((host, port))
        print(f"Command connection established to {host}:{port}")

        # Send identification message
        cmd_sock.sendall(('enable_command_only_mode\n').encode('utf-8'))
        try:
            response = cmd_sock.recv(4096)
            if response:
                print(f"Server response: {response.decode('utf-8').strip()}")
            else:
                print("No response received for command mode activation.")
        except socket.timeout:
            print("Timeout. No response received for command mode activation.")
        except Exception as e:
            print(f"An error occurred while receiving response: {e}")

        # Create the data socket
        data_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        data_sock.connect((host, port))
        print(f"Data connection established to {host}:{port}")

        # Start the data reception thread
        image_queue = Queue()
        data_thread = threading.Thread(target=receive_data, args=(data_sock, image_queue, image_size, bytes_per_pixel), daemon=True)
        data_thread.start()

        # Start the user input thread
        user_input_thread = threading.Thread(target=handle_user_input, args=(cmd_sock,), daemon=True)
        user_input_thread.start()

        # Setup matplotlib interactive mode
        plt.ion()
        fig, ax = plt.subplots()
        img_plot = None

        # Variables for FPS calculation
        fps = 0.0
        fps_display_interval = 1.0  # Update FPS every 1 second
        frame_count = 0
        fps_time = time.time()

        while True:
            if not image_queue.empty():
                image_data = image_queue.get()

                # Convert the image data to a NumPy array
                if bit_depth <= 8:
                    dtype = np.uint8
                elif bit_depth <= 16:
                    dtype = np.uint16
                else:
                    dtype = np.uint32  # Adjust as needed for higher bit depths

                image_array = np.frombuffer(image_data, dtype=dtype)
                image_array = image_array.reshape((height, width))

                # Update FPS calculation
                frame_count += 1
                current_time = time.time()
                elapsed_time = current_time - fps_time
                if elapsed_time >= fps_display_interval:
                    fps = frame_count / elapsed_time
                    frame_count = 0
                    fps_time = current_time

                # Display the image
                if img_plot is None:
                    img_plot = ax.imshow(image_array, cmap='gray', vmin=0, vmax=2**bit_depth - 1)
                    plt.title('OCT Image')
                    plt.colorbar(img_plot, ax=ax)
                    # Add FPS text to the plot
                    fps_text = ax.text(0.05, 0.95, '', transform=ax.transAxes, color='yellow', fontsize=14, verticalalignment='top')
                else:
                    img_plot.set_data(image_array)
                    img_plot.set_clim(0, 2**bit_depth - 1)
                # Update FPS text
                fps_text.set_text(f'FPS: {fps:.2f}')
                plt.draw()
                plt.pause(0.001)  # Brief pause to allow the plot to update

            else:
                # Sleep briefly to prevent 100% CPU usage
                time.sleep(0.01)

            # Check if user input thread is still alive
            if not user_input_thread.is_alive():
                print("User input thread has exited. Exiting main loop.")
                break

        # Close sockets
        data_sock.close()
        print("Connections closed.")

    except ConnectionRefusedError:
        print(f"Could not connect to the server at {host}:{port}. Is the server running?")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == '__main__':
    main()
