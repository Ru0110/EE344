import matplotlib.pyplot as plt 
import numpy as np 
from scipy.fft import fft

numpy_waveform = np.array(list(map(float, open("sample_waveform.txt").read().splitlines())))
#data = data*801/18196
print("Max reading:", np.max(numpy_waveform))
print("Min reading:", np.min(numpy_waveform))

# Given parameters
sampling_rate = 8000  # Sampling rate in Hz

signal_fft = fft(numpy_waveform)

# Calculate frequency bins with the given sampling rate
fft_freqs = np.fft.fftfreq(n=numpy_waveform.size, d=1/sampling_rate)

# Plotting Periodogram with the known sampling rate
plt.figure(figsize=(12, 6))
plt.plot(fft_freqs, np.abs(signal_fft))
plt.title('Periodogram with Sampling Rate of 8,000 Hz')
plt.xlabel('Frequency [Hz]')
plt.ylabel('Magnitude')
plt.xlim([0, 100])  # Display up to Nyquist frequency
plt.grid(True)
plt.show()
