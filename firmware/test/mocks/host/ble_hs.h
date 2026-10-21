#ifndef _HOST_BLE_HS_H_
#define _HOST_BLE_HS_H_

/*
 * host/ble_hs.h — NimBLE host mock for host-side unit tests.
 *
 * Provides the NimBLE types, macros and functions used by
 * ble_callbacks.c (notifications, indications, mbuf helpers, GATT
 * access context) as host-side stubs so the GATT handler logic can be
 * compiled and tested on host without the NimBLE stack.
 *
 * The mock exposes test-controllable globals so the GATT read/write
 * handlers can be exercised end-to-end:
 *   - g_mock_write_data / g_mock_write_len / g_mock_om_pktlen stage the
 *     payload of an incoming GATT write (ble_handle_write).
 *   - g_mock_read_capture / g_mock_read_capture_len capture the bytes
 *     appended by ble_handle_read() so tests can assert on the response.
 *   - g_mock_mbuf_alloc_ok gates ble_hs_mbuf_from_flat (notify/indicate).
 *   - g_mock_mbuf_append_fail forces os_mbuf_append to fail (error path).
 * All globals default to the original no-op behaviour so pre-existing
 * tests are unaffected.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* Opaque mbuf; ble_callbacks.c only passes pointers to it. */
struct os_mbuf {
    int _unused;
};

/*
 * GATT access context. ble_callbacks.c reads ctxt->op (read vs write)
 * and ctxt->om (the mbuf carrying the value). Only those two fields
 * are referenced, so the mock struct provides exactly those.
 */
struct ble_gatt_access_ctxt {
    struct os_mbuf *om;
    int             op;
};

#define BLE_GATT_ACCESS_OP_READ_CHR    1
#define BLE_GATT_ACCESS_OP_WRITE_CHR   2

#define BLE_ATT_ERR_UNLIKELY               0x0E
#define BLE_ATT_ERR_INSUFFICIENT_RES       0x0D
#define BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN 0x0D

#define BLE_HS_CONN_HANDLE_NONE  0xFFFF
#define BLE_HS_ENOTCONN          12

#define BLE_UUID_TYPE_128  128

/*
 * BLE device address — matches NimBLE's struct ble_addr used in
 * ESP-IDF v5.x.  The .val field holds the 6-byte address and .type
 * holds the address type (public, random, etc.).
 */
struct ble_addr {
    uint8_t type;
    uint8_t val[6];
};

/*
 * GAP connection descriptor — used by ble_gap_conn_find() to look up
 * the peer address for a given connection handle.  The host test mock
 * provides a global mock variable so tests can simulate connected /
 * disconnected / handle-reuse scenarios.
 */
struct ble_gap_conn_desc {
    uint16_t conn_handle;
    struct ble_addr peer_id_addr;
};

/* ============================================================
 * Test-controllable mock state (declared before the inline
 * functions that reference them).  All default to the original
 * no-op behaviour.
 * ============================================================ */

/* Mock state for ble_gap_conn_find().  Default: rc = -1 (not found),
 * so validation fails unless a test sets it. */
static struct ble_gap_conn_desc g_mock_conn_desc = {0};
static int g_mock_conn_find_rc = -1;

/* Announced length of an incoming GATT write (OS_MBUF_PKTLEN). */
static uint16_t g_mock_om_pktlen = 0;

/* Staged write payload for ble_hs_mbuf_to_flat().  When non-NULL, the
 * bytes are copied into the handler's buffer so ble_handle_write()
 * sees the client-written value. */
static const uint8_t *g_mock_write_data = NULL;
static uint16_t g_mock_write_len = 0;

/* Capture buffer for read responses appended via os_mbuf_append(). */
#define MOCK_READ_CAP_MAX 256
static uint8_t g_mock_read_capture[MOCK_READ_CAP_MAX];
static uint16_t g_mock_read_capture_len = 0;

/* Force os_mbuf_append() to fail (read response too large path). */
static bool g_mock_mbuf_append_fail = false;

/* Mock mbuf instance and allocation control for notify/indicate. */
static struct os_mbuf g_mock_mbuf = {0};
static bool g_mock_mbuf_alloc_ok = false;

/* Call counters so tests can assert whether notify/indicate was sent
 * or skipped by the conn-handle validation guard (H4). */
static int g_mock_notify_call_count = 0;
static int g_mock_indicate_call_count = 0;

/* ============================================================
 * Inline mock functions
 * ============================================================ */

static inline int ble_gap_conn_find(uint16_t conn_handle,
                                    struct ble_gap_conn_desc *desc)
{
    (void)conn_handle;
    if (desc) {
        *desc = g_mock_conn_desc;
    }
    return g_mock_conn_find_rc;
}

/* OS_MBUF_PKTLEN reports the staged write length (default 0). */
#define OS_MBUF_PKTLEN(om) ((void)(om), g_mock_om_pktlen)

static inline struct os_mbuf *ble_hs_mbuf_from_flat(const void *data, uint16_t len)
{
    (void)data; (void)len;
    return g_mock_mbuf_alloc_ok ? &g_mock_mbuf : NULL;
}

static inline int ble_gattc_notify_custom(uint16_t conn_handle,
                                          uint16_t attr_handle,
                                          struct os_mbuf *om)
{
    (void)conn_handle; (void)attr_handle; (void)om;
    g_mock_notify_call_count++;
    return 0;
}

static inline int ble_gattc_indicate_custom(uint16_t conn_handle,
                                            uint16_t attr_handle,
                                            struct os_mbuf *om)
{
    (void)conn_handle; (void)attr_handle; (void)om;
    g_mock_indicate_call_count++;
    return 0;
}

/* ble_handle_write() flattens the incoming mbuf into a stack buffer.
 * The mock copies the staged write payload so the handler sees the
 * bytes the client "sent". */
static inline int ble_hs_mbuf_to_flat(struct os_mbuf *om, uint8_t *buf,
                                       uint16_t maxlen, uint16_t *len)
{
    (void)om;
    uint16_t n = g_mock_write_len;
    if (n > maxlen) { n = maxlen; }
    if (n > 0 && g_mock_write_data != NULL) {
        memcpy(buf, g_mock_write_data, n);
    }
    if (len) { *len = n; }
    return 0;
}

/* ble_handle_read() appends the response to the context mbuf.  The mock
 * captures the appended bytes so tests can assert on the exact response. */
static inline int os_mbuf_append(struct os_mbuf *om, const void *data, uint16_t len)
{
    (void)om;
    if (g_mock_mbuf_append_fail) {
        return 1;
    }
    if (len > 0 && data != NULL) {
        uint16_t avail = (uint16_t)(MOCK_READ_CAP_MAX - g_mock_read_capture_len);
        uint16_t n = (len > avail) ? avail : len;
        memcpy(&g_mock_read_capture[g_mock_read_capture_len], data, n);
        g_mock_read_capture_len = (uint16_t)(g_mock_read_capture_len + n);
    }
    return 0;
}

#endif /* _HOST_BLE_HS_H_ */
