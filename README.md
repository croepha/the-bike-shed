# the-bike-shed




## Getting started on Development using VSCode
0. For best results, listen to this on repeat: youtu.be/YUvOeN1zNYg
1. Install Visual Studio Code
2. Install and setup docker desktop
3. Install the "Remote - Containers" (`ms-vscode-remote.remote-containers`)  exension
4. Clone this code locally ex: `cd ~/my_projects/ && git clone --depth=1 git@github.com:croepha/the-bike-shed.git`
5. Open the project folder in VSCode
6. It should prompt to "Reopen in Container", do that. else do Command Pallette -> "Reopen in Container"
  - Note: You can ignore the message about git missing, it will get installed in the next step
7. Once it's loaded in the new container run the "setup dev environment" task. Command Pallette -> "Run Task"... On a fresh system that might take a while, go-ahead and install the extensions below
8. Add these extensions inside the Docker workspace:
  - clangd (llvm-vs-code-extensions.vscode-clangd)
9. Run the default build task to build...



#### TODO:
- create skeleton code for shed
  - async curl
  - p2p UDP packets...
  - Periodic autosave
- testing framework
- non vscode directions for building?
- prototype code for PI0 GPIO keypad
- prototype code for PI0 GPIO LCD 2x16 Char display
  - memfd device file...

- Research into ATMEL - RPI long distance/cheap/reliable communication...
  - TTL? MAX232?

- LIBCURL grab master-config from server
- Parse master config
- Trade config with peers

- Synchonize time?
  - Do it with the time http headers when we get master-config?

- Crypto all the things
  - sign p2p configs? have peer keys in master-config?
  - argon2d for
  - Encrypt the list of IPs in the master-config

- Make buildroot config
- Make schematics for exterior board
