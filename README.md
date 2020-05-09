# the-bike-shed




## Getting started on Development using VSCode
0. For best results, listen to this on repeat: https://youtu.be/T1CowKULMx8
1. Install Visual Studio Code
2. Install and setup docker:
  - Windows & MAC install docker desktop
  - Ubuntu: `sudo apt install docker.io; sudo usermod -aG docker $USER` re-login
  - Linux follow directions for your distro...
3. Install the "Remote - Containers" (`ms-vscode-remote.remote-containers`)  exension
4. Clone this code locally ex: `cd ~/my_projects/ && git clone --depth=1 git@github.com:croepha/the-bike-shed.git`
5. Open the project folder in VSCode
6. It should prompt to "Reopen in Container", do that. else do Command Pallette -> "Reopen in Container"
  - Note: You can ignore the message about git missing, it will get installed in the next step
7. Once it's loaded in the new container run the "setup dev environment" task. Command Pallette -> "Run Task"
8. Re-open window (so that vscode will realize git is now installed)
9. Run the default build task to build...
	-To clean and rebuild:  uncomment line 44 and then comment out lines 45 & 47 of build.bash. Save and run build task again. Then undo and run build task again



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

- Hardware research:
    - Research into ATMEL - RPI long distance/cheap/reliable communication...
        - TTL? MAX232? MAX485?
    - Research into solenoid/electric strike, need to pin down voltage
      - Candidate: https://www.amazon.com/Failure-Standard-Electric-American-Holding/dp/B01MXZ0EKQ
    - Power mosfet
      - Candidate: 
    - Power supply:

- LIBCURL grab master-config from server
- Parse master config
- Trade config with peers

- Synchonize time?
  - Do it with the time http headers when we get master-config?

- Crypto all the things
  - sign p2p configs? have peer keys in master-config?
  - argon2d for RFID serials
  - Encrypt the list of IPs in the master-config

- Make buildroot config
- Make schematics for exterior board
- SDCARD leveling torture test, need to make sure that the SDcard will last for the lifetime, 30 new accesses per day * 365 days * 5 years should be plenty

