# SocketStreamExtension
SocketStreamExtension is an OCTproZ extension designed for streaming processed OCT data to another application running on the same computer or to a different computer within the same network. <br>

You have the option to stream via TCP/IP or through inter-process communication (IPC). IPC is implemented by using QLocalServer and QLocalSocket, which utilize _Unix Domain Sockets_ on Linux operating systems and _Named Pipes_ on Windows.<br>

A simple client application for testing purposes can be found here: [SocketStreamClient](https://github.com/spectralcode/SocketStreamClient)

Don't forget to enable "Stream Processed Data to Ram" in the OCTproZ processing settings!

# How to install
Download zip file from [the release section](https://github.com/spectralcode/SocketStreamExtension/releases) and copy all files into OCTproZ's plugins folder. 
