import socket

host = "0.0.0.0"  # Escucha en todas las interfaces
port = 15000

with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as server_socket:
    server_socket.bind((host, port))
    print(f"Escuchando en {host}:{port}")

    while True:
        data, addr = server_socket.recvfrom(1024)
        print(f"Recibido: {data.decode('utf-8')} desde {addr}")
