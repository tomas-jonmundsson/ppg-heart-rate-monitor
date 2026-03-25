import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import butter, filtfilt
from scipy.fft import fft, fftfreq

# Set seed for reproducibility
np.random.seed(0)

# Constants
SAMPLE_RATE = 100  # Hz
DURATION = 10  # seconds
HEART_RATE_FREQ = 1.2  # Hz (approximately 72 BPM)
NOISE_AMPLITUDE = 0.5  # Amplitude of noise

# Generate time array
time_array = np.arange(0, DURATION, 1/SAMPLE_RATE)

# Create synthetic PPG signal with noise
ppg_data = (NOISE_AMPLITUDE * np.sin(2 * np.pi * HEART_RATE_FREQ * time_array) +
            NOISE_AMPLITUDE * np.random.randn(len(time_array)))

# Center the signal around zero
ppg_data -= np.mean(ppg_data)

# Filter design parameters
HP_CUTOFF = 0.5  # High-pass filter cutoff frequency
LP_CUTOFF = 5.0   # Low-pass filter cutoff frequency

# High-pass filter implementation
def high_pass_filter(data, cutoff, fs):
    b, a = butter(2, cutoff / (0.5 * fs), btype='high')
    return filtfilt(b, a, data)

# Low-pass filter implementation
def low_pass_filter(data, cutoff, fs):
    b, a = butter(2, cutoff / (0.5 * fs), btype='low')
    return filtfilt(b, a, data)

# Apply filters
filtered_ppg = low_pass_filter(high_pass_filter(ppg_data, HP_CUTOFF, SAMPLE_RATE), LP_CUTOFF, SAMPLE_RATE)

# FFT computation
def compute_fft(signal, fs):
    n = len(signal)
    fft_result = np.abs(fft(signal))
    freq = fftfreq(n, d=1/fs)
    return freq, fft_result

frequency_array, fft_magnitude = compute_fft(filtered_ppg, SAMPLE_RATE)

# Extract frequencies within the specified range
freq_range = (frequency_array >= 0.5) & (frequency_array <= 10)
selected_freqs = frequency_array[freq_range]
selected_magnitude = fft_magnitude[freq_range]

# Calculate BPM
dominant_freq = selected_freqs[selected_magnitude.argmax()]
bpm_value = dominant_freq * 60

# Visualization
plt.figure(figsize=(12, 8))

# Plot original PPG signal
plt.subplot(3, 1, 1)
plt.plot(time_array, ppg_data, color='red')
plt.title("Original PPG Signal")
plt.xlabel("Time (s)")
plt.ylabel("Amplitude")

# Plot processed PPG signal
plt.subplot(3, 1, 2)
plt.plot(time_array, filtered_ppg, color='red')
plt.title("Processed PPG Signal")
plt.xlabel("Time (s)")
plt.ylabel("Amplitude")

# Plot FFT results
plt.subplot(3, 1, 3)
plt.plot(selected_freqs, selected_magnitude, color='blue')
plt.scatter(dominant_freq, max(selected_magnitude), color='limegreen', edgecolor='black', s=100, zorder=3)
plt.annotate(f"BPM: {bpm_value:.1f}", 
             (dominant_freq, max(selected_magnitude)), 
             xytext=(dominant_freq*1.1, max(selected_magnitude)*0.9), fontsize=12)
plt.title("FFT of Processed PPG Signal")
plt.xlabel("Frequency (Hz)")
plt.ylabel("Magnitude")

plt.tight_layout()
plt.show()
