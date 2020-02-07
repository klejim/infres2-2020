from cryptography.hazmat.primitives.ciphers.aead import AESGCM

import hashlib
import socket

import sys
sys.path.append("..")
import base


AUTHENTICATION_SALT = b'\xa6\x0b\x90\xc1UAh\xf5\x04\x03\x94\xd8'


class MyClient(base.BaseApp):
    def __init__(self, config_file):
        super().__init__()
        self.config = base.read_config(config_file)
        self.log("config : ", self.config)
        if self.config is not None:
            self.ok = True
            self.server_address = self.config["server_address"]
            self.server_port = self.config["server_port"]
            if "key" not in self.config.keys():
                print("Clef de chiffrement absente du fichier config")
                self.ok = False
            else:
                self.key = self.config["key"]
            if "client_name" not in self.config:
                self.name = input("Your name : ")
            else:
                self.name = self.config["client_name"]
            self.password = input("Mot de passe : ")          
        else:
            self.ok = False

    def authenticate_self(self, sckt):
        self.log("=== authenticate_self ===")
        Ha = hashlib.sha256(self.password.encode() + AUTHENTICATION_SALT).digest()
        self.log("Ha = ", Ha)
        self.send(self.name, sckt)
        # calcul + envoi H(challenge+Ha)
        challenge = self.receive(sckt)[30:]
        self.log("challenge received : ", challenge)
        self.log(len(challenge))
        Hr = hashlib.sha256(challenge + Ha).digest()
        self.log("sending Hr : ", Hr)
        self.send(Hr, sckt)
        # réponse du serveur
        answer = self.receive(sckt)[30:]
        ok =  answer.decode() == "OK"
        return ok

    def run(self):
        while True:
            with socket.socket(socket.AF_INET) as sock:
                sock.connect((self.server_address, self.server_port))
                # L'envoi du message est précédé de l'étape d'authentification
                try:
                    if self.authenticate_self(sock):
                        msg = input(">>> ")
                        self.send(msg, sock)
                        answer = self.receive(sock)
                        server_name = answer[:30].strip().decode()
                        server_msg = answer[30:].strip().decode()
                        print("{} : {}".format(server_name, server_msg))
                    else:
                        print("Echec de l'authentification")
                        self.password = input("Mot de passe : ")
                except (ConnectionResetError, ConnectionAbortedError) as err:
                    print("Connexion interrompue par le serveur")
                    sys.exit(0)

if __name__ == "__main__":
    client = MyClient("config.json")
    if client.ok: 
        client.run()