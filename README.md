# SocketStreamExtension

SocketStreamExtension is an OCTproZ extension designed for streaming processed OCT data to another application running on the same computer or to a different computer within the same network.

You have the option to stream via TCP/IP or through inter-process communication (IPC). IPC is implemented by using QLocalServer and QLocalSocket, which utilize _Unix Domain Sockets_ on Linux operating systems and _Named Pipes_ on Windows.

A simple client application for testing purposes can be found here: [SocketStreamClient](https://github.com/spectralcode/SocketStreamClient)

Don't forget to enable "Stream Processed Data to Ram" in the OCTproZ processing settings!

# Example usage with Python
You can find a minimalistic python script that shows how to connect to SocketStreamExtensions in the [examples folder](examples)

# How to install
Download zip file from [the release section](https://github.com/spectralcode/SocketStreamExtension/releases) and copy all files into OCTproZ's plugins folder.

# Available Remote Commands

SocketStreamExtension supports remote commands to control OCT processing and settings. Commands are sent as newline-terminated strings over the socket connection.

## Processing Control

| Command | Description |
|---------|-------------|
| `remote_start` | Start OCT processing |
| `remote_stop` | Stop OCT processing |

## Recording

| Command | Description |
|---------|-------------|
| `remote_record` | Start recording using current settings |
| `remote_record:<path>` | Set save folder, then start recording |
| `remote_record:<path>\|<name>` | Set save folder and file name, then start recording |
| `set_rec_path:<path>` | Set recording save folder (path must exist) |
| `set_rec_name:<name>` | Set recording file name |
| `set_buffers_to_record:<N>` | Set number of buffers to record (`N` > 0) |
| `set_rec_options:<key>=<val>:...` | Set recording option flags (see below) |
| `set_preallocation:<0\|1>` | Enable/disable recording buffer preallocation |

The `|` character separates path from name in `remote_record` because `|` is invalid in file paths on all operating systems, avoiding ambiguity with colons in Windows paths.

Examples:
- `remote_record:C:/Users/username/data|experiment_001`
- `remote_record:/home/username/data|experiment_001`

**Note:** Recording commands (`set_rec_path`, `set_rec_name`, `set_buffers_to_record`, `set_rec_options`, `set_preallocation`) modify the internal parameters directly without updating the OCTproZ sidebar GUI. If the sidebar is used after these remote commands, the sidebar values may overwrite the remotely set parameters.

### Recording Options (`set_rec_options`)

Only specified keys are changed; omitted keys keep their current values. All values are boolean (`1`/`true` or `0`/`false`).

| Key | Description |
|-----|-------------|
| `raw` | Record raw buffers |
| `processed` | Record processed buffers |
| `screenshot` | Save screenshots |
| `meta` | Save metadata |
| `stop_after` | Stop acquisition after recording |
| `start_first` | Start recording with the first buffer |
| `float32` | Save as 32-bit float |

Example: `set_rec_options:raw=1:processed=1:meta=0:stop_after=1`

### Buffer Preallocation (`set_preallocation`)

When enabled (`set_preallocation:1`), recording memory is allocated and committed immediately. This avoids potential allocation delays when recording starts. Use `set_preallocation:0` to free the preallocated memory.

## Settings

| Command | Description |
|---------|-------------|
| `load_settings:<path>` | Load settings from file |
| `save_settings:<path>` | Save current settings to file |

Examples:
- `load_settings:C:/Users/username/octproz_settings.ini`
- `save_settings:C:/Users/username/octproz_settings_backup.ini`

## Processing Parameters

| Command | Description |
|---------|-------------|
| `set_disp_coeff:<c1>:<c2>:<c3>:<c4>` | Set dispersion coefficients (double or `null`) |
| `set_grayscale_conversion:<log>:<max>:<min>:<mult>:<offset>` | Configure grayscale conversion |
| `set_klin_coeffs:<c0>:<c1>:<c2>:<c3>` | Set k-linearization polynomial coefficients (double or `null`) |
| `set_klin_curve:<v0>,<v1>,...,<vN>` | Set custom resampling curve (comma-separated floats) |
| `load_klin_curve:<file_path>` | Load resampling curve from CSV file |

### Dispersion Coefficients (`set_disp_coeff`)
Each coefficient can be a double or `nullptr`/`null` to leave the respective coefficient unchanged.

Examples:
- `set_disp_coeff:0.0:1.5e-6:0.0:0.0` — set all four coefficients
- `set_disp_coeff:null:1.5e-6:null:null` — change only the second coefficient

### Grayscale Conversion (`set_grayscale_conversion`)
- `enable_log_scaling`: `true`/`1` or `false`/`0`
- `max`, `min`, `multiplicator`, `offset`: Double values. Use `nan`, `null`, or `nullptr` for any parameter to leave it unchanged.

### K-Linearization Curve Files (`load_klin_curve`)
The CSV file should use semicolons as delimiters with resampling values in the second column. The first line is skipped as a header. This is the same format used by OCTproZ's sidebar for loading resampling curves. Use forward slashes in file paths.

Examples:
- Windows: `load_klin_curve:C:/Users/username/curves/klin_curve.csv`
- Linux: `load_klin_curve:/home/username/curves/klin_curve.csv`

