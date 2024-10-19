# Minimalistic script for connecting to OCTproZ with SocketStreamExtension over TCP/IP

import socket

def main():
    host = '127.0.0.1'  # Server address
    port = 1234         # Server port

    # Available commands
    available_commands = [
        'remote_start',
        'remote_stop',
        'load_settings:<file_path>',
        'save_settings:<file_path>'
    ]

    try:
        # Create a TCP/IP socket
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
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
                sock.sendall((command).encode('utf-8'))

    except ConnectionRefusedError:
        print(f"Could not connect to {host}:{port}. Is the server running?")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == '__main__':
    main()
