#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

//Reference: https://gist.github.com/Jon-Schneider/8b7c53d27a7a13346a643dac9c19d34f


/**
 * Struct for WAV header data.
 */
struct WavHeader
{
    char riff[4];  //"RIFF" filetype
    uint32_t wave_file_size; // Total size of the file in bytes
    char wave[4]; //"WAVE" Indicates the wav filetype
    char chunk_marker[4]; // Format chunk marker
    uint32_t wav_length; // Length of the format data based on the format chunk marker
    uint16_t audio_fmt; // Audio format for a wav file, PCM is represented by 1
    uint16_t audio_chn; // Number of audio channels, 1 for monophonic audio
    uint32_t sampling_rate; // Sampling rate of the saved audio file
    uint32_t bytes_ps; // Bytes per second: (Sample rate * Bits per sample * Number of channels)/8
    uint16_t bytes_ps_chn; // Bytes per sample per channel:  (Bits per sample * Number of channels)/8
    uint16_t bits_ps; // Bits per sample
    char data_chunk[4]; // Data chunk header
    uint32_t data_size; // Total size of the data in bytes excluding the 44 byte header
} __attribute__((packed));


// int SAMPLING_RATE = 20000;
// const int WAVE_LENGTH = 16;
// const int AUDIO_CHANNELS = 1;
// const int AUDIO_FORMAT = 1;
// const int BITS_PER_SAMPLE = 16;
// const int DATA_SIZE = 500000;

int SAMPLING_RATE = 0;
int WAVE_LENGTH = 16;
int AUDIO_CHANNELS = 1;
int AUDIO_FORMAT = 1;
int BITS_PER_SAMPLE = 0;
long DATA_SIZE = 0;

struct WavHeader wav_header;


int main(int argc, char *argv[]) {
    char file_path[1024] = "";
    char output_file_path[1024] = "";
    long SAMPLING_RATE_long = 0;
    long BITS_PER_SAMPLE_long = 0;
    long GAIN_long = 0;

    if (argc >= 6) {
        strcpy(file_path, argv[1]); // Input file location and name, relative to where the executable is placed
        strcpy(output_file_path, argv[2]); // Output file location and name, relative to where the executable is placed
        SAMPLING_RATE_long = strtol(argv[3], NULL, 10);
        BITS_PER_SAMPLE_long = strtol(argv[4], NULL, 10);
        GAIN_long = strtol(argv[5], NULL, 10);
    } else {
        puts("Insufficient arguments cuh.");
        return 1;
    }

    SAMPLING_RATE = SAMPLING_RATE_long;
    BITS_PER_SAMPLE = BITS_PER_SAMPLE_long;
    float GAIN = GAIN_long;


    // Read the binary data
    FILE* fp_bin = fopen(file_path, "rb");
    if (fp_bin == NULL) {
        puts("Error opening input file");
        return 1;
    }

    // Get file size
    // Reference: https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
    fseek(fp_bin, 0, SEEK_END);
    long DATA_SIZE = ftell(fp_bin);
    fseek(fp_bin, 0, SEEK_SET);  // Rewind to start of file


    // Initialize the wav header struct
    strncpy(wav_header.riff, "RIFF", 4);
    strncpy(wav_header.wave, "WAVE", 4);
    strncpy(wav_header.chunk_marker, "fmt ", 4);
    strncpy(wav_header.data_chunk, "data", 4);

    wav_header.wave_file_size = DATA_SIZE + 44;
    wav_header.wav_length = WAVE_LENGTH;
    wav_header.audio_fmt = AUDIO_FORMAT;
    wav_header.audio_chn = AUDIO_CHANNELS;
    wav_header.sampling_rate = SAMPLING_RATE;
    wav_header.bytes_ps = (SAMPLING_RATE * BITS_PER_SAMPLE * AUDIO_CHANNELS)/8;
    wav_header.bytes_ps_chn = (BITS_PER_SAMPLE * AUDIO_CHANNELS) / 8;
    wav_header.bits_ps = BITS_PER_SAMPLE;
    wav_header.data_size = DATA_SIZE;

    // Write to the file
    FILE* fp = fopen(output_file_path, "wb");
    fwrite(&wav_header, sizeof(struct WavHeader), 1, fp); // Writes the file header first


    uint16_t* adc_data = (uint16_t*)malloc(DATA_SIZE);
    int16_t* adc_scaled = (int16_t*)malloc(DATA_SIZE);
    if (adc_data == NULL || adc_scaled == NULL) {
        puts("Memory allocation failed");
        fclose(fp_bin);
        return 1;
    }

    fread(adc_data, 1, DATA_SIZE, fp_bin);

    // Here we're converting the unsigned 12-bit ADC value to a signed 16 bit value range which is in accordance with the
    // PCM standard

    for (int i = 0; i < DATA_SIZE / 2; i++) {
        int32_t sample = ((int32_t)adc_data[i] * 65535 / 4095) - 32768;

        // Apply gain
        sample = (int32_t)(sample * GAIN);

        // Clamp the values to a 16-bit signed range
        if (sample > 32767) sample = 32767;
        else if (sample < -32768) sample = -32768;

        adc_scaled[i] = (int16_t)sample;
    }

    // Write the binary data
    fwrite(adc_scaled, 1, DATA_SIZE, fp);
    free(adc_data);
    free(adc_scaled);
    fclose(fp_bin);
    fclose(fp);
    return 0;
}