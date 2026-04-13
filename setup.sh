#!/bin/bash
# ---------------------------------------------------------
# Team 15: Space Station Environment One-Click Setup Script (2026-04-12)
# ---------------------------------------------------------

echo "Step 1: Installing low-level development dependencies..."
sudo apt-get update
sudo apt-get install -y libasound2-dev cmake g++ bluetooth bluez libfftw3-dev

echo "Step 2: Configuring hardware access permissions..."
# Allow user to directly read/write display buffer, touch screen, and SPI interface
sudo usermod -aG video $USER
sudo usermod -aG input $USER
sudo usermod -aG spi $USER

echo "Step 3: Enabling user session lingering..."
# Key: Resolves the bug where Bluetooth audio disconnects automatically after SSH exit
sudo loginctl enable-linger $USER

echo "Step 4: Creating build directory..."
mkdir -p build

echo "Configuration complete! Please log in again to the Raspberry Pi or run 'newgrp video', then you can start compiling."
# Added to the end of the setup.sh file
echo "----------------------------------------------------"
echo "Tip: System environment is ready."
echo "Please ensure your JBL Bluetooth speaker is in pairing mode, then manually execute:"
echo "bluetoothctl"
echo "In the console, enter: pair [Your Speaker MAC], trust [Your Speaker MAC], connect [Your Speaker MAC]"
echo "----------------------------------------------------"
