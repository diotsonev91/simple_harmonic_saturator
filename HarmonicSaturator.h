#pragma once

#include <cmath>
#include <algorithm>

class HarmonicSaturator {
public:
    void prepare(double newSampleRate, int newChannels) {
        sampleRate = newSampleRate;
        channels = newChannels;
    }

    void setDrive(float newDrive) {
        drive = std::max(0.0f, newDrive);
    }

    float processSample(float x) const {
        return std::tanh(drive * x);
    }

private:
    double sampleRate = 44100.0;
    int channels = 1;

    float drive = 5.0f;

};