#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <stdint.h>

#define SAMPLE_RATE 44100  // Sample rate (44.1 kHz is standard)
#define BUFFER_FRAMES 128  // Number of frames in the buffer

// WAV file header structure
typedef struct {
    char chunkID[4];        // "RIFF"
    uint32_t chunkSize;     // Size of the entire file in bytes minus 8 bytes for 'chunkID' and 'chunkSize'
    char format[4];         // "WAVE"
    char subchunk1ID[4];    // "fmt "
    uint32_t subchunk1Size; // Size of the fmt chunk (16 for PCM)
    uint16_t audioFormat;   // PCM = 1
    uint16_t numChannels;   // Mono = 1, Stereo = 2, etc.
    uint32_t sampleRate;    // 44100, etc.
    uint32_t byteRate;      // SampleRate * NumChannels * BitsPerSample/8
    uint16_t blockAlign;    // NumChannels * BitsPerSample/8
    uint16_t bitsPerSample; // 16 bits per sample
    char subchunk2ID[4];    // "data"
    uint32_t subchunk2Size; // NumSamples * NumChannels * BitsPerSample/8 (size of the data)
} WAVHeader;

// Function to write the WAV header to a file
void write_wav_header(FILE *file, unsigned int sample_rate, int channels, int duration) {
    WAVHeader header;
    int bits_per_sample = 16;
    int byte_rate = sample_rate * channels * bits_per_sample / 8;
    int data_size = duration * sample_rate * channels * bits_per_sample / 8;

    // RIFF Header
    memcpy(header.chunkID, "RIFF", 4);
    header.chunkSize = 36 + data_size; // 4 bytes for "data" + data_size
    memcpy(header.format, "WAVE", 4);

    // fmt sub-chunk
    memcpy(header.subchunk1ID, "fmt ", 4);
    header.subchunk1Size = 16;    // PCM
    header.audioFormat = 1;       // PCM = 1
    header.numChannels = channels;
    header.sampleRate = sample_rate;
    header.byteRate = byte_rate;
    header.blockAlign = channels * bits_per_sample / 8;
    header.bitsPerSample = bits_per_sample;

    // data sub-chunk
    memcpy(header.subchunk2ID, "data", 4);
    header.subchunk2Size = data_size;

    // Write the header to the file
    fwrite(&header, sizeof(WAVHeader), 1, file);
}

// Function to initialize ALSA for capture
snd_pcm_t* init_alsa(const char* pcm_name, snd_pcm_stream_t stream, unsigned int sample_rate, int channels) {
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *params;
    unsigned int rate = sample_rate;
    int dir;

    // Open the PCM device
    if (snd_pcm_open(&pcm_handle, pcm_name, stream, 0) < 0) {
        fprintf(stderr, "Error opening PCM device %s\n", pcm_name);
        return NULL;
    }

    // Allocate and initialize hardware parameters
    snd_pcm_hw_params_malloc(&params);
    snd_pcm_hw_params_any(pcm_handle, params);
    
    // Set the parameters
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, channels);  // Set the number of channels
    snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, &dir);

    // Write the parameters to the PCM device
    if (snd_pcm_hw_params(pcm_handle, params) < 0) {
        fprintf(stderr, "Error setting HW params\n");
        snd_pcm_hw_params_free(params);
        return NULL;
    }

    // Free hardware parameters and return the handle
    snd_pcm_hw_params_free(params);
    return pcm_handle;
}

int main(int argc, char *argv[]) {
    snd_pcm_t *capture_handle;
    short *buffer;
    int frames_to_capture, frames_captured = 0, err, duration, channels;
    FILE *wav_file;

    // Check if duration, channels, and output file name arguments are provided
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <duration_in_seconds> <channels> <output_file.wav>\n", argv[0]);
        return -1;
    }

    // Parse the duration and channels from command line
    duration = atoi(argv[1]);
    channels = atoi(argv[2]);
    if (duration <= 0 || channels <= 0) {
        fprintf(stderr, "Invalid duration or channels\n");
        return -1;
    }

    // Calculate the total frames to capture (sample_rate * duration)
    frames_to_capture = SAMPLE_RATE * duration;

    // Initialize ALSA for capture
    capture_handle = init_alsa("default", SND_PCM_STREAM_CAPTURE, SAMPLE_RATE, channels);
    if (!capture_handle) {
        fprintf(stderr, "Failed to initialize ALSA capture device\n");
        return -1;
    }

    // Allocate buffer to hold captured audio data (for the specified number of channels)
    buffer = (short *)malloc(BUFFER_FRAMES * channels * sizeof(short));

    // Open the output WAV file
    wav_file = fopen(argv[3], "wb");
    if (!wav_file) {
        fprintf(stderr, "Error opening WAV file for writing\n");
        return -1;
    }

    // Write the WAV header (we'll fill in the correct sizes later)
    write_wav_header(wav_file, SAMPLE_RATE, channels, duration);

    // Capture audio
    while (frames_captured < frames_to_capture) {
        // Capture audio from the microphone
        err = snd_pcm_readi(capture_handle, buffer, BUFFER_FRAMES);
        if (err == -EPIPE) {
            fprintf(stderr, "Buffer overrun during capture\n");
            snd_pcm_prepare(capture_handle);
        } else if (err < 0) {
            fprintf(stderr, "Error capturing audio: %s\n", snd_strerror(err));
        }

        // Write captured audio to the WAV file
        fwrite(buffer, sizeof(short), err * channels, wav_file);

        // Increment the number of frames captured
        frames_captured += err;
    }

    // Free resources
    free(buffer);
    snd_pcm_drain(capture_handle);
    snd_pcm_close(capture_handle);
    fclose(wav_file);

    printf("Audio captured and saved to %s\n", argv[3]);

    return 0;
}
