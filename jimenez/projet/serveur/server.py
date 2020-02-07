from cryptography.hazmat.primitives.ciphers.aead import AESGCM

import hashlib
import os
import socketserver

import sys
sys.path.append("..")
import base

# todo : base de données...
CLIENT_HA = b'L\xa0\x8e\x04\xac\xda\x0fh\x9d\xa1A\xbb\x10\xfc\xfePI\xfa\x19\xda\x9bW\xf0\x8b\x91\xcd%j\x84UF\x04'
#AUTHENTICATION_SALT = b'\x9f\xf9\xf1\xe9TT\x14Y\x9d\xa7\xc7\xaf'
hashed_client_salts = {}

class MyServer(socketserver.TCPServer):
    def __init__(self, address, handler):
        super().__init__(address, handler)
        self.app = base.BaseApp()
        self.config = base.read_config("config.json")
        self.app.log(self.config)
        if self.config is not None:
            self.ok = True
            if "server_name" in self.config:
                self.app.name = self.config["server_name"]
            else:
                self.app.name = input("Server name : ")
            self.password = input("Mot de passe : ")
        else:
            self.ok = False
            self.server_close()

    def validate_config(self):
        pass


class MyTCPHandler(socketserver.BaseRequestHandler):
    def handle(self):
        # authentification du client
        client_name = self.request.recv(1024)
        if self.authenticate_client(client_name):
            data = self.receive()
            client_name = data[0:30].strip().decode()
            client_msg = data[30:].strip().decode()
            client_display = "{}({})".format(client_name, self.client_address[0])
            print("{} : {}".format(client_display, client_msg))
            msg = input(">>> ")
            self.send(msg)

    def authenticate_client(self, client_name):
        self.app.log("authenticating client", client_name)
        # envoi du challenge
        self.app.log(CLIENT_HA)
        self.app.log()
        challenge = os.urandom(12)
        self.app.log("sending challenge : ", challenge)
        self.send(challenge)
        # calcu H(challenge+Ha)
        h = hashlib.sha256()
        h.update(challenge + CLIENT_HA)
        Hr = h.digest()
        Hr_client = self.receive()[30:]
        self.app.log("Hr from client : ", Hr_client)
        self.app.log("Hr : ", Hr)
        ok = Hr == Hr_client
        self.app.log(ok)
        self.send("OK" if ok else "NOPE")
        return ok
    
    def receive(self):
        return self.server.app.receive(self.request)

    def send(self, msg):
        self.server.app.send(msg, self.request)
    

if __name__ == "__main__":
    HOST, PORT = 'localhost', 999
    server = MyServer((HOST, PORT), MyTCPHandler)
    server.serve_forever()
    server.server_close()