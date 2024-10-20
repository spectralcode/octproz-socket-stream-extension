## Scripts Overview

### 1. `octproz_tcpip_connection.py`

- **Description:** A minimalistic script to connect to OCTproZ and send commands from a predefined list.
- **Note:** This script does **not** receive or display OCT data.

### 2. `octproz_tcpip_connection_two_clients.py`

- **Description:** Establishes two TCP/IP connections to OCTproZ:
  - **Command Connection:** For sending commands and receiving responses.
  - **Data Connection:** For receiving OCT data asynchronously.
- **Note:** Data processing logic is left for the user to implement.

### 3. `octproz_tcpip_connection_two_clients_display.py`

- **Description:** Extends the previous script by processing and displaying OCT images in real-time.
- **Features:**
  - Displays images using `matplotlib`.
  - Shows FPS (Frames Per Second) information on the plot.
  - Handles different image dimensions and bit depths (configurable in the script).
