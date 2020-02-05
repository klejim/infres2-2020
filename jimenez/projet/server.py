import socketserver

server_name = input("Server name : ")

class MyTCPHandler(socketserver.BaseRequestHandler):
    def handle(self):
        self.data = self.request.recv(1024).decode()
        client_name, client_msg = self.data[0:30].strip(), self.data[30:].strip()
        client_display = "{}({})".format(client_name, self.client_address[0])
        print("{} : {}".format(client_display, client_msg))
        msg = input(">>> ")
        answer = server_name.ljust(30) + msg
        self.request.sendall(answer.encode())


if __name__ == "__main__":
    HOST, PORT = 'localhost', 999
    server = socketserver.TCPServer((HOST, PORT), MyTCPHandler)
    server.serve_forever()
    server.server_close()