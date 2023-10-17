#include <iostream>
#include <string>
#include <vector>
#include "sndfile.h"
#include <fstream>
#include <sstream>
#include <exception>

const int sample_rate = 44100;

// Базовый класс для всех исключений в программе
class SoundProcessorException : public std::exception {
public:
    SoundProcessorException(const std::string& message) : message_(message) {}

    virtual const char* what() const noexcept override {
        return message_.c_str();
    }

private:
    std::string message_;
};

// Класс для ошибок, связанных с открытием/чтением WAV-файла
class FileOpenException : public SoundProcessorException {
public:
    FileOpenException(const std::string& filename)
            : SoundProcessorException("Ошибка: Невозможно открыть файл '" + filename + "'") {}
};

class FileParametersException : public SoundProcessorException {
public:
    FileParametersException(const std::string& filename)
            : SoundProcessorException("Ошибка: неверные параметры файла '" + filename + "'") {}
};

// Класс для ошибок, связанных с записью WAV-файла
class FileWriteException : public SoundProcessorException {
public:
    FileWriteException(const std::string& filename)
            : SoundProcessorException("Ошибка: Невозможно записать в файл '" + filename + "'") {}
};

// Класс для ошибок, связанных с конфигурационными файлами
class ConfigFileException : public SoundProcessorException {
public:
    ConfigFileException(const std::string& filename)
            : SoundProcessorException("Ошибка: Невозможно прочитать конфигурационный файл '" + filename + "'") {}
};
class WAVManager{


public:
    std::vector<double> loadAudioFile(const std::string& filename) {
        try {SF_INFO sfinfo;
            SNDFILE* sndfile = sf_open(filename.c_str(), SFM_READ, &sfinfo);

            if (!sndfile) {
                throw FileOpenException(filename);
            }

            if (sfinfo.channels != 1 || sfinfo.samplerate != sample_rate) {
                throw FileParametersException(filename);
            }

            std::vector<double> samples(sfinfo.frames * sfinfo.channels);
            sf_read_double(sndfile, samples.data(), samples.size());

            sf_close(sndfile);

            return samples;
        } catch (const SoundProcessorException &e) {
            std::cerr << e.what() << std::endl;
            exit(1);
        }

    }

    void saveAudioFile(const std::string& filename, const std::vector<double>& samples, int channels, int sampleRate) {
        try {
            SF_INFO sfinfo;
            sfinfo.samplerate = sampleRate;
            sfinfo.channels = channels;
            sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
            SNDFILE *sndfile = sf_open(filename.c_str(), SFM_WRITE, &sfinfo);

            if (!sndfile) {
                throw FileWriteException(filename);

            }

            sf_write_double(sndfile, samples.data(), samples.size());
            sf_close(sndfile);
        } catch (const SoundProcessorException &e) {
            std::cerr << e.what() << std::endl;
            exit(1);
        }
    }
};


class Converter{
public:
    std::vector<double> applyMute(const std::vector<double>& input1, int start, int end) {
        std::vector<double> buffer(input1.size());
        std::copy(input1.begin(), input1.end(), buffer.begin());
        const int startFrame = static_cast<int>(start * sample_rate);
        const int endFrame = static_cast<int>(end * sample_rate);

        for (int frame = 0; frame < input1.size(); ++frame) {
            if (frame >= startFrame && frame < endFrame) {
                buffer[frame] = 0.0;
            }
        }
        return buffer;
    }


    std::vector<double> applyMix(const std::vector<double>& input1, const std::vector<double>& input2, int position=0) {


        const int startFrame = static_cast<int>(position * sample_rate);

        std::vector<double> buffer1(input1.size());
        std::vector<double> buffer2(input2.size());
        std::copy(input1.begin(), input1.end(), buffer1.begin());
        std::copy(input2.begin(), input2.end(), buffer2.begin());

        // Смешиваем данные
        for (int frame = 0; frame < input1.size(); ++frame) {
            if (frame >= startFrame)
                buffer1[frame] += buffer2[frame];
            buffer1[frame] /= 2;
        }

        return buffer1;
    }



// Ваш собственный конвертер
    std::vector<double> applySpeedUp(const std::vector<double>& input1, double speedFactor) {


        std::vector<double> buffer(input1.size());
        std::copy(input1.begin(), input1.end(), buffer.begin());

        // Увеличиваем частоту дискретизации (интерполяция)
        std::vector<double> outputBuffer( static_cast<int>(input1.size() / speedFactor));

        for (int frame = 0; frame < input1.size() / speedFactor; ++frame) {
            double t = frame * speedFactor;
            int intT = static_cast<int>(t);
            double fraction = t - intT;

            outputBuffer[frame] = (1 - fraction) * buffer[intT] + fraction * buffer[(intT + 1)];
        }
        return outputBuffer;
    }

};

class SoundProcessor {
public:
    std::vector<double> main_sample;
    std::string outputFilename;
    std::vector<std::vector<double>> input_samples;

    SoundProcessor(const std::string& inputFile, const std::string& outputFile, const std::vector<std::string>& inputFilenames) {
        WAVManager(WAV);
        main_sample = WAV.loadAudioFile(inputFile);
        outputFilename = outputFile;
        for (const auto & inputFilename : inputFilenames) {
            input_samples.push_back(WAV.loadAudioFile(inputFilename));
        }
    }

    void processConfigFile(const std::string& configFilename) {
        std::ifstream inputFile(configFilename);
        try {
            if (!inputFile.is_open()) {
                throw ConfigFileException(configFilename);
            }


        Converter(converter);
        WAVManager(WAV);
        std::string line;
        while (std::getline(inputFile, line)) {
            if (line.empty() || line[0] == '#') {
                continue;
            }

            std::istringstream iss(line);
            std::string command;
            iss >> command;

            if (command == "mute") {
                int start, end;
                iss >> start >> end;
                std::cout << "Muting from " << start << " seconds to " << end << " seconds." << std::endl;

                main_sample = converter.applyMute(main_sample, start, end);
            } else if (command == "mix") {
                std::string inputRef;
                int startTime;
                iss >> inputRef >> startTime;

                if (inputRef[0] == '$') {
                    int inputIndex = std::stoi(inputRef.substr(1));
                    std::cout << "Mixing with input" << inputIndex << " starting from " << startTime << " seconds." << std::endl;
                    main_sample = converter.applyMix(main_sample, input_samples[inputIndex-2], startTime);
                } else {
                    throw ConfigFileException(configFilename);
                }
            } else if (command == "speed_up") {
                double parameters;
                iss >> parameters;
                std::cout << "speed up on " << parameters << std::endl;
                main_sample = converter.applySpeedUp(main_sample, parameters);
            } else {
                throw ConfigFileException(configFilename);
            }
        }

        inputFile.close();
        WAV.saveAudioFile(outputFilename, main_sample, 1, sample_rate);
        } catch (const SoundProcessorException &e) {
            std::cerr << e.what() << std::endl;
            exit(1);
        }
    }
};




int main(int argc, char* argv[]) {
    if (argv[1][0] == '-' && argv[1][1] == 'h') {
        std::cout << "Usage: sound_processor -c <config.txt> <output.wav> <input1.wav> [<input2.wav> ...]" << std::endl;
        return 0;
    }
    if (argc < 5) {
        std::cerr << "Usage: sound_processor -c <config.txt> <output.wav> <input1.wav> [<input2.wav> ...]" << std::endl;
        return 1;
    }

    std::string configFile = argv[2];
    char *outputFile = argv[3];
    char *inputFile = argv[4];

    std::vector<std::string> inputFiles;
    for (int i = 5; i < argc; ++i) {
        inputFiles.push_back(argv[i]);
    }
    SoundProcessor sound_proc(inputFile, outputFile, inputFiles);
    sound_proc.processConfigFile(configFile);
    return 0;
}

