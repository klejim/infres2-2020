from cryptography.hazmat.primitives.ciphers.aead import AESGCM

import json
import os
import socketserver

# todo : base de données...
CLIENT_HA = b'L\xa0\x8e\x04\xac\xda\x0fh\x9d\xa1A\xbb\x10\xfc\xfePI\xfa\x19\xda\x9bW\xf0\x8b\x91\xcd%j\x84UF\x04'


def read_config(filename):
    conf = {}
    try:
        conf = json.load(open(filename, "r"))
    except (json.JSONDecodeError, IOError) as err:
        print("erreur dans le fichier conf : ", err)
        conf = None
    finally:
        return conf

class MyServer(socketserver.TCPServer):
    def __init__(self, address, handler):
        self.config = read_config("config.json")
        print(self.config)
        if self.config is not None:
            self.ok = True
            if "server_name" in self.config:
                self.name = self.config["server_name"]
            else:
                self.name = input("Server name : ")
            self.password = input("Mot de passe : ")
            self.key = b'6\x0c{\xd0l-?o`\xf0\xe5\xf0\xd2\xad\x90\xa3$\x08Xb&\x95\xb5\x07y;\xc0\x15\x10\xac\x97\x13'
            print("taille de la clef", len(self.key))
            self.aesgcm = AESGCM(self.key)
        else:
            self.ok = False
            self.server_close()
        super().__init__(address, handler)

    def validate_config(self):
        pass


class MyTCPHandler(socketserver.BaseRequestHandler):
    def handle(self):
        # authentification du client
        #
        data = self.receive()
        client_name, client_msg = data[0:30].strip(), data[30:].strip()
        client_display = "{}({})".format(client_name, self.client_address[0])
        print("{} : {}".format(client_display, client_msg))
        msg = input(">>> ")
        self.send(msg)

    def encrypt(self, data):
        noonce = os.urandom(12)
        ct = self.server.aesgcm.encrypt(noonce, data.encode(), b'')
        return ct, noonce

    def decrypt(self, data, noonce):
        return self.server.aesgcm.decrypt(noonce, data, b'')

    def receive(self):
        print("=== receive ===")
        answer = self.request.recv(1024)
        print("answer = ", answer)
        # les douze premiers octets sont le noonce
        noonce, content = answer[:12], answer[12:]
        print("noonce = {}, content = {}".format(noonce, content))
        text = self.decrypt(content, noonce).decode()
        print("texte déchiffré = ", text)
        return text

    def send(self, msg):
        print("=== send ===")
        content = self.server.name.ljust(30) + msg
        print("content = ", content)
        encrypted_content, salt = self.encrypt(content)
        print("encrypted content = ", content)
        self.request.sendall(salt + encrypted_content)
    

if __name__ == "__main__":
    HOST, PORT = 'localhost', 999
    server = MyServer((HOST, PORT), MyTCPHandler)
    server.serve_forever()
    server.server_close()