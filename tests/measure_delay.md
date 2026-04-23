# measure_delay.py

A small latency probe for the Socket Stream Extension.

## What it measures

For each frame OCTproZ sends, the script measures how long the frame took to arrive; from the moment OCTproZ sent it to the moment the script finished receiving it. After 1000 frames it prints mean and median times. 

## How it works

OCTproZ stamps each frame's header with the current time right before sending it. When the script finishes receiving the frame, it reads that stamp and compares it to its own clock. The difference is the delay.

Both clocks are on the same machine, so they're directly comparable.

Header layout when the timestamp option is enabled (21 bytes, big-endian):

```
magic (uint32) | size (uint32) | width (uint16) | height (uint16) | bitDepth (uint8) | send_ms (uint64)
```


## How to use

1. In OCTproZ, open the Socket Stream Extension and enable:
   - **Include header to data transfer**
   - **Append send timestamp (for latency measurement)**
2. Pick a transport (TCP/IP or IPC) and click **Start**.
3. Start acquisition so frames are being broadcast.
4. Run the script:

   ```
   python measure_delay.py                      # TCP on 127.0.0.1:1234
   python measure_delay.py --pipe <pipe_name>   # IPC, matches the extension's Pipe name
   ```


## Limits

- Single-machine only. The script compares OCTproZ's wall clock to Python's wall clock; it assumes both are on the same host so the clocks match.
