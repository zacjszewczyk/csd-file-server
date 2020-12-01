File Server
===========

This project consists of a lightweight file server that can store and serve files, written in C, and a client that can upload and request files using simple protocol, written in Python.

## System Requirements

Both the server and client use only standard libraries. The Python client was built using Python 3.7.3.

## Usage

To use these tools, simply clone this repository and transfer the `server` folder to your file server, and the `client` folder to your workstation. Start the file server with a command like the one below, which will serve and store files from the current working directory over port 8000:

```
./server -p 8000 -d ./
```

The file server will accept any valid port number and any valid directory path on startup.

With the file server running, upload the file `0.html` with the following command:

```
./client.py -s localhost -p 8000 -f 0.html -u
```

You may also download the file `c.png` from the file server with the following command:

```
./client.py -s localhost -p 8000 -f c.png -d
```

## License

This project is licensed under the [Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License](https://creativecommons.org/licenses/by-nc-sa/4.0/). Read more about the license [at my website](https://zacs.site/disclaimers.html). Generally speaking, this license allows individuals to remix this work provided they release their adaptation under the same license and cite this project as the original, and prevents anyone from turning this work or its derivatives into a commercial product.