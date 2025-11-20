# Interactive ssh server made in c++23 with Libssh  
Currently displays a centered ANSI art image when you connect to the server. By design it does not have any authentication.  
  
Goal for this is to be used as a base for an interactive ssh application, for example a game. 
<script src="https://asciinema.org/a/pv6lnMsLalF62kwH5szdSzE6M.js" id="asciicast-pv6lnMsLalF62kwH5szdSzE6M" async="true"></script>
## Requirements
- Libssh 
- CMake
## How to build
Run ``make`` and ``make build``
## How to run
To run the ssh server you have to first create a server key (Example of creating one):  
```sh
ssh-keygen -t rsa -b 2048 -f ssh_host_rsa_key
```
  
After you have the key, you can run the build executable in the build folder:  
```sh
./sshGame 2200 ssh_host_rsa_key 
```

First argument is the port to run the ssh server on, and the second is the previously generated private key path.  
  
You can then connect to the server with:
```sh
ssh -p 2200 0.0.0.0
```
Or you can ommit the -p if you set the port of the server to 22.  
The server will not ask for any authentication and should just display an ANSI art image.
