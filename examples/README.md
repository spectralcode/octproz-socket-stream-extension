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

- **Description:** Extends the previous script by processing and displaying OCT using mtplotlib
- **Features:**
  - Displays images using `matplotlib`.
  - Shows FPS information on the plot.
  - Handles different image dimensions and bit depths (configurable in the script).

### 3. `octproz_tcpip_connection_opencv.py`

- **Description:** Displays received OCT data in real-time using OpenCV. The header information streamed with the OCT data is utilized to automatically set buffer and image dimensions.
- **Features:**
  - Displays images using `OpenCV`.
  - Shows data rate, FPS received and FPS displayed information on the window title.

### 45. `octproz_websocket_client.html`

- **Description:** WebSocket client that runs in a browser
- **Link:** [OCTproZ WebSocket Demo](https://spectralcode.github.io/SocketStreamExtension/examples/octproz_websocket_client.html)
- **Features:**
  - Connects to the OCTproZ WebSocket server and displays real-time OCT images.
  - Allows manipulation of displayed images through zoom, translation, and rotation.
  - Shows FPS.
