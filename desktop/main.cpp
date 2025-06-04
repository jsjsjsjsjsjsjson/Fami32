#include <SDL2/SDL.h>
#include "ftm_file.h"
#include "fami32_player.h"

bool _debug_print = false;
bool _midi_output = false;
bool edit_mode = false;
int g_vol = 16;

FAMI_PLAYER player;

static void audio_callback(void* userdata, Uint8* stream, int len) {
    int16_t* out = reinterpret_cast<int16_t*>(stream);
    int samples = len / sizeof(int16_t);
    while (samples > 0) {
        player.process_tick();
        int to_copy = player.get_buf_size();
        if (to_copy > samples) to_copy = samples;
        memcpy(out, player.get_buf(), to_copy * sizeof(int16_t));
        out += to_copy;
        samples -= to_copy;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <module.ftm>\n", argv[0]);
        return 1;
    }

    if (ftm.open_ftm(argv[1]) || ftm.read_ftm_all()) {
        fprintf(stderr, "Failed to load %s\n", argv[1]);
        return 1;
    }

    player.init(&ftm);
    player.start_play();

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Fami32 Desktop", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, 0);
    SDL_AudioSpec want{}, have{};
    want.freq = SAMP_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = player.get_buf_size();
    want.callback = audio_callback;
    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!dev) {
        fprintf(stderr, "SDL audio error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_PauseAudioDevice(dev, 0);

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
        }
        SDL_Delay(10);
    }

    SDL_CloseAudioDevice(dev);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
