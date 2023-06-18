'''
Time Trial Tracker - Julian Lewis (2023)

This is the main server (which we'll set up a domain for)

'''

import socket

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

server_address = ('192.168.1.104', 12435)
server_socket.bind(server_address)

# Listen for incoming connections
server_socket.listen(1)
print('Server listening on {}:{}'.format(*server_address))

while True:
    # Wait for a client to connect
    print('Waiting for a connection...')
    client_socket, client_address = server_socket.accept()
    print('Connected to {}:{}'.format(*client_address))

    try:
        # Receive data from the client
        data = client_socket.recv(1024)
        print('Received data: {}'.format(data.decode()))

    finally:
        # Close the client socket
        client_socket.close()