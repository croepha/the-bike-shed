SHED: Secure-ish Hacker Entry Device
================================================================================

An RFID based access control system for [Noisebridge].  Some specific goals:
- A community based educational project, try to expose people to embedded systems programming and design
- Be mostly like the current system (EARL)
- But more reliable, designed to run for 5 years without maintenance
  - Limit run time dependancies
  - Limit complexity
  - Unit and Functional testing
- More accessable management, the plan is that the master config will just be pulled from a URL, like a github hosted file for example...
- support multiple semi-autonomous units (currently unmet)
- possibly be adaptable to other applications like appliance lockouts, or lockers (currently unmet)

[Noisebridge]: https://www.noisebridge.net/


Install
--------------------------------------------------
- Build the Interior and Exterior modules found in the schematics directory (We will work on an easy PCB you can have made)
- Download a release zip, (from Github releases, should be a file named like `release-12341234.zip`)
- Extract it to an SD Card formatted as FAT (most come this way)
- Read the `README-release.txt` file that you just extracted
- Install the SD Card in the PI0, in the interior module
- Use Arduino studio to build the exterior code, flash that to the ESP32 in the exterior module

Development
--------------------------------------------------
### Dev Quickstart:

For best results, listen to this on repeat: https://www.youtube.com/watch?v=T1CowKULMx8

For general coding:
0. Download and install Virtualbox (or similar)
1. Setup a Ubuntu 20.04 VM
    - Hint: Deselect "Set up this disk as an LVM group" so that it just gives you the whole disk
    - ubuntu-server is reccomended, it will be a lighter install, and we dont need the GUI
    - 40 GB should be plenty
    - 4GB+ of RAM
    - Don't forget to give it a few CPUs if you want compiles to be faster
    - intall openssh with SSH, setup login for root
        - Set networking options to use bridged networking
        - `sudo apt install openssh-server` Or select (Install OpenSSH server during ubuntu install)
        - `ip addr # print ip address`
        - You should now at-least be able to ssh in as your non-root user
        - `sudo passwd root # set a root password`
        - `echo "PermitRootLogin yes" | sudo tee -a /etc/ssh/sshd_config # Appends option to end of file`
        - `sudo systemctl restart sshd`
        - Pro tip, add a section like this to your local `~/.ssh/config`

              Host shed-dev-vbox
                Hostname 1.2.3.4 # Relplace with your virtualbox ip address
                User root
                ForwardAgent yes  # Enables ssh agent forwarding, which makes
                                  # accessing github over ssh easier

          Then you should be able to do `ssh shed-dev-vbox`

    - Setup SAMBA for remote file access, useful for using KiCad or Arduino Studio on your local workstation, also makes it easier to copy files out
        - `sudo apt install samba`
        - edit `/etc/samba/smb.conf` Add an entry at the end like this at the bottom:

              [root_samba]
                  browseable = yes
                  read only = no
                  writable = yes
                  create mask = 0755
                  directory mask = 0755
                  path = /

        - `sudo smbpasswd root -a # Set a root password for SMB access`
        - Now you should be able to easilly access the VM's files directly over SMB, login as root with the password you just set

2. Log in as root (you will be doing everything as root) [VSCode] is reccomended (but not required) for [remote work over SSH]
3. `mkdir /workspaces && git clone https://github.com/croepha/the-bike-shed.git /workspaces/the-bike-shed && cd /workspaces/the-bike-shed` (optionally open a vscode workspace there)
4. `bash setup_dev_environment.bash` (or run the "setup dev environment" vscode task)
5. To build the code and run the tests: `bash build.bash` (or the build task in vscode) Note the first
   time you run the build, you might get an error like this:  `fatal error: '/build/parse_headers.re.c' file not found`
   Just run the build again, currently we aren't telling ninja about some files were generating, this only effects first time builds.

The "clangd" extension for VSCode is highly reccomended

Hopefully that just worked, if it didn't, please let us know

For Schematics, you should also grab [KiCad].  Currently, the exterior code has a hard requirement on Arduino Studio, and it doesn't matter where you run that

[VSCode]: https://code.visualstudio.com/
[remote work over SSH]: https://code.visualstudio.com/docs/remote/ssh
[KiCad]: https://www.kicad-pcb.org/

## Dev setup FAQ:

### Q: Do I really need to run inside of Virtualbox?

A: No, not really, you can setup on bare metal or run inside docker with success depending on your
exact setup.  It is encouraged to not run setup_dev_environment.bash on your daily driver system,
as it does some things that may be considered rude, such as installing system packages, and writing
things to random directories within the system.  Mixed results have been seen with Docker Desktop,
on some setups there are issues setting the core_pattern or other low level systemy things.

### Q: Why does everything run as root?

A: Well its already in a VM, and its just easier that way... In actuality, there are only a few things
that really need root, mostly the tests, like some call iptables to simulate various network situations,
sometimes chroot or mount is called.  We could fix this if we wanted, but as far as I can tell it would
be more work, that doesn't actually solve a problem, but if you care, then please feel free empowered to
fix it yourself

### Q: Why fixed paths, or: Do I really have to put my code in `/workspaces/the-bike-shed`?

A: It's mostly out of lazyness at this point, we might fix this if there is interest.  It's a similar
situation to "Why does everything run as root".  Everything is already inside a container, why does it matter
where it is inside of that?  There are some actual advantages, albeit minor:
  - Fixed paths make debugging easier, if something is always in the same place, you don't have to go hunt for it
  - Collaberation is easier, homogenous environments make it easier to catch people up on eachother's envrironments, and you can copy/paste commands...


### Q: This doesn't build everything does it?

A: You probably didn't ask that question, but I asked it for you.  No, the quickstart assumes that you aren't interested in
building the kernel or the root OS.  Most people won't need to, they can just copy from the latest release, but if you _are_
interested, then see `build_root.bash`.  If you actually did want to build _everything_ from scratch then do this:
 - `bash build_root.bash get_buildroot`
 - `bash build_release.bash`
 - The release binaries should now be in `/build/release`



Everything below this line is probably out of date TODO
--------------------------------------------------


Hardware List
--------------------------------------------------

This list is a work in progress.

- [Raspberry PI Zero W][pi0w]
- PI0 GPIO keypad (`@TODO` what type? link?)
- PI0 GPIO LCD 2x16 Char display (`@TODO` what type? link?)
- SD Card:
  - Candidate: [AMLC]
- Communication (See `TODO -> Hardware Research`)
- Mains Power supply (See `TODO -> Hardware Research`)
- Solenoid / electric strike (See `TODO -> Hardware Research`)
- Internal 5v regulator (See `TODO -> Hardware Research`)
- RFID:
  - Candidate: [PN7462]
  - Candidate: [MFRC522]


[MFRC522]: https://www.google.com/search?q=mfrc522
[PN7462]: https://discuss.noisebridge.info/t/pn7462-nfc-deep-dive/1712
[AMLC]: https://www.digikey.com/products/en/memory-cards-modules/memory-cards/501?k=AMLC
[pi0w]: https://www.raspberrypi.org/products/raspberry-pi-zero-w/

TODO
--------------------------------------------------

Items below are either incomplete (☐) or complete (☑). Skipped items
are <strike>striked out</strike>.


### Code

- ☐ Create skeleton code for shed
  - ☐ async curl
  - ☐ p2p UDP packets...
  - ☐ Periodic autosave
- ☐ Testing framework

- ☐ non vscode directions for building

- ☐ prototype code for PI0 GPIO keypad

- ☐ prototype code for PI0 GPIO LCD 2x16 Char display
  - ☐ memfd device file...

- ☐ LIBCURL grab master-config from server

- ☐ Parse master config

- ☐ Trade config with peers

- ☐ Synchonize time?
  - Do it with the time http headers when we get master-config?

- ☐ Crypto all the things
  - ☐ sign p2p configs? have peer keys in master-config?
  - ☐ [argon2d][a2d] for RFID serials
  - ☐ Encrypt the list of IPs in the master-config

- ☐ Make buildroot config
- ☐ Change from using NTP to something else... NTP has some security issues
- ☐ Enable hardware watchdog: https://diode.io/raspberry%20pi/running-forever-with-the-raspberry-pi-hardware-watchdog-20202/

[a2d]: https://en.wikipedia.org/wiki/Argon2


### Hardware

- ☐ Make schematics for exterior board.
- ☐ SDCARD leveling torture test, need to make sure that the SDcard
  will last for the lifetime, 30 new accesses per day * 365 days * 5
  years should be plenty.

### Hardware Research

- ☐ Research into ATMEL - RPI long distance/cheap/reliable
  communication...
    - TTL? [MAX232][max232]? MAX485?
    - Research reliabiliy options like parity? or CRC?
- ☐ Research into solenoid/electric strike, need to pin down voltage
  - Candidate: [(DC12V) Failure Secure ANSI Standard Heavy Duty Electric Strike Lock][lock]
- ☐ Power mosfet
  - Article: [Byte and Switch (Part 1)][bas]
  - Candidate: [DMG2302UK][DMG2302UK]
- ☐ Mains Power supply:
  - Canidate: [RS-25-12][rs25] (Probably is overkill?)
- ☐ Internal 5v regulator:
  - StackExchange: [Powering a PI from 12v][se12v]

[DMG2302UK]: https://www.diodes.com/assets/Datasheets/DMG2302UK.pdf
[max232]: https://en.wikipedia.org/wiki/MAX232
[lock]: https://www.amazon.com/dp/B01MXZ0EKQ
[bas]: https://www.embeddedrelated.com/showarticle/77.php
[rs25]: https://www.digikey.com/product-detail/en/mean-well-usa-inc/RS-25-12/1866-4140-ND/7706175
[se12v]: https://raspberrypi.stackexchange.com/a/19964


Streatch Goals (TODOs for after MVP)
--------------------------------------------------

- ☐ Research alternate auth mechanisms:
  - Just using serials from NFC has a few security issues:
    - Easily skimmed
    - Small address space, trivially brute-forced, especially on clippercards
  - Possible Solutions:
    - Use bigger (1k) payloads
    - Use Challenge response NFC
    - Use BLE (phone?)


Temporary Protoyping parts list: ( Just some notes of parts im considering buying )
--------------------------------------------------

https://www.digikey.com/product-detail/en/MAX489CPD%2b/MAX489CPD%2b-ND/948035


#### Interconnect link between interrior and exterior
RJ45, designed to use a straight through 568B/568B Cable.  With some minimal
effort not to blow out ethernet if there is some user error (no gaurentees)

    GREEN           EXTERIOR+DATA   PIN6    |
    GREEN-STRIPE    EXTERIOR-DATA   PIN3    |
    ORANGE          INTERIOR+DATA   PIN2    |
    ORANGE-STRIPE   INTERIOR-DATA   PIN1    |
    BLUE            INTERIOR+12v    PIN4    |
    BLUE-STRIPE     INTERIOR+12v    PIN5    |
    BROWN           INTERIOR-GND    PIN8    |
    BROWN-STRIPE    INTERIOR-GND    PIN7    |

Same list sorted by pins

    ORANGE-STRIPE   INTERIOR-DATA   PIN1    |
    ORANGE          INTERIOR+DATA   PIN2    |
    GREEN-STRIPE    EXTERIOR-DATA   PIN3    |
    BLUE            INTERIOR+12v    PIN4    |
    BLUE-STRIPE     INTERIOR+12v    PIN5    |
    GREEN           EXTERIOR+DATA   PIN6    |
    BROWN-STRIPE    INTERIOR-GND    PIN7    |
    BROWN           INTERIOR-GND    PIN8    |



#### Why does the build need Docker?

Good question.  Because of the nature of this project we need a number of
compilers and realted tools installed, doing this via docker is a simple
way to have a consistent build environment for anyone who wants to develop

More detail:
- We're using buildroot to build the custom linux base system, that by itself
  requires a number of random dependencies
- We are doing a bunch of testing, I wanted an easy way to mock out SMTP and WEB services
  so this means installing some python things and installing nginx

I don't like it:
  Have you actually tried it? its super easy to get setup...
But... I still don't like it:
  PRs welcome, but please don't break existing workflows


