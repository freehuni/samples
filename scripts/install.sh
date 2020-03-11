sudo apt-get -y install ssh
sudo apt-get -y install subversion
sudo apt-get -y install git
sudo apt-get -y install valgrind
sudo apt-get -y install meld
sudo apt-get -y install gtkterm
sudo apt-get -y install gupnp-tools
sudo apt-get -y install shutter
#sudo apt-get -y install doxygen
#sudo apt-get -y install doxygen-gui
sudo apt-get -y install docker.io

# to run 32bit executable binary on 64bit machine.
sudo apt-get -y install lib32ncurses5 lib32z1
sudo apt-get -y install lib32ncurses5-dev
sudo apt-get -y install build-essential gcc-multilib g++-multilib
sudo apt-get -y install cmake
sudo apt-get -y install qt5-default
sudo apt-get -y install libz-dev

# android
sudo apt install qemu-kvm
grep kvm /etc/group
sudo adduser $USER kvm
sudo chown $USER /etv/kvm

# Wireshark
sudo apt-get -y install -y wireshark-qt
sudo apt-get -y install -y wireshark-gtk
sudo chgrp $USER /usr/bin/dumpcap
sudo chmod 777 /usr/bin/dumpcap
sudo setcap cap_net_raw,cap_net_admin+eip /usr/bin/dumpcap

echo 'setup serial port configuration'
groups  $USER
sudo gpasswd --add $USER dialout
sudo usermod -a -G dialout $USER

# tftp
echo 'setup tftp...'
sudo apt-get -y install xinetd tftp tftpd
sudo sh -c "echo 'service tftp' 									>  /etc/xinetd.d/tftp"
sudo sh -c "echo '{' 												>> /etc/xinetd.d/tftp"
sudo sh -c "echo '    socket_type     = dgram'      				>> /etc/xinetd.d/tftp"
sudo sh -c "echo '    protocol        = udp'        				>> /etc/xinetd.d/tftp"
sudo sh -c "echo '    wait            = yes'        				>> /etc/xinetd.d/tftp"
sudo sh -c "echo '    user            = root'       				>> /etc/xinetd.d/tftp"
sudo sh -c "echo '    server          = /usr/sbin/in.tftpd'		>> /etc/xinetd.d/tftp"
sudo sh -c "echo '    server_args     = -s /tftpboot' 			>> /etc/xinetd.d/tftp"
sudo sh -c "echo '    disable         = no'         				>> /etc/xinetd.d/tftp"
sudo sh -c "echo '    per_source      = 11'         				>> /etc/xinetd.d/tftp"
sudo sh -c "echo '    cps             = 100 2'      				>> /etc/xinetd.d/tftp"
sudo sh -c "echo '    flags           = IPv4'       				>> /etc/xinetd.d/tftp"
sudo sh -c "echo '}' 												>> /etc/xinetd.d/tftp"

sudo mkdir /tftpboot
sudo chmod 777 /tftpboot
sudo /etc/init.d/xinetd restart

#samba
echo 'setup samba...'
sudo apt-get -y install samba
sudo smbpasswd -a $USER

sudo sh -c "echo '['$USER']' 				>> /etc/samba/smb.conf"
sudo sh -c "echo 'path = '$HOME 			>> /etc/samba/smb.conf"
sudo sh -c "echo 'read only = no' 			>> /etc/samba/smb.conf"
sudo sh -c "echo 'valid users = '$USER 	>> /etc/samba/smb.conf"
sudo sh -c "echo 'browseable = yes' 		>> /etc/samba/smb.conf"
sudo /etc/init.d/smbd restart

# NFS
echo 'setup nfs...'
sudo apt-get -y install nfs-common nfs-kernel-server
sudo sh -c "echo '/nfsroot        *(rw,no_root_squash,no_all_squash)' >> /etc/exports"
sudo mkdir /nfsroot
sudo chmod 777 /nfsroot
sudo /etc/init.d/nfs-kernel-server restart

# SUDOERS
sudo sh -c "echo $USER 'ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers"

sudo apt-get update

echo "Installation of build server succeeded!"


