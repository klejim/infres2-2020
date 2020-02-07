from cryptography.hazmat.primitives.ciphers.aead import AESGCM

import json
import os

def read_config(filename):
    conf = None
    try:
        conf = json.load(open(filename, "r"))
    except (json.JSONDecodeError, IOError) as err:
        print("Erreur dans le fichier cong : ", err)
    finally:
        return conf

class BaseApp:
    def __init__(self, *args):
        self.log_file = open("log.txt", "a")
        self.name = ""
        self.key = b'6\x0c{\xd0l-?o`\xf0\xe5\xf0\xd2\xad\x90\xa3$\x08Xb&\x95\xb5\x07y;\xc0\x15\x10\xac\x97\x13'
        self.aesgcm = AESGCM(self.key)

    def log(self, *args, **kwargs):
        print(*args, **kwargs, file=self.log_file)

    def encrypt(self, data):
        salt = os.urandom(12)
        ct = self.aesgcm.encrypt(salt, data, b'')
        return ct, salt

    def decrypt(self, data, noonce):
        return self.aesgcm.decrypt(noonce, data, b'')

    def send(self, msg, sckt):
        self.log("=== send ===")
        if isinstance(msg, str):
            msg = msg.encode()
        name = self.name.encode().ljust(30)
        content = name + msg
        self.log("content = ", content)
        encrypted_content, noonce = self.encrypt(content)
        self.log("content chiffré = ", encrypted_content)
        sckt.sendall(noonce + encrypted_content)
        

    def receive(self, sckt):
        answer = sckt.recv(1024)
        self.log("===receive===")
        # les douze premiers octets sont le nonnce
        nonnce, content = answer[:12], answer[12:]
        self.log("nonnce = {}, content = {}".format(nonnce, content))
        text = self.decrypt(content, nonnce)
        self.log("texte déchiffré = ", text)
        return text