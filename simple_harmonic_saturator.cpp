#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

#pragma pack(push, 1)
struct WavHeader {
    char riff[4];
    uint32_t fileSize;
    char wave[4];
    char fmt[4];
    uint32_t fmtSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char data[4];
    uint32_t dataSize;
};
#pragma pack(pop)

constexpr float PI = 3.14159265358979323846f;

float saturate(float x, float drive) {
    return std::tanh(drive * x);
}

float sinc(float x) {
    if (std::abs(x) < 1e-6f) return 1.0f;
    return std::sin(PI * x) / (PI * x);
}

std::vector<float> makeLowpassFIR(int taps, float cutoff) {
    std::vector<float> h(taps);
    int mid = taps / 2;

    for (int i = 0; i < taps; ++i) {
        int n = i - mid;

        // sinc low-pass
        float value = 2.0f * cutoff * sinc(2.0f * cutoff * n);

        // Hann window
        float window = 0.5f - 0.5f * std::cos(2.0f * PI * i / (taps - 1));

        h[i] = value * window;
    }

    // normalize gain
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

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: ./saturator input.wav output.wav [drive] [outputGain] [cutoff frequency]\n";
        return 1;
    }

    const char* inputFile = argv[1];
    const char* outputFile = argv[2];

    std::ifstream in(inputFile, std::ios::binary);
    if (!in) {
        std::cerr << "Cannot open input file\n";
        return 1;
    }

    WavHeader header;
    in.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));

    if (header.audioFormat != 1 || header.bitsPerSample != 16) {
        std::cerr << "Only 16-bit PCM WAV supported.\n";
        return 1;
    }

    int numSamples = header.dataSize / sizeof(int16_t);
    std::vector<int16_t> samples(numSamples);
    in.read(reinterpret_cast<char*>(samples.data()), header.dataSize);
    in.close();

    int channels = header.numChannels;
    int frames = numSamples / channels;

    const int OS = 4;
    const int taps = 101;

    float drive = 5.0f;
    float outputGain = 0.7f;
    float cutoffHz = 18000;

    if (argc >= 4)
        drive = std::stof(argv[3]);

    if (argc >= 5)
        outputGain = std::stof(argv[4]);

    if (argc >= 6)
        cutoffHz = std::stof(argv[5]);

    float oversampledRate =
    header.sampleRate * OS;

    float maxCutoffHz = (header.sampleRate / 2.0f) * 0.95f;
    cutoffHz = std::clamp(cutoffHz, 20.0f, maxCutoffHz);

    float cutoff = cutoffHz / oversampledRate;

    std::cout << "Drive      : " << drive << "\n";
    std::cout << "OutputGain : " << outputGain << "\n";
    std::cout << "CutoffHz     : " << cutoffHz << "\n";  
    std::cout << "Cutoff     : " << cutoff << "\n";    

    auto fir = makeLowpassFIR(taps, cutoff);    

    std::vector<int16_t> processed(numSamples);

    for (int ch = 0; ch < channels; ++ch) {
        std::vector<float> mono(frames);

        for (int i = 0; i < frames; ++i) {
            mono[i] = samples[i * channels + ch] / 32768.0f;
        }

        // 1. Zero-stuffing / upsample
        std::vector<float> upsampled(frames * OS, 0.0f);

        for (int i = 0; i < frames; ++i) {
            upsampled[i * OS] = mono[i] * OS;
        }

        // 2. Anti-imaging FIR low-pass
        auto reconstructed = applyFIR(upsampled, fir);

        // 3. Saturation at 4x sample rate
        for (float& x : reconstructed) {
            x = saturate(x, drive);
        }

        // 4. Anti-alias FIR low-pass before downsampling
        auto filtered = applyFIR(reconstructed, fir);

        // 5. Decimate / take every 4th sample
        for (int i = 0; i < frames; ++i) {
            float y = filtered[i * OS];

            y *= outputGain;
            y = std::clamp(y, -1.0f, 1.0f);

            processed[i * channels + ch] = static_cast<int16_t>(y * 32767.0f);
        }
    }

    std::ofstream out(outputFile, std::ios::binary);
    if (!out) {
        std::cerr << "Cannot create output file\n";
        return 1;
    }

    out.write(reinterpret_cast<char*>(&header), sizeof(WavHeader));
    out.write(reinterpret_cast<char*>(processed.data()), header.dataSize);
    out.close();

    std::cout << "Done. Created " << outputFile << "\n";
    return 0;
}