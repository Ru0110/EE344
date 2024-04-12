#!/usr/bin/python

import struct
import socket
import sys
import csv 
import pylab as pyl
import time

#unpacking_str = "fffffffffffi"
unpacking_str = "ifffffffffffffi" + "i"*256
print("Unpacking length: ", struct.calcsize(unpacking_str))
received_params = ["Time (ms)", "RMS Voltage (V)", "VBrms", "RMS Current (A)", "IBrms", "Power (W)", "PowerB", "Power factor", "PfB", "Frequency (Hz)", "FrequencyB", "Temperature (C)", "Pressure (Pa)", "Humidity (%)", "Emergency Status"]
not_needed = ["VBrms", "IBrms", "PowerB", "PfB", "FrequencyB"]

# Check server ip address set
if len(sys.argv) < 2:
    raise RuntimeError('pass IP address of the server')

# Set the server address here like 1.2.3.4
SERVER_ADDR = sys.argv[1]

# These constants should match the server
BUF_SIZE = 1084 
SERVER_PORT = 4242
TEST_ITERATIONS = 50

# Open socket to the server
sock = socket.socket()
addr = (SERVER_ADDR, SERVER_PORT)
sock.connect(addr)

csvfile = open('readings.csv', 'w')
writer = csv.writer(csvfile)
writer.writerow(received_params)

#pyl.ion()
#fig, ax = pyl.subplots(3,1, figsize = (20,20), sharex=True)


# Repeat test indefinitely
while(True):

    try:

        # Read BUF_SIZE bytes from the server
        total_size = BUF_SIZE
        read_buf = b''
        while total_size > 0:
            buf = sock.recv(BUF_SIZE)
            #print('read %d bytes from server' % len(buf))
            total_size -= len(buf)
            read_buf += buf

        # Check size of data received
        if len(read_buf) != BUF_SIZE:
            raise RuntimeError('wrong amount of data read %d', len(read_buf))
        else: 
            print("")
            received_data = struct.unpack(unpacking_str, read_buf)
            for i in range(15):
                if(received_params[i] not in not_needed):
                    print("%-20s: %-10f" % (received_params[i], received_data[i]))
            print("======================== Waveform Buffer ======================")
            for i in range(15, 15+256):
                print(received_data[i], end = " ")
            print("\n===============================================================")

            writer.writerow(received_data[:15])
            #ax[0].scatter(received_data[0], received_data[1])
            #ax[1].scatter(received_data[0], received_data[3])
            #ax[2].scatter(received_data[0], received_data[5])
            #ax[2].set_xlabel(received_params[0])
            #ax[0].set_ylabel(received_params[1])
            #ax[1].set_ylabel(received_params[3])
            #ax[2].set_ylabel(received_params[5])
            #pyl.draw()
            #pyl.pause(0.01)
    
    except KeyboardInterrupt:
        print("Ended by user")
        print("Exiting")
        sock.close()
        print("test completed")
        csvfile.close()
        #pyl.ioff()
        #pyl.show()
        sys.exit()


        
        

    # Send the data back to the server
    #write_len = sock.send(read_buf)
    #print('written %d bytes to server' % write_len)
    #if write_len != BUF_SIZE:
    #    raise RuntimeError('wrong amount of data written')

