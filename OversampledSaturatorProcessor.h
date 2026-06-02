#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

#include "HarmonicSaturator.h"

class OversampledSaturatorProcessor {
public:


    void prepare(int sampleRate, int channels) {
        this->sampleRate = sampleRate;
        this->channels = channels;

        saturator.prepare(sampleRate * OS, channels);
    }

    void setDrive(float drive) {
        saturator.setDrive(drive);
    }

    void setOutputGain(float gain) {
        outputGain = gain;
    }

    void setCutoffHz(float cutoffHz) {
        float maxCutoffHz = (sampleRate / 2.0f) * 0.95f;
        cutoffHz = std::clamp(cutoffHz, 20.0f, maxCutoffHz);

        float oversampledRate = sampleRate * OS;
        float cutoff = cutoffHz / oversampledRate;

        fir = makeLowpassFIR(taps, cutoff);
    }

    void setMix(float newMix) {
        mix = std::clamp(newMix, 0.0f, 1.0f);
    }

    std::vector<int16_t> process(const std::vector<int16_t>& samples) {
        int numSamples = samples.size();
        int frames = numSamples / channels;

        std::vector<int16_t> processed(numSamples);

        for (int ch = 0; ch < channels; ++ch) {
            std::vector<float> mono(frames);

            for (int i = 0; i < frames; ++i) {
                mono[i] = samples[i * channels + ch] / 32768.0f;
            }

            std::vector<float> upsampled(frames * OS, 0.0f);

            for (int i = 0; i < frames; ++i) {
                upsampled[i * OS] = mono[i] * OS;
            }

            auto reconstructed = applyFIR(upsampled, fir);

            for (float& x : reconstructed) {
                x = saturator.processSample(x);
            }

            auto filtered = applyFIR(reconstructed, fir);

            for (int i = 0; i < frames; ++i) {

                if (mix <= 0.0f) {
                    float y = mono[i] * outputGain;
                    y = std::clamp(y, -1.0f, 1.0f);

                    processed[i * channels + ch] =
                        static_cast<int16_t>(y * 32767.0f);

                    continue;
                }

                float dry = mono[i];
                float wet = filtered[i * OS];

                float wetAmount = mix * mix;
                float dryAmount = 1.0f - wetAmount;

                float y = dry * dryAmount + wet * wetAmount;

                y *= outputGain;
                y = std::clamp(y, -1.0f, 1.0f);

                processed[i * channels + ch] =
                    static_cast<int16_t>(y * 32767.0f);
            }
        }

        return processed;
    }

private:
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr int OS = 4;
    static constexpr int taps = 101;

    int sampleRate = 44100;
    int channels = 1;
    float outputGain = 0.7f;
    float mix = 1.0f;

    HarmonicSaturator saturator;
    std::vector<float> fir;

    float sinc(float x) {
        if (std::abs(x) < 1e-6f) return 1.0f;
        return std::sin(PI * x) / (PI * x);
    }

    std::vector<float> makeLowpassFIR(int taps, float cutoff) {
        std::vector<float> h(taps);
        int mid = taps / 2;

        for (int i = 0; i < taps; ++i) {
            int n = i - mid;

            float value = 2.0f * cutoff * sinc(2.0f * cutoff * n);
            float window = 0.5f - 0.5f * std::cos(2.0f * PI * i / (taps - 1));

            h[i] = value * window;
        }

        float sum = 0.0f;
        for (float v : h) sum += v;
        for (float& v : h) v /= sum;

        return h;
    }

    std::vector<float> applyFIR(const std::vector<float>& input,
                                const std::vector<float>& kernel) {
        std::vector<float> output(input.size(), 0.0f);

        int taps = kernel.size();
        int mid = taps / 2;

        for (int i = 0; i < (int)input.size(); ++i) {
            float acc = 0.0f;

            for (int k = 0; k < taps; ++k) {
                int index = i - k + mid;

                if (index >= 0 && index < (int)input.size()) {
                    acc += input[index] * kernel[k];
                }
            }

            output[i] = acc;
        }

        return output;
    }
};