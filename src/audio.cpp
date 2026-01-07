#include "audio.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

static ma_engine gEngine;
static ma_sound gMusic;
static bool gAudioReady = false;

// Inicializa o engine de audio e comeca a tocar a musica de fundo
bool InitAudioEngine(const char *musicPath) {
  // Evita reinicializacao e valida caminho
  if (gAudioReady || !musicPath) {
    return true;
  }

  // Configura o engine e cria o stream de musica
  ma_engine_config cfg = ma_engine_config_init();
  if (ma_engine_init(&cfg, &gEngine) != MA_SUCCESS) {
    return false;
  }

  // Carrega musica de fundo
  ma_result res = ma_sound_init_from_file(&gEngine, musicPath, MA_SOUND_FLAG_STREAM, nullptr, nullptr, &gMusic);
 
  // se falhar a carregar a musica, desinicializa o engine
  if (res != MA_SUCCESS) {
    ma_engine_uninit(&gEngine);
    return false;
  }

  // Loop continuo da musica
  ma_sound_set_looping(&gMusic, MA_TRUE);
  ma_sound_start(&gMusic);
  gAudioReady = true;
  return true;
}

void ShutdownAudioEngine() {
  // Liberta recursos de audio
  if (!gAudioReady) {
    return;
  }
  ma_sound_uninit(&gMusic);
  ma_engine_uninit(&gEngine);
  gAudioReady = false;
}
