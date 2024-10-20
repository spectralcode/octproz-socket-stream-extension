# Minimalistic script for connecting to OCTproZ with SocketStreamExtension over TCP/IP
# This script allows you to send any command from the list of available commands (refer to "available_commands" within the code)
# Note: It does not receive or display OCT data.


import socket

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
        'enable_command_only_mode',
        'disable_command_only_mode'
    ]

    try:
        # Create a TCP/IP socket
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            # Set a timeout if desired
            # sock.settimeout(5)  # Timeout in seconds

            # Connect to server
            sock.connect((host, port))
            print(f"Connected to {host}:{port}")

            # Display available commands
            print("Available commands:")
            for cmd in available_commands:
                print(f" - {cmd}")

            # Command input loop
            while True:
                command = input("Enter command (or 'exit' to quit): ").strip()

                if command.lower() == 'exit':
                    print("Exiting.")
                    break

                # Send the command to the server
                sock.sendall((command + '\n').encode('utf-8'))

                # If the command is 'ping', 'enable_command_only_mode' or 'disable_command_only_mode', expect a response
                if command.lower() == 'ping' or command.lower() == 'enable_command_only_mode' or command.lower() == 'disable_command_only_mode':
                    try:
                        # Receive response from server
                        response = sock.recv(4096)
                        if response:
                            print(f"Server response: {response.decode('utf-8').strip()}")
                        else:
                            print("No response received.")
                    except socket.timeout:
                        print("Time out. No response received.")
                    except Exception as e:
                        print(f"An error occurred while receiving response: {e}")

    except ConnectionRefusedError:
        print(f"Could not connect to {host}:{port}. Is the server running?")
    except socket.timeout:
        print("Socket timed out.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == '__main__':
    main()
