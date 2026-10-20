/*
 * carplay_media.c — Lenker-Controller event -> CarPlay media-command mapping
 *
 * Pure translation table (no NimBLE / FreeRTOS dependencies) so it can be
 * unit-tested on the host with GCC.  The bridge subscriber
 * (carplay_bridge.c) calls carplay_media_cmd_from_event() and forwards a
 * non-zero result to carplay_client_media_ctrl(), which writes the byte to
 * the peer display's CHR_CP_MEDIA_CTRL (0x9104).
 *
 * Closes gap C4 / FU-4 (D2-1, D2-4).
 */

#include "carplay_media.h"

uint8_t carplay_media_cmd_from_event(app_event_type_t type)
{
    switch (type) {
    case APP_EVENT_CTRL_MEDIA_NEXT:
        return CARPLAY_MEDIA_CMD_NEXT;
    case APP_EVENT_CTRL_MEDIA_PREV:
        return CARPLAY_MEDIA_CMD_PREV;
    default:
        return 0;
    }
}
