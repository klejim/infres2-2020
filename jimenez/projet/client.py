import socket
ADRESS = "127.0.0.1"
PORT = 999
client_name = input("Your name : ")
while True:
    with socket.socket(socket.AF_INET) as sock:
        sock.connect((ADRESS, PORT))
        msg = input(">>> ")
        content = client_name.ljust(30) + msg
        sock.sendall(content.encode())
        answer = sock.recv(1024).decode()
        server_name, server_msg = answer[:30].strip(), answer[30:].strip()
        print("{} : {}".format(server_name, server_msg))
