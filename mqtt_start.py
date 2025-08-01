from meshtastic import serial_interface
from meshtastic.util import findPorts
from pubsub import pub
import json
import time

print("Starting up...", flush=True)

def onReceive(packet, interface):
    serializable_packet = {
        "from": packet.get("from"),
        "to": packet.get("to"),
        "id": packet.get("id"),
        "rxTime": packet.get("rxTime"),
        "rxSnr": packet.get("rxSnr"),
        "rxRssi": packet.get("rxRssi"),
        "hopLimit": packet.get("hopLimit"),
        "hopStart": packet.get("hopStart"),
        "fromId": packet.get("fromId"),
        "toId": packet.get("toId"),
        "decoded": {
            "portnum": packet.get("decoded", {}).get("portnum"),
            "text": packet.get("decoded", {}).get("text"),
            "bitfield": packet.get("decoded", {}).get("bitfield"),
            "latitude": packet.get("decoded", {}).get("latitude"),
            "longitude": packet.get("decoded", {}).get("longitude"),
            "altitude": packet.get("decoded", {}).get("altitude"),
            "batteryLevel": packet.get("decoded", {}).get("batteryLevel")
        }
    }
    print(json.dumps(serializable_packet), flush=True)

ports = findPorts()
if not ports:
    print("No Meshtastic device found!", flush=True)
    exit(1)

print(f"Found ports: {ports}", flush=True)
print(f"Connecting to {ports[0]}...", flush=True)
try:
    interface = serial_interface.SerialInterface(ports[0])
    pub.subscribe(onReceive, "meshtastic.receive")
    print("Connected! Waiting for messages...", flush=True)
    while True:
        time.sleep(1)
except Exception as e:
    print(f"Error occurred: {e}", flush=True)