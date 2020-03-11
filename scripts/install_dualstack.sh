echo "install isc-dhcpc-server"
sudo apt-get -y install isc-dhcp-server

echo "install router advertisement daemon service"
sudo apt-get -y install radvd

echo "install bind9(DNS Server)"
sudo apt-get -y install bind9 bind9utils bind9-doc

echo "install apache web server"
sudo apt-get -y install apache2
