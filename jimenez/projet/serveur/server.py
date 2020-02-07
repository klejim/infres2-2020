from cryptography.hazmat.primitives.ciphers.aead import AESGCM

import hashlib
import os
import socketserver

import sys
sys.path.append("..")
import base
import database as db


db.connect_db("server_db.db")
#db.print_table("MESSAGES")
#db.print_table("MEMBERS")


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
        else:
            self.ok = False
            self.server_close()

    def validate_config(self):
        pass

    def handle_error(self, *args):
        # gestion de l'exception levée si la connection est interrompue
        etype, value = sys.exc_info()[:2]
        if isinstance(value, ConnectionAbortedError):
            print("Connexion interrompue par le client")


class MyTCPHandler(socketserver.BaseRequestHandler):
    def handle(self):
        # authentification du client
        client_name = self.receive()[:30].decode().strip()
        if self.authenticate_client(client_name):
            data = self.receive(save=True)
            client_name = data[0:30].strip().decode()
            client_msg = data[30:].strip().decode()
            client_display = "{}({})".format(client_name, self.client_address[0])
            print("{} : {}".format(client_display, client_msg))
            msg = input(">>> ")
            self.send(msg, save=True)

    def authenticate_client(self, client_name):
        self.server.app.log("authenticating client", client_name)
        # envoi du challenge
        Ha = db.get_user_hash(client_name)
        self.server.app.log("Ha from db : ", Ha)
        challenge = os.urandom(12)
        self.server.app.log("sending challenge : ", challenge)
        self.send(challenge)
        # calcul H(challenge+Ha)
        h = hashlib.sha256()
        h.update(challenge + Ha)
        Hr = h.digest()
        Hr_client = self.receive()[30:]
        self.server.app.log("Hr from client : ", Hr_client)
        self.server.app.log("Hr : ", Hr)
        ok = Hr == Hr_client
        self.server.app.log(ok)
        self.send("OK" if ok else "NOPE")
        return ok
    
    def receive(self, save=False):
        return self.server.app.receive(self.request, save)

    def send(self, msg, save=False):
        self.server.app.send(msg, self.request, save)
    

if __name__ == "__main__":
    HOST, PORT = 'localhost', 999
    server = MyServer((HOST, PORT), MyTCPHandler)
    print("Connecté en tant que", server.app.name)
    server.serve_forever()
    server.server_close()