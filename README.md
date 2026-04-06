# SocketStreamExtension
SocketStreamExtension is an OCTproZ extension designed for streaming processed OCT data to another application running on the same computer or to a different computer within the same network. <br>

You have the option to stream via TCP/IP or through inter-process communication (IPC). IPC is implemented by using QLocalServer and QLocalSocket, which utilize _Unix Domain Sockets_ on Linux operating systems and _Named Pipes_ on Windows.<br>

A simple client application for testing purposes can be found here: [SocketStreamClient](https://github.com/spectralcode/SocketStreamClient)

Don't forget to enable "Stream Processed Data to Ram" in the OCTproZ processing settings!

# Example usage with Python
You can find a minimalistic python script that shows how to connect to SocketStreamExtensions in the [examples folder](examples)

# How to install
Download zip file from [the release section](https://github.com/spectralcode/SocketStreamExtension/releases) and copy all files into OCTproZ's plugins folder. 

# Available Remote Commands

`SocketStreamExtension` supports several remote commands to control OCT processing and settings. These commands are sent as strings and should be formatted as described below.

- **`remote_start`**: Starts the OCT processing.
- **`remote_stop`**: Stops the OCT processing.
- **`remote_record`**: Starts recording the OCT data.
- **`load_settings:<path_to_settings_file>`**: Loads settings from a specified file. Replace `<path_to_settings_file>` with the actual file path.
- **`save_settings:<path_to_settings_file>`**: Saves current settings to a specified file. Replace `<path_to_settings_file>` with the desired file path.
- **`set_disp_coeff:<coeff1>:<coeff2>:<coeff3>:<coeff4>`**: Sets dispersion coefficients. Each coefficient can be a double or `nullptr` (or `null`) if not applicable.
- **`set_grayscale_conversion:<enable_log_scaling>:<max>:<min>:<multiplicator>:<offset>`**: Configures grayscale conversion. Parameters include:
  - `enable_log_scaling`: Set to `true` (or `1`) to enable log scaling, or `false` (or `0`) to disable it.
  - `max`, `min`, `multiplicator`, `offset`: Double values for the respective grayscale conversion settings. Use `nan`, `null`, or `nullptr` for any parameter you wish to leave unset.
- **`set_klin_coeffs:<c0>:<c1>:<c2>:<c3>`**: Sets the polynomial coefficients for k-linearization. Each coefficient can be a double or `null` (or `nullptr`) to leave the respective coefficient unchanged.
- **`set_klin_curve:<v0>,<v1>,<v2>,...,<vN>`**: Sets a custom resampling curve for k-linearization by providing comma-separated float values directly in the command.
- **`load_klin_curve:<file_path>`**: Loads a custom resampling curve from a CSV file. The file should use semicolons as delimiters with the resampling values in the second column. The first line is skipped as a header. This is the same format used by OCTproZ's sidebar for loading resampling curves. Use forward slashes or double backslashes in the file path. Examples:
  - Windows: `load_klin_curve:C:/Users/username/curves/klin_curve.csv`
  - Linux: `load_klin_curve:/home/username/curves/klin_curve.csv`

If an invalid command or format is detected, an error message will be emitted to help debug the issue.