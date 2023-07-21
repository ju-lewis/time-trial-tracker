'''

    Raspberry Pi GPS read and upload test

'''
import serial
import os
import socket

HOST = "127.0.0.1"
PORT = 8765

#Configure the serial port
port = serial.Serial('/dev/ttyUSB1', baudrate=57600, timeout=0.2)

def DM_to_DEG(lat, lon):
    '''Takes lat, lon as str returns deg str'''

    lat_integer_degrees = int(lat[0:2])
    lat_minutes = float(lat[2:])



    lon_integer_degrees = int(lon[0:3])
    lon_minutes = float(lon[3:])


    return lat_integer_degrees + (lat_minutes / 60), lon_integer_degrees + (lon_minutes / 60)


def log_run():
    
    sentence = b""
    # Clear gps_log.txt
    fp1 = open("gps_log.txt", "w")
    fp1.close()
    
    while True:
    # Read incoming data
    data = port.read()
    
    #Check if any data is available
    if data and data != b'\n' and data != b'\r':
        sentence += data
    elif data == b'\n':
        # Sentence complete
        sentence = sentence.decode()
        data_list = sentence.split(",")
    
        #Ignore everything except main positioning data
        if data_list[0] != "$GNRMC":
            #pass
            sentence = b""
            continue
    
        #Skip if position hasn't been obtained yet
        if data_list[2] == "V":
            os.system('clear')
            print("Positioning not obtained yet!", data_list[1])
            sentence = b""
            continue
    
        os.system("clear")
        print(sentence)
        with open("gps_log.txt", "a") as fp:
            fp.write(sentence + "\n")
        sentence = b""
    

def upload_data():
    with(open "gps_log.txt", "r") as fp:
        for line in fp.readlines():
            pass
