#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

#include "OversampledSaturatorProcessor.h"
#include "WavFile.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: ./saturator input.wav output.wav [drive] [outputGain] [cutoff frequency] [mix]\n";
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
  
    float drive = 5.0f;
    float outputGain = 0.7f;
    float cutoffHz = 18000;
    float mix = 1.0f;

    if (argc >= 4)
        drive = std::stof(argv[3]);

    if (argc >= 5)
        outputGain = std::stof(argv[4]);

    if (argc >= 6)
        cutoffHz = std::stof(argv[5]);

    if (argc >= 7)
        mix = std::stof(argv[6]);


    std::cout << "Drive      : " << drive << "\n";
    std::cout << "OutputGain : " << outputGain << "\n";
    std::cout << "CutoffHz     : " << cutoffHz << "\n";  
    std::cout << "mix     : " << mix<< "\n";     

    OversampledSaturatorProcessor processor;

    processor.prepare(header.sampleRate, channels);
    processor.setDrive(drive);
    processor.setOutputGain(outputGain);
    processor.setCutoffHz(cutoffHz);
    processor.setMix(mix);

    std::vector<int16_t> processed = processor.process(samples);

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