
import socket
import threading

def receive_data(sock):
    """
    Function to receive data from the server asynchronously.
    """
    try:
        while True:
            data = sock.recv(4096)
            if data:
                # Place your data processing logic here
                pass
            else:
                # Connection closed by the server
                print("Server closed the data connection.")
                break
    except Exception as e:
        print(f"An error occurred while receiving data: {e}")

def main():
    host = '127.0.0.1'  # Server address
    port = 1234         # Server port

    # Available commands
    available_commands = [
        'remote_start',
        'remote_stop',
        'load_settings:<file_path>',
        'save_settings:<file_path>',
        'ping',
    ]

    try:
        # Create the command socket
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as cmd_sock:
            cmd_sock.connect((host, port))
            print(f"Command connection established to {host}:{port}")

            # Send identification message
            cmd_sock.sendall(('enable_command_only_mode' + '\n').encode('utf-8'))
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
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as data_sock:
                data_sock.connect((host, port))
                print(f"Data connection established to {host}:{port}")

                # Send identification message if needed
                # data_sock.sendall(b"DATA\n")

                # Start the data reception thread
                data_thread = threading.Thread(target=receive_data, args=(data_sock,), daemon=True, name='DataReceiverThread')
                data_thread.start()

                # Command input loop
                while True:
                    command = input("Enter command (or 'exit' to quit): ").strip()

                    if command.lower() == 'exit':
                        print("Exiting.")
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

    except ConnectionRefusedError:
        print(f"Could not connect to the server at {host}:{port}. Is the server running?")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == '__main__':
    main()