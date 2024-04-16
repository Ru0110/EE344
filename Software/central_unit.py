import struct
import time
import socket
import network
import urequests as requests
import umail
import gc
from machine import Pin, I2C, PWM
from ssd1306 import SSD1306_I2C
import utime
import json
import framebuf

# --- Display Configuration ---
i2c = I2C(1, sda=Pin(2), scl=Pin(3))
display = SSD1306_I2C(128, 64, i2c)

URL = "https://ap-south-1.aws.data.mongodb-api.com/app/data-ibwwd/endpoint/data/v1/action/"
API_KEY = "" # Relace with complete API key

# Replace with the actual IP address of the server
SERVER_ADDR = ""

# Constants (matching the server)
BUF_SIZE = 60 + 1024  # Adjust this if your server uses a different value
SERVER_PORT =   # Mention the port used for communication 
format_string = "IfffffffffffffI" + "h"*512  # Format of the struct data you are trying to receive

# Define your thresholds (Both upper and lower caps)
TEMPERATURE_THRESHOLD_U = 45 
HUMIDITY_THRESHOLD_U = 40 
VARMS_THRESHOLD_U = 250 
VBRMS_THRESHOLD_U = 250
IARMS_THRESHOLD_U = 10 
IBRMS_THRESHOLD_U = 10 
POWERA_THRESHOLD_U = 2500
POWERB_THRESHOLD_U = 2500
TEMPERATURE_THRESHOLD_L = -1
HUMIDITY_THRESHOLD_L = -1
VARMS_THRESHOLD_L = -1
VBRMS_THRESHOLD_L = -1 
IARMS_THRESHOLD_L = -1 
IBRMS_THRESHOLD_L = -1
POWERA_THRESHOLD_L = -1
POWERB_THRESHOLD_L = -1

# --- Buzzer Configuration ---
BUZZER_PIN = 15
BUZZER_FREQUENCY = 500
BUZZER_DURATION = 1  # Seconds
BUZZER_DUTY_CYCLE = 100  # Percent

NTFY_TOPIC = "" # Name of the ntfy topic where you want send notifications
NTFY_URL = "https://ntfy.sh/" + NTFY_TOPIC
        
def insertOne_sensor(time, VArms, VBrms, IArms, IBrms, PowerA, PowerB, PfA, PfB, freqA, freqB, temp, pressure, humidity):
    try:
        headers = { "api-key": API_KEY }
        documentToAdd = {"Time": time,
                         "VArms": VArms,
                         "VBrms": VBrms,
                         "IArms": IArms,
                         "IBrms": IBrms,
                         "PowerA": PowerA,
                         "PowerB": PowerB,
                         "PfA": PfA,
                         "PfB": PfB,
                         "freqA": freqA,
                         "freqB": freqB,
                         "Temperature (C)": temp,
                         "Pressure": pressure,
                         "Humidity": humidity,
                         }
        
        insertPayload = {
            "dataSource": "Cluster0",
            "database": "SensorData",
            "collection": "Sensor",
            "document": documentToAdd,
        }
        response = requests.post(URL + "insertOne", headers=headers, json=insertPayload)
        print(response)
        print("Response: (" + str(response.status_code) + "), msg = " + str(response.text))
        if response.status_code >= 200 and response.status_code < 300:
            print("Success Response")
        else:
            print(response.status_code)
            print("Error")
        response.close()
    except Exception as e:
        print(e)

def insertOne_waveform(waveform, time):
    try:
        headers = { "api-key": API_KEY }
        documentToAdd = {"Waveform Array": waveform,
                         "Time": time}
        
        insertPayload = {
            "dataSource": "Cluster0",
            "database": "SensorData",
            "collection": "Waveform",
            "document": documentToAdd,
        }
        response = requests.post(URL + "insertOne", headers=headers, json=insertPayload)
        print(response)
        print("Response: (" + str(response.status_code) + "), msg = " + str(response.text))
        if response.status_code >= 200 and response.status_code < 300:
            print("Success Response")
        else:
            print(response.status_code)
            print("Error")
        response.close()
    except Exception as e:
        print(e)

def insertMany_sensor(document_list):
    try:
        headers = { "api-key": API_KEY }
        insertPayload = {
            "dataSource": "Cluster0",
            "database": "SensorData",
            "collection": "Sensor",
            "documents": document_list,
        }
        response = requests.post(URL + "insertMany", headers=headers, json=insertPayload)
        print("Response: (" + str(response.status_code) + "), msg = " + str(response.text))
        if response.status_code >= 200 and response.status_code < 300:
            print("Success Response")
        else:
            print(response.status_code)
            print("Error")
        response.close()
    except Exception as e:
        print(e)

def insertMany_waveform(document_list):
    try:
        headers = { "api-key": API_KEY }
        insertPayload = {
            "dataSource": "Cluster0",
            "database": "SensorData",
            "collection": "Waveform",
            "documents": document_list,
        }
        response = requests.post(URL + "insertMany", headers=headers, json=insertPayload)
        print("Response: (" + str(response.status_code) + "), msg = " + str(response.text))
        if response.status_code >= 200 and response.status_code < 300:
            print("Success Response")
        else:
            print(response.status_code)
            print("Error")
        response.close()
    except Exception as e:
        print(e)
        
def connect_to_wifi(ssid, psk):
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(ssid, psk)

    while not wlan.isconnected() and wlan.status() >= 0:
        print("Waiting to Connect")
        time.sleep(10)
    if not wlan.isconnected():
        raise Exception("Wifi not available")
    print("Connected to WiFi")
    
def display_data(temperature, humidity, voltage, current, power, alert_message = None):
    if alert_message is None:
        display.fill(0)  
        display.text('Temp: ' + str(temperature) + 'C', 0, 0)
        display.text('RH: ' + str(humidity) + '%', 0, 12)
        display.text('Voltage: ' + str(voltage) + 'V', 0, 24)
        display.text('Current: ' + str(current) + 'A', 0, 36)
        display.text('Power: ' + str(power) + 'W', 0, 48)
        display.show()
    else:
        display.fill(0)  
        display.text(alert_message, 0, 0)
        display.text(alert_message, 0, 12)
        display.text(alert_message, 0, 24)
        display.text(alert_message, 0, 36)
        display.text(alert_message, 0, 48)
        display.show()
    
def get_formatted_time_and_date():
    current_time = utime.localtime()
    return f"{current_time[0]:04d}-{current_time[1]:02d}-{current_time[2]:02d} {current_time[3]:02d}:{current_time[4]:02d}:{current_time[5]:02d}"

def play_buzzer():
    buzzer = PWM(Pin(BUZZER_PIN))
    buzzer.freq(BUZZER_FREQUENCY)
    duty_u16 = int(BUZZER_DUTY_CYCLE / 100 * 65535)  
    buzzer.duty_u16(duty_u16)
    utime.sleep(BUZZER_DURATION)
    buzzer.deinit()

def stop_buzzer(pin_num=15):
    buzzer_pin = Pin(pin_num, Pin.OUT) 
    buzzer_pin.value(0)
    
def send_notification(text):
    message = text + "\n Lab is probably on fire. You should check it out."     
    requests.post(NTFY_URL,
    data = message,
    headers={
        "Title": "Threshold Alert!",
    #    "Email": "",  # Fill the email address to which you want to receive the notifications
        "Priority": "urgent",
        "Tags": "warning,skull" 
    })
    
# Generic function for sending alerts
def send_alert(alert, value, threshold, upper_threshold = True, extra_info=""):
    if upper_threshold:
        text = f"Warning: {alert}! Current value is {value}, exceeding threshold of {threshold}. {extra_info}"
        print(text)
        alert_message = alert
        play_buzzer()
        send_notification(text)

    else:
        text = f"Warning: {alert}! Current value is {value}, falling below threshold of {threshold}. {extra_info}"
        print(text)
        alert_message = alert
        play_buzzer()
        send_notification(text)
        
def check_and_send_alerts():
    if Temperature > TEMPERATURE_THRESHOLD_U:
        send_alert("Temperature Alert", Temperature, TEMPERATURE_THRESHOLD_U, upper_threshold = True)

    if Humidity > HUMIDITY_THRESHOLD_U:
        send_alert("Humidity Alert", Humidity, HUMIDITY_THRESHOLD_U, upper_threshold = True)

    if VArms > VARMS_THRESHOLD_U:
        send_alert("VArms Alert", VArms, VARMS_THRESHOLD_U, upper_threshold = True)

    if VBrms > VBRMS_THRESHOLD_U:
        send_alert("VBrms Alert", VBrms, VBRMS_THRESHOLD_U, upper_threshold = True)
        
    if IArms > IARMS_THRESHOLD_U:
        send_alert("IArms Alert", IArms, IARMS_THRESHOLD_U, upper_threshold = True)

    if IBrms > IBRMS_THRESHOLD_U:
        send_alert("IBrms Alert", IBrms, IBRMS_THRESHOLD_U, upper_threshold = True)
        
    if PowerA > POWERA_THRESHOLD_U:
        send_alert("PowerA Alert", PowerA, POWERA_THRESHOLD_U, upper_threshold = True)

    if PowerB > POWERB_THRESHOLD_U:
        send_alert("PowerB Alert", PowerB, POWERB_THRESHOLD_U, upper_threshold = True)
        
    if Temperature < TEMPERATURE_THRESHOLD_L:
        send_alert("Temperature Alert", Temperature, TEMPERATURE_THRESHOLD_L, upper_threshold = False)

    if Humidity < HUMIDITY_THRESHOLD_L:
        send_alert("Humidity Alert", Humidity, HUMIDITY_THRESHOLD_L, upper_threshold = False)

    if VArms < VARMS_THRESHOLD_L:
        send_alert("VArms Alert", VArms, VARMS_THRESHOLD_L, upper_threshold = False)

    if VBrms < VBRMS_THRESHOLD_L:
        send_alert("VBrms Alert", VBrms, VBRMS_THRESHOLD_L, upper_threshold = False)
        
    if IArms < IARMS_THRESHOLD_L:
        send_alert("IArms Alert", IArms, IARMS_THRESHOLD_L, upper_threshold = False)

    if IBrms < IBRMS_THRESHOLD_L:
        send_alert("IBrms Alert", IBrms, IBRMS_THRESHOLD_L, upper_threshold = False)
        
    if PowerA < POWERA_THRESHOLD_L:
        send_alert("PowerA Alert", PowerA, POWERA_THRESHOLD_L, upper_threshold = False)

    if PowerB < POWERB_THRESHOLD_L:
        send_alert("PowerB Alert", PowerB, POWERB_THRESHOLD_L, upper_threshold = False)
        
    
connect_to_wifi(SSID, PSK) # Fill Wifi credentials to connect to database

# Socket Setup
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
sock.connect((SERVER_ADDR, SERVER_PORT))

alert_message = None

while True:
    
    stop_buzzer()
    alert_message = None
    
    total_size = BUF_SIZE
    read_buf = b''
    while total_size > 0:
        buf = sock.recv(BUF_SIZE)
        print('read %d bytes from server' % len(buf)) 
        total_size -= len(buf)
        read_buf += buf
        utime.sleep(0.5)

    # Data Processing
    if len(read_buf) != BUF_SIZE:
        sock.close()
        raise RuntimeError('wrong amount of data read %d', len(read_buf))
    else:
        
        # Unpack the data using struct.unpack
        formatted_time = get_formatted_time_and_date()
        unpacked_data = struct.unpack(format_string, read_buf)
        
        VArms = unpacked_data[1]
        VBrms = unpacked_data[2]
        IArms = unpacked_data[3]
        IBrms = unpacked_data[4]
        PowerA = unpacked_data[5]*620/401
        PowerB = unpacked_data[6]
        PfA = unpacked_data[7]
        PfB = unpacked_data[8]
        freqA = unpacked_data[9]
        freqB = unpacked_data[10]
        Temperature = unpacked_data[11]
        Pressure = unpacked_data[12]
        Humidity = unpacked_data[13]

        waveform_buffer = unpacked_data[-512:]

        print("Time:", formatted_time)
        print("VArms:", VArms, "V")
        print("VBrms:", VBrms, "V")
        print("IArms:", IArms, "A")
        print("IBrms:", IBrms, "A")
        print("PowerA:", PowerA, "W")
        print("PowerB:", PowerB, "W")
        print("PfA:", PfA)
        print("PfB:", PfB)
        print("freqA:", freqA)
        print("freqB:", freqB)
        print("Temperature:", Temperature, "C")
        print("Pressure:", Pressure, "Pa")
        print("Relative Humidity:", Humidity, "%")
        print("Length of Waveform Buffer:", len(waveform_buffer))
        
        waveform_json = []
        waveform_json.append({"Time": formatted_time,
                    "Waveform Buffer": waveform_buffer})
        
        display_data(Temperature, Humidity, VArms, IArms, PowerA)
        check_and_send_alerts()
        display_data(Temperature, Humidity, VArms, IArms, PowerA)
        
        insertOne_sensor(formatted_time, VArms, VBrms, IArms, IBrms, PowerA, PowerB, PfA, PfB, freqA, freqB, Temperature, Pressure, Humidity)
        insertMany_waveform(waveform_json)

        utime.sleep(1)


