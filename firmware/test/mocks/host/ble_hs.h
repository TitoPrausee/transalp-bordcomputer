#ifndef _HOST_BLE_HS_H_
#define _HOST_BLE_HS_H_

/*
 * host/ble_hs.h — NimBLE host mock for host-side unit tests.
 *
 * Provides the NimBLE types, macros and functions used by
 * ble_callbacks.c (notifications, indications, mbuf helpers) as
 * no-ops so the GATT handler logic can be compiled and tested on
 * host without the NimBLE stack.
 */

#include <stdint.h>
#include <stddef.h>

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

/*
 * Mock state for ble_gap_conn_find().  Tests set g_mock_conn_desc and
 * g_mock_conn_find_rc before exercising code that calls ble_gap_conn_find.
 * Default: rc = -1 (not found), so validation fails unless a test sets it.
 */
static struct ble_gap_conn_desc g_mock_conn_desc = {0};
static int g_mock_conn_find_rc = -1;

static inline int ble_gap_conn_find(uint16_t conn_handle,
                                    struct ble_gap_conn_desc *desc)
{
    (void)conn_handle;
    if (desc) {
        *desc = g_mock_conn_desc;
    }
    return g_mock_conn_find_rc;
}

/* Host mock does not model mbuf length. */
#define OS_MBUF_PKTLEN(om) ((uint16_t)0)

/* Mock mbuf instance and allocation control.  Tests set
 * g_mock_mbuf_alloc_ok = true to let ble_hs_mbuf_from_flat return a
 * non-NULL mbuf so the full notify/indicate path is exercised. */
static struct os_mbuf g_mock_mbuf = {0};
static bool g_mock_mbuf_alloc_ok = false;

static inline struct os_mbuf *ble_hs_mbuf_from_flat(const void *data, uint16_t len)
{
    (void)data; (void)len;
    return g_mock_mbuf_alloc_ok ? &g_mock_mbuf : NULL;
}

/* Call counters so tests can assert whether notify/indicate was sent
 * or skipped by the conn-handle validation guard (H4). */
static int g_mock_notify_call_count = 0;
static int g_mock_indicate_call_count = 0;

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

static inline int ble_hs_mbuf_to_flat(struct os_mbuf *om, uint8_t *buf,
                                      uint16_t maxlen, uint16_t *len)
{
    (void)om; (void)buf; (void)maxlen;
    if (len) { *len = 0; }
    return 0;
}

static inline int os_mbuf_append(struct os_mbuf *om, const void *data, uint16_t len)
{
    (void)om; (void)data; (void)len;
    return 0;
}

#endif /* _HOST_BLE_HS_H_ */
