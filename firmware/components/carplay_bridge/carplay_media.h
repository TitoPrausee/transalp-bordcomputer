#ifndef _CARPLAY_MEDIA_H_
#define _CARPLAY_MEDIA_H_

/*
 * carplay_media.h — Lenker-Controller event -> CarPlay media-command mapping
 *
 * Pure, host-testable mapping used by the CarPlay bridge to translate
 * APP_EVENT_CTRL_MEDIA_* events (dispatched by the handlebar controller onto
 * the payload event loop) into the single command byte expected by the
 * CarPlay display's CHR_CP_MEDIA_CTRL (0x9104) characteristic.
 *
 * See docs/CARPLAY-INTEGRATION.md and docs/INTEGRATION-TEST-REPORT.md
 * (gaps C4 / D2-1 / D2-4, follow-up FU-4).
 */

#include <stdint.h>
#include "app_event_loop.h"   /* app_event_type_t */
#include "carplay_uuids.h"    /* enum carplay_media_cmd */

/*
 * Map an app_event_type_t controller event to a carplay_media_cmd byte.
 * Returns 0 when the event has no CarPlay media-command mapping (caller
 * should ignore the event).  Currently mapped:
 *   APP_EVENT_CTRL_MEDIA_NEXT -> CARPLAY_MEDIA_CMD_NEXT  (0x02)
 *   APP_EVENT_CTRL_MEDIA_PREV -> CARPLAY_MEDIA_CMD_PREV  (0x03)
 *
 * Volume up/down have no counterpart in the display's media-control
 * protocol (enum carplay_media_cmd defines no volume command), so they
 * are intentionally unmapped.
 */
uint8_t carplay_media_cmd_from_event(app_event_type_t type);

#endif /* _CARPLAY_MEDIA_H_ */
