# Simple Harmonic Saturator

A command-line audio DSP project written in C++.

This project implements a harmonic saturator for 16-bit PCM WAV files using:

* Hyperbolic tangent (tanh) waveshaping
* 4x oversampling
* Sinc-generated FIR low-pass filters
* Hann windowing
* Anti-imaging filtering
* Anti-alias filtering
* Stereo WAV support
* Configurable drive, output gain and cutoff frequency

## DSP Pipeline

Input WAV

↓

4x Zero-Stuffing Upsampling

↓

FIR Anti-Imaging Low-Pass Filter

↓

tanh Waveshaping Saturation

↓

FIR Anti-Alias Low-Pass Filter

↓

Downsampling (Decimation)

↓

Output WAV

## Usage

```bash
./saturator input.wav output.wav [drive] [outputGain] [cutoffHz]
```

Example:

```bash
./saturator input.wav output.wav 5.0 0.7 18000
```

## Learning Goals

This project was created as a practical exploration of:

* Digital Signal Processing (DSP)
* FIR filter design
* Windowed-sinc filters
* Oversampling
* Nyquist frequency and aliasing
* Audio effects implementation in C++

```
```
