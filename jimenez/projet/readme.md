Lancement du serveur :

    cd serveur
    python server.py

Lancement du client :

    cd client
    python client.py  

Le client devrait s'identifier comme l'utilisateur "Jean" dont le mot de passe est "mdpjean". Le client et le serveur peuvent échanger des messages chiffrés (AES256-GCM). L'identité du client est vérifiée par le serveur selon l'authentification par challenge. Les messages reçus et envoyés par le serveur sont stockés chiffrés dans la base de données.

La seule librairie non standard utilisée est pyca/cryptography qui est une librairie de cryptographie. Son installation se fait via pip :  

    python -m pip install cryptography
