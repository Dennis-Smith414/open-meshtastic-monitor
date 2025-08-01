#!/bin/bash
echo "starting meshtastic venv"
echo "RUN WITH source ./mesh.sh"
source ~/meshtastic_venv/bin/activate
#dont hard code this
sudo chmod 666 /dev/ttyUSB0
##python mqtt_start.py | ./parce 
python mqtt_start.py
