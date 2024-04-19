#!/bin/bash

# Function to install Boost
install_boost() {
    echo "Installing Boost..."
    wget -O boost_1_81_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.81.0/boost_1_81_0.tar.gz/download
    tar xzvf boost_1_81_0.tar.gz
    cd boost_1_81_0/
    ./bootstrap.sh --prefix=/usr/
    ./b2
    sudo ./b2 install
    cd ..
    echo "Boost installed successfully."
}

# Function to install GMP
install_gmp() {
    echo "Installing GMP..."
    wget https://gmplib.org/download/gmp/gmp-6.2.1.tar.xz
    tar xJvf gmp-6.2.1.tar.xz
    cd gmp-6.2.1
    ./configure
    make
    make check
    sudo make install
    cd ..
    echo "GMP installed successfully."
}

# Function to install NTL
install_ntl() {
    echo "Installing NTL..."
    wget https://libntl.org/ntl-11.5.1.tar.gz
    gunzip ntl-11.5.1.tar.gz
    tar xf ntl-11.5.1.tar
    cd ntl-11.5.1/src
    ./configure
    make
    make check
    sudo make install
    cd ../..
    echo "NTL installed successfully."
}

# Main script to install all libraries
echo "Installation script started."

install_boost
install_gmp
install_ntl

echo "All libraries installed successfully."