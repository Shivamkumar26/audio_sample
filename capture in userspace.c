#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#define SAMPLE_RATE 44100  // Sample rate (44.1 kHz is standard)
#define CHANNELS 2         // Stereo
#define BUFFER_FRAMES 128  // Number of frames in the buffer
#define DURATION 5         // Duration to capture and play (in seconds)

// Function to initialize ALSA for capture or playback
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
    snd_pcm_hw_params_set_channels(pcm_handle, params, channels);
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

int main() {
    snd_pcm_t *capture_handle, *playback_handle;
    short *buffer;
    int frames_to_capture, err;

    // Calculate the total frames to capture and play (sample_rate * duration)
    frames_to_capture = SAMPLE_RATE * DURATION;

    // Initialize ALSA for capture and playback
    capture_handle = init_alsa("default", SND_PCM_STREAM_CAPTURE, SAMPLE_RATE, CHANNELS);
    playback_handle = init_alsa("default", SND_PCM_STREAM_PLAYBACK, SAMPLE_RATE, CHANNELS);
    
    if (!capture_handle || !playback_handle) {
        fprintf(stderr, "Failed to initialize ALSA devices\n");
        return -1;
    }

    // Allocate buffer to hold captured audio data (for 2 channels)
    buffer = (short *)malloc(BUFFER_FRAMES * CHANNELS * sizeof(short));

    // Capture audio and then immediately play it back
    for (int i = 0; i < frames_to_capture / BUFFER_FRAMES; i++) {
        // Capture audio from the microphone
        err = snd_pcm_readi(capture_handle, buffer, BUFFER_FRAMES);
        if (err == -EPIPE) {
            fprintf(stderr, "Buffer overrun during capture\n");
            snd_pcm_prepare(capture_handle);
        } else if (err < 0) {
            fprintf(stderr, "Error capturing audio: %s\n", snd_strerror(err));
        }

        // Play back the captured audio
        err = snd_pcm_writei(playback_handle, buffer, BUFFER_FRAMES);
        if (err == -EPIPE) {
            fprintf(stderr, "Buffer underrun during playback\n");
            snd_pcm_prepare(playback_handle);
        } else if (err < 0) {
            fprintf(stderr, "Error playing audio: %s\n", snd_strerror(err));
        }
    }

    // Free resources and close PCM devices
    free(buffer);
    snd_pcm_drain(playback_handle);
    snd_pcm_drain(capture_handle);
    snd_pcm_close(playback_handle);
    snd_pcm_close(capture_handle);

    return 0;
}
