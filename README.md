# dhstream
Stream video from Dahua security cameras using SDK

## Installation

### Requirements
- [Dahua NetSDK][sdk]
- A C++ 17 compatible compiler
- Linux or MacOS

Note
1. Make sure to download the correct SDK build for your platform
2. The code might work on Windows as well (maybe with some tweak), but it is currently untested.

### Steps
1. Clone the repository
2. Copy `libdhnetsdk.so` and `dhnetsdk.h` from [Dahua NetSDK][sdk] to the repository
3. Run `make`

Note: On Linux, you may need to ensure `libdhnetsdk.so` is in the `LD_LIBRARY_PATH` environment variable. You can do this by either copying `libdhnetsdk.so` to a system directory (e.g., `/usr/lib/`) or manually setting the `LD_LIBRARY_PATH` environment variable to point to the current directory.

## Usage

See `--help`

## Typical use

The video stream is printed to `stdout` using DHAV muxer. `ffmpeg` can demux this format. You can view the stream using the following command.
```sh
./dhstream 192.168.xx.xx 37777 -u admin -p password -s 1 | ffplay -i pipe:0
```

You may also redirect the data stream to a file
```sh
./dhstream 192.168.xx.xx 37777 -u admin -p password -s 1 > outputdhav
```
and process it with `ffmpeg` later (for example, convert it to other format)
```sh
ffmpeg -i outputdhav -c copy output.mkv 
```

## If the RTSP stream exists, why using this?

Using the correct parameters, this stream can have lower latency than the RTSP stream in some circumstances.

For example, to get generally good result
```sh
./dhstream ... | ffplay -probesize 32 -sync ext -f dhav -i pipe:0
```

To get the lowest latency (but at the cost of higher frame drop)
```sh
./dhstream ... | ffplay -fflags nobuffer -f dhav -i pipe:0
```

## License

MIT

[sdk]: https://depp.dahuasecurity.com/integration/guide/download/sdk