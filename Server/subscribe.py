#this script subscribes to the lcoal MQTT server on the topic "P" and saves all incoming messages to
# /var/log/gpstrace

# a transformation on the format of the GPS coordinates takes place before the values are stored
# the prefered format used on the server is the decimal degrees dd. The transformation dd = d + mm.mm/60 
# the values sent by the GPS tracker  follow this format ddmm.mm 

import paho.mqtt.client as mqtt
import math
import os
# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("P")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    S = msg.payload.split(",")
    A = [0.0,0.0]
    A[0] = float(S[0])
    A[1] = float(S[1])
    A[0]=math.floor(A[0]/100)+(A[0]/100-math.floor(A[0]/100))*5/3
    A[1]=math.floor(A[1]/100)+(A[1]/100-math.floor(A[1]/100))*5/3
    
    if os.path.exists("/var/log/gpstrace"):
        #print("file exist\n")
        f = open("/var/log/gpstrace", 'rb+')
        f.seek(-1, os.SEEK_END)
        f.truncate()
        f.close()
        f = open("/var/log/gpstrace", 'a')
        f.write(",[%.8f,%.8f]\n]" % (A[1],A[0]))
        f.close()
    else:
        #print("File does not exist. file was created now\n")
        f = open("/var/log/gpstrace", 'a')
        f.write("[\n[%.8f,%.8f]\n]" % (A[1],A[0]))
        f.close()
    

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("localhost", 1883, 60)

client.loop_forever()