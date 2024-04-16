from pymongo import MongoClient
import numpy as np
import matplotlib.pyplot as plt
from scipy.fft import fft

while True:

    conn_string = ""  # Provide the connection string unique to each user accessing the database
    client = MongoClient(conn_string) 

    db = client['SensorData']  
    collection = db['Waveform']  

    signal = []
    time = []
    for document in collection.find():
        signal.append(document['Waveform Buffer'])
        time.append(document['Time'])

    numpy_waveform = np.array(signal[-1])
    time_rn = str(time[-1])

    # Given parameters
    sampling_rate = 6400  # Sampling rate in Hz
    signal_fft = fft(numpy_waveform)

    # Calculate frequency bins with the given sampling rate
    fft_freqs = np.fft.fftfreq(n=numpy_waveform.size, d=1/sampling_rate)

    # Plotting Periodogram with the known sampling rate
    fig, ax = plt.subplots(2, 1, figsize = (12, 8))

    ax[0].plot(fft_freqs, np.abs(signal_fft))
    ax[0].set_title(f'Periodogram and Voltage values sampled at Time: {time_rn}')
    ax[0].set_xlabel('Frequency [Hz]')
    ax[0].set_ylabel('Magnitude')
    ax[0].set_xlim([0, 150])  
    ax[0].grid(True)

    ax[1].plot(numpy_waveform)
    ax[1].set_xlabel('Timestamps')
    ax[1].set_ylabel('Volatge')
    ax[1].grid(True)
    #plt.show()

    fig.savefig('Waveform_plot.png')
