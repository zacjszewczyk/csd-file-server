#!/usr/bin/python3

# Imports
from argparse import ArgumentParser, ArgumentTypeError # CLI argument parsing
from re import match # Regex IP input validation
from os.path import exists # Validate files to send/receive
import socket

# Constants
CHUNKSIZE = 1024
ENCODING = "utf-8"

# Method: ip
# Purpose: Validate that the user entered a string containing four groups of
# 1 to 3 digits each as the server's IP address.
# Parameters:
# - arg: User-submitted server IP address.
# Return:
# - Success: User-submitted server IP address. (String)
# - Failure: Error raised and execution halted. (ArgumentTypeError)
def ip(arg):
    # Transform "localhost" into a valid IP address.
    if (arg == "localhost"): arg = "127.0.0.1"
    # Validate user-submitted server IP address.
    if not match(r"(\d{1,3}\.){3}\d{1,3}",arg): raise ArgumentTypeError
    return arg

class conn:
    def __init__(self, server=None, server_port=None):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server = server
        self.server_port = server_port

    def connect(self, server=None, server_port=None):
        # Ensure that the user has, either when creating a new "conn" object
        # or when calling this function, provided a server IP address. Store
        # that server IP address in the self.server variable.
        if (self.server == None and server == None):
            raise Exception("Error: No server specified. Please try again.")
        self.server = self.server if self.server != None else server
        
        # Ensure that the user has, either when creating a new "conn" object
        # or when calling this function, provided a server port. Store that
        # server port in the self.server_port variable.
        if (self.server_port == None and server_port == None):
            raise Exception("Error: No server port specified. Please try again.")
        self.server_port = self.server_port if self.server_port != None else server_port
        
        # Connect to the server at self.server on port self.server_port
        self.sock.connect((self.server, self.server_port))

    def close(self):
        self.sock.shutdown(socket.SHUT_RDWR)
        self.sock.close()

    def send_file(self, filename):
        fd = open(filename, "rb")
        while True:
            chunk = fd.read(CHUNKSIZE)
            self._send(chunk)
            if (len(chunk) < CHUNKSIZE): break
        fd.close()
        del fd

    def send_msg(self, msg):
        b = bytes(msg, encoding="utf-8")
        for chunk in (b[i:i+CHUNKSIZE] for i in range(0, len(b), CHUNKSIZE)):
            self._send(chunk)
        del b

    def _send(self, buff):
        self.sock.send(buff)
    
    def save_as(self, filename):
        fd = open(filename, "wb")
        fd.write(self._recv())
        fd.close()
        return 0

    def get_msg(self):
        return self._recv().decode("utf-8")

    def _recv(self):
        buff = []
        while True:
            buff.append(self.sock.recv(CHUNKSIZE))
            if (len(buff[-1]) < 1024): break
        return b"".join(buff)


    # def myreceive(self):
    #     chunks = []
    #     bytes_recd = 0
    #     while bytes_recd < MSGLEN:
    #         chunk = self.sock.recv(min(MSGLEN - bytes_recd, 2048))
    #         if chunk == b'':
    #             raise RuntimeError("socket connection broken")
    #         chunks.append(chunk)
    #         bytes_recd = bytes_recd + len(chunk)
    #     return b''.join(chunks)

# If run as a standalone program, parse the supplied arguments and act
# accordingly; if this script is imported into another, this section will
# not execute.
if (__name__ == "__main__"):
    # Define an instance of the built-in command-line interface argument parser.
    parser = ArgumentParser()

    # Add flags.
    ## Add optional flag for controlling output verbosity, "-v" or "--verbose".
    parser.add_argument("-v", "--verbose", help="Verbose output", action="store_true", dest="verbose")

    ## Add required flags.
    ### a. An argument indicates the server to connect to. (-s or --server)
    parser.add_argument("-s", "--server", help="Server IP address", type=ip, required=True, dest="server")
    ### b. An argument indicates the port to attempt to connect to the server over.
    ###    (-p or --port)
    parser.add_argument("-p", "--port", help="Server port", type=int, required=True, dest="server_port")
    ### c. An argument indicates whether or not the client is uploading a file or
    ###    downloading a file. (-u or --upload for uploading,
    ###    or -d or --download for downloading). Only one is accepted; entering 
    ###    both will generate an error and halt execution.
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("-u", "--upload", help="Upload flag", action="store_const", dest="action", const="UPLOAD")
    group.add_argument("-d", "--download", help="Download flag", action="store_const", dest="action", const="DOWNLOAD")
    ### d. An argument that indicates the name of the file that will be uploaded or requested for download.
    parser.add_argument("-f", "--file", help="Target file name", type=str, required=True, dest="filename")

    # Parse command-line arguments.
    args = parser.parse_args()

    # Perform basic bounds checking
    ## Validate all server IP address octets are within the valid 0-255 range.
    ## Produce an error and then exit if not.
    for octet in args.server.strip().split("."):
        if (int(octet) < 0 or int(octet) > 255):
            raise Exception("Error: Invalid server IP address. Please try again.")
    ## Validate server port is within the valid range 0-65535. Produce an error
    ## and then exit if not.
    if (args.server_port < 0 or args.server_port > 65535):
        raise Exception("Error: Invalid server port specified. Please try again.")

    # Create a new connection object c for the server connection, then connect.
    c = conn(args.server, args.server_port)
    c.connect()

    # Handle file uploading.
    if (args.action == "UPLOAD"):
        # Verify that the file to be uploaded exists on the local system.
        # Produce an error and then exit if it does not.
        if not (exists(args.filename)):
            raise Exception("Error: File to upload does not exist.")

        # Verbose output
        if (args.verbose): print(f"Uploading file {args.filename} ... ", end="", flush=True)

        # Notify the server of a pending file upload.
        c.send_msg(f"{args.action} {args.filename}")
        
        # If the server is ready for the file upload, first send the filename,
        # then upload the file.
        if ("READY" in c.get_msg()):
            c.send_file(args.filename)
        # If the server is not ready for the file upload, print an error
        # message for the user and prompt them to retry.
        else:
            print("Error: Server not ready to accept file transfer. Please try again.")
            exit(0)

        # Verbose output
        if (args.verbose): print("done")

    elif (args.action == "DOWNLOAD"):
        # Verify that the file to be downloaded does not already exist. Produce
        # an error and then exit if downloading the specified file would
        # overwrite an existing one.
        if (exists(args.filename)):
            raise Exception("Error: File to download already exists.")
        
        # Request a file download from the server.
        c.send_msg(f"DOWNLOAD {args.filename}")

        # If the server is ready for the file download, send the filename.
        if (c.get_msg() == "READY"):
            # If the server responde with "SENDING", receive the file and save
            # it with the filename supplied by the user.
            resp  = c.get_msg()
            if ("SENDING" in resp):
                # Verbose output
                if (args.verbose): print(f"Downloading file {args.filename} with {resp.split(' ',1)[1]} bytes ... ", end="", flush=True)
                c.save_as(args.filename)
                print("done.")
            # If the server responds with "FILE_NOT_FOUND", the file does not
            # exist.
            else:
                print("Error: Server cannot find file. Please try again.")
                exit(0)    
        else:
            print("Error: Server not ready to transfer file. Please try again.")
            exit(0)
    # Account for an edge case where the client provides an unspecified action.
    else:
        raise Exception("Invalid action specified. Please try again.")

    # Close the connection to the server once the upload or download operation
    # has completed. Account for the fact that the server may close the
    # connection first.
    try: c.close()
    except: pass