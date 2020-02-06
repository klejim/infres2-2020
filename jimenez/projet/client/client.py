from cryptography.hazmat.primitives.ciphers.aead import AESGCM

import json
import os
import socket

SERVER_HA = None # todo : une base de données...

def encrypt():
    key = AESGCM.generate_key(bit_length=256)
    aesgcm = AESGCM(key)
    salt = os.urandom(12)
    salted_key = aesgcm.encrypt(salt, key, b"")
    print("salted key", salted_key)

def read_config(filename):
    conf = {}
    try:
        conf = json.load(open(filename, "r"))
    except (json.JSONDecodeError, IOError) as err:
        print("Erreur dans le fichier cong : ", err)
        conf = None
    finally:
        return conf

class MyClient:
    def __init__(self, config_file):
        self.config = read_config(config_file)
        print("config : ", self.config)
        if self.config is not None:
            self.ok = True
            self.server_address = self.config["server_address"]
            self.server_port = self.config["server_port"]
            if "key" not in self.config:
                print("Clef de chiffrement absente du fichier config")
                self.ok = False
            else:
                self.key = self.config["key"]
            if "client_name" not in self.config:
                self.name = self.config["client_name"]
            else:
                self.name = input("Your name : ")
            self.key = b'6\x0c{\xd0l-?o`\xf0\xe5\xf0\xd2\xad\x90\xa3$\x08Xb&\x95\xb5\x07y;\xc0\x15\x10\xac\x97\x13'
            self.aesgcm = AESGCM(self.key)
        else:
            self.ok = False

    def encrypt(self, data):
        salt = os.urandom(12)
        ct = self.aesgcm.encrypt(salt, data.encode(), b'')
        return ct, salt

    def decrypt(self, data, noonce):
        return self.aesgcm.decrypt(noonce, data, b'')

    def send(self, msg, sckt):
        print("=== send ===")
        content = self.name.ljust(30) + msg
        print("content = ", content)
        encrypted_content, noonce = self.encrypt(content)
        print("content chiffré = ", encrypted_content)
        sckt.sendall(noonce + encrypted_content)

    def receive(self, sckt):
        answer = sckt.recv(1024)
        print("===receive===")
        # les douze premiers octets sont le nonnce
        nonnce, content = answer[:12], answer[12:]
        print("nonnce = {}, content = {}".format(nonnce, content))
        text = self.decrypt(content, nonnce).decode()
        print("texte déchiffré = ", text)
        return text

    def run(self):
        while True:
            with socket.socket(socket.AF_INET) as sock:
                sock.connect((self.server_address, self.server_port))
                msg = input(">>> ")
                self.send(msg, sock)
                answer = self.receive(sock)
                server_name = answer[:30].strip()
                server_msg = answer[30:].strip()
                print("{} : {}".format(server_name, server_msg))


if __name__ == "__main__":
    client = MyClient("config.json")
    if client.ok: 
        client.run()