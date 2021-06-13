
# TODO....

exit -1


# Gather these files:
/build/root-pi0w-dev/images/rpi-firmware/overlays
/build/root-pi0w-dev/images/rpi-firmware/bootcode.bin
/build/root-pi0w-dev/images/rpi-firmware/fixup.dat
/build/root-pi0w-dev/images/rpi-firmware/start.elf
/build/root-pi0w-dev/images/bcm2708-rpi-zero-w.dtb
/build/root-pi0w-dev/images/rootfs.squashfs
/build/root-pi0w-dev/images/zImage
/build/supervisor.pi0wdbg.exec
/build/shed3.pi0wdbg.exec


Generate these files:


config.txt:
-------------------------
boot_delay=0
kernel=zImage
disable_overscan=1
gpu_mem_256=50
gpu_mem_512=50
gpu_mem_1024=50
dtoverlay=miniuart-bt
dtoverlay=pwm-2chan
--------------------------


cmdline.txt
--------------------------
root=/dev/mmcblk0p2 rootwait console=tty1 -- /dev/mmcblk0p1 rootfs.squashfs
--------------------------


shed-config
--------------------------
# This is the email address that notifications are sent from:
EmailAddress: example@example.com
# SMTP Server for outgoing mail:
EmailServer: smtps://smtp.example.com:465
# SMTP Server credentials:
EmailUserPass: example-user:example-password
# Destication email address where notifications are sent to:
DestinationEmailAddress: example@example.com
# Url where configuration is pulled from:
ConfigURL: https://example.com/config
# Seconds after midnight when the space opens (8 AM)
OpenAtSec: 28800
# Seconds after midnight when the space closes (8 PM)
CloseAtSec: 72000
--------------------------



TODO:
autoexec.sh
supervisor.config
wpa_supplicant.conf
