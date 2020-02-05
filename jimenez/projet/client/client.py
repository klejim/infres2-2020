import json
import socket

def read_config(filename):
    conf = {}
    try:
        conf = json.loads(filename)
    except (json.JSONDecodeError, IOError) as err:
        conf = {
            "client_name": "François"
        }
    finally:
        return conf

ADRESS = "127.0.0.1"
PORT = 999

config = read_config("config.json")

if "client_name" in config:
    client_name = config["client_name"]
else:
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
