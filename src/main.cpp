#include <SSHServer.hpp>

#include <print>

const char* hostkey_path = "ssh_host_rsa_key";
const int PORT = 2200;

int main() {
    auto server = std::make_shared<sshGame::SSHServer>("0.0.0.0", PORT, hostkey_path);
    server->bindListen();
    server->listenForConnections();
}

