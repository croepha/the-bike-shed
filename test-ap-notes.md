
This was a failure, ended up just going with a normal physical AP


Installed Ubuntu Server 20.04 in virtualbox
disabled audio, setup USB
attached USB WIFI adapter to VM
ssh-copy-id
visudo add NOPASSWD:

do upgrades, reboot


cat << EOF | sudo tee /etc/hostapd/hostapd.conf > /dev/null
interface=wlx00c0ca902f8a
bridge=br0
logger_syslog=-1
logger_syslog_level=0
logger_stdout=0
logger_stdout_level=4
ssid=test-ap
country_code=US
hw_mode=g
channel=1
wpa=2
wpa_passphrase=secret-passphrase
EOF

sudo systemctl unmask  hostapd.service
sudo systemctl enable  hostapd.service
sudo systemctl restart  hostapd.service

cat << EOF | sudo tee /etc/netplan/00-installer-config.yaml > /dev/null
network:
  ethernets:
    enp0s3:
      dhcp4: false
    wlx00c0ca902f8a:
      dhcp4: false
  bridges:
    br0:
      dhcp4: true
      interfaces:
        - enp0s3
        - wlx00c0ca902f8a
  version: 2
EOF
sudo netplan try




