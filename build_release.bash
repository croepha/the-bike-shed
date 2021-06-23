set -eEuo pipefail

bash build_root.bash clean_all
bash build_root.bash build_all
bash build_root.bash package_sdk
SHOULD_CLEAN=1 bash build.bash
bash build_root.bash build_linux

rm -rf /build/release
mkdir -p /build/release/debugging

cp /build/supervisor.pi0wfast.exec /build/release/debugging/
cp /build/supervisor.pi0wfast.exec /build/release/supervisor.exec
/build/root-pi0w-dev-sdk/host/arm-buildroot-linux-uclibcgnueabihf/bin/strip /build/release/supervisor.exec

cp /build/shed3.pi0wfast.exec /build/release/debugging/
cp /build/shed3.pi0wfast.exec /build/release/shed.exec
/build/root-pi0w-dev-sdk/host/arm-buildroot-linux-uclibcgnueabihf/bin/strip /build/release/shed.exec

cp -r \
/build/root-pi0w-dev/images/rpi-firmware/overlays \
/build/root-pi0w-dev/images/rpi-firmware/bootcode.bin \
/build/root-pi0w-dev/images/rpi-firmware/fixup.dat \
/build/root-pi0w-dev/images/rpi-firmware/start.elf \
/build/root-pi0w-dev/images/bcm2708-rpi-zero-w.dtb \
/build/root-pi0w-dev/images/rootfs.squashfs \
/build/root-pi0w-dev/images/zImage \
/build/release/

cat << 'EOF' > /build/release/config.txt
boot_delay=0
kernel=zImage
disable_overscan=1
gpu_mem_256=50
gpu_mem_512=50
gpu_mem_1024=50
dtoverlay=miniuart-bt
dtoverlay=pwm-2chan
EOF

echo "root=/dev/mmcblk0p2 rootwait console=tty1 -- /dev/mmcblk0p1 rootfs.squashfs" > /build/release/cmdline.txt

cat << 'EOF' > /build/release/shed.config
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
EOF

cat << 'EOF' > /build/release/supervisor.config
# This is the email address that notifications are sent from:
EmailAddress: example@example.com
# SMTP Server for outgoing mail:
EmailServer: smtps://smtp.example.com:465
# SMTP Server credentials:
EmailUserPass: example-user:example-password
# Destication email address where notifications are sent to:
DestinationEmailAddress: example@example.com

DebugSupervisorArg: /mnt/physical/shed.exec
DebugSupervisorArg: /mnt/physical/shed.config
EOF

cat << 'EOF' > /build/release/wifi_action.sh
#!/bin/sh
if [ "$2" = "CONNECTED" ]; then
    ip addr add 1.2.3.4/24 dev wlan0 # Set static IP address
    ip route add default via 1.2.3.1 # Set static default route
    echo "nameserver 8.8.8.8" > /etc/resolv.conf # Use Google DNS servers
fi
EOF

cat << 'EOF' > /build/release/wpa_supplicant.conf
update_config=1
ctrl_interface=/var/run/wpa_supplicant
ap_scan=1

network={
    key_mgmt=WPA-PSK
    ssid="Some WPA network"
    psk="Some Passphrase"
}
network={
   key_mgmt=NONE
   ssid="Some open entwork"
}
EOF

cat << 'EOF' > /build/release/autoexec.sh
haveged
iptables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
iptables -A INPUT -i wlan0 -j DROP # comment to disable firewall

date --set "$( date -r /mnt/physical/shed-config '+%F %T' )"
/mnt/physical/supervisor.exec /mnt/physical/supervisor.config 2>&1 | tee -a /var/log/supervisor.log &

hostname shed-unconfigured
modprobe brcmfmac
wpa_supplicant -B  -i wlan0 -c /mnt/physical/wpa_supplicant.conf
wpa_cli -Ba /mnt/physical/wifi_action.sh
ntpd -p 0.us.pool.ntp.org -p 1.us.pool.ntp.org -p 2.us.pool.ntp.org -p 3.us.pool.ntp.org

# /usr/sbin/telnetd -l sh # Uncomment to enable shell access via telnet

while true; do {
    echo -e "\n\n\n\n\n"
    date
    ip link
    echo
    ip addr
    sleep 5
}; done
EOF

(
echo "Git HEAD: $(git rev-parse HEAD)"
echo "Git Status:"
git status
echo
find /build/release -type f -exec sha384sum {} +
) > /build/release/debugging/info

( cd /build/release; zip -r /build/release-$(date +%s).zip .; )

exit -1

# QA Check list:
# - can ping it
# - basic badge in test

