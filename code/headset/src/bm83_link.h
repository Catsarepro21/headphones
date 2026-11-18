#ifndef BM83_LINK_H
#define BM83_LINK_H

#include <stdbool.h>

enum bm83_media_control {
    BM83_MEDIA_PLAY_PAUSE,
    BM83_MEDIA_VOLUME_DOWN,
    BM83_MEDIA_VOLUME_UP,
};

int bm83_start(void);
void bm83_poll(void);
void bm83_set_selected(bool selected);
void bm83_request_pairing(void);
void bm83_request_media(enum bm83_media_control control);
bool bm83_audio_available(void);

#endif
