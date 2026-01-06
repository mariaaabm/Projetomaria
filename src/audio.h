#pragma once

// Inicializa o motor de áudio e começa a tocar a música em loop.
// Retorna true em sucesso.
bool InitAudioEngine(const char *musicPath);

// Liberta recursos do motor de áudio.
void ShutdownAudioEngine();
