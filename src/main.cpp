#include <SSHServer.hpp>

#include <print>

int main(int argc, char* argv[]) {
    
    if (argc < 3) {
	std::println("Not enough arguments provided: port, host key path");
	return -1;
    }

    int arg_port = std::stoi(argv[1]);
    char * key_path = argv[2];


    auto server = std::make_shared<sshGame::SSHServer>("0.0.0.0", arg_port, key_path);
    server->bindListen();
    server->listenForConnections();
}

