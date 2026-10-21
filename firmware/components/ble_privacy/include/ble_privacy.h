#ifndef _BLE_PRIVACY_H_
#define _BLE_PRIVACY_H_

#include <stdint.h>
#include <stdbool.h>

/*
 * ble_privacy.h — BLE address privacy (RPA + IRK distribution).
 *
 * The device advertises and connects with a Resolvable Private Address
 * (RPA) derived from the device IRK.  The RPA rotates at the interval set by
 * CONFIG_BT_NIMBLE_RPA_TIMEOUT, so a static BLE address can no longer be used
 * to track the device across sessions.  Bonded peers receive the IRK and the
 * device's static random identity address during pairing
 * (BLE_SM_PAIR_KEY_DIST_ID) and use the IRK to resolve the device's current
 * RPA.
 *
 * Responsibility split:
 *  - This module owns the device's STATIC RANDOM IDENTITY ADDRESS: it
 *    generates and persists it in NVS (so bonded peers' stored identity stays
 *    valid across reboots) and registers it with the NimBLE host via the
 *    public ble_hs_id_set_rnd() API.
 *  - NimBLE owns the device IRK: it generates and persists the IRK in the bond
 *    store (CONFIG_BT_NIMBLE_NVS_PERSIST=y) automatically during host startup.
 *    The application only configures IRK DISTRIBUTION via
 *    ble_privacy_configure() so bonded peers receive it.
 *
 * This module is a leaf dependency shared by the BLE GATT server
 * (ble_service.c, advertising) and the CarPlay central client
 * (carplay_client.c, connection initiation) so both paths use the same
 * privacy-aware own address type without a circular component dependency.
 *
 * Lifecycle:
 *   1. ble_privacy_configure(&ble_hs_cfg) — before nimble_port_init(), to
 *      set the IRK/identity key-distribution knobs on the host config.
 *   2. ble_privacy_init()                — after nimble_port_init(), to load
 *      or generate + persist the static random identity address in NVS.  No
 *      HCI commands are issued here (the controller is not synced yet).
 *   3. ble_privacy_own_addr_type()       — passed to ble_gap_adv_start() /
 *      ble_gap_connect().  On first call (in the host-controller sync context)
 *      it registers the identity address with the host via
 *      ble_hs_id_set_rnd() and infers the RPA own address type via
 *      ble_hs_id_infer_auto(1).  Falls back to BLE_OWN_ADDR_PUBLIC on any
 *      failure so advertising/connecting still work (degraded, non-private).
 *
 * See docs/BLE-INTERFACE-SPEC.md §1.1 and SECURITY.md.
 */

/* Forward-declare the NimBLE host config so this public header does not need
 * to pull in the full NimBLE host headers.  The full type is available where
 * ble_privacy.c is compiled (via host/ble_hs.h).  Callers that already include
 * host/ble_hs.h (e.g. ble_service.c, carplay_client.c) see the real type. */
struct ble_hs_cfg;

/*
 * Own-address type and key-distribution identifiers (BLE_OWN_ADDR_*,
 * BLE_SM_PAIR_KEY_DIST_*) are intentionally NOT redefined here — they come
 * from the NimBLE headers (host/ble_hs.h) that callers already include.
 * ble_privacy_own_addr_type() returns a BLE_OWN_ADDR_* value; see the NimBLE
 * header or the implementation comments for the meaning of each value.
 */

/*
 * Configure the privacy-related knobs on a NimBLE host config struct:
 *  - distribute our IRK + identity address (BLE_SM_PAIR_KEY_DIST_ID) and LTK
 *    (BLE_SM_PAIR_KEY_DIST_ENC) to bonded peers, so they can resolve our RPA
 *    after bonding;
 *  - request the peer's IRK + identity address and LTK, so we can resolve the
 *    peer's RPA too.
 *
 * Idempotent: ORs the flags into the existing mask, so callers can layer
 * additional key-distribution flags.  Safe to call with NULL (no-op).
 */
void ble_privacy_configure(struct ble_hs_cfg *cfg);

/*
 * Initialise BLE privacy at runtime:
 *  - load or generate a static random identity address, persisted in NVS so
 *    the same identity survives reboots (bonded peers keep resolving);
 *  - prepare it for registration with the NimBLE host.
 *
 * Must run after nimble_port_init().  Does NOT issue HCI commands — the actual
 * registration (ble_hs_id_set_rnd) happens lazily in ble_privacy_own_addr_type()
 * within the host-controller sync context.  On NVS failure a volatile address
 * is generated so privacy still works for the current session.
 */
void ble_privacy_init(void);

/*
 * Own address type to pass to ble_gap_adv_start() / ble_gap_connect().
 * Returns BLE_OWN_ADDR_RPA_RANDOM_DEFAULT once the identity address has been
 * registered and the host inferred an RPA own address type, or
 * BLE_OWN_ADDR_PUBLIC if ble_privacy_init() has not run or registration /
 * inference failed.
 */
uint8_t ble_privacy_own_addr_type(void);

/*
 * True once the device is advertising/connecting with a non-public address
 * (RPA active).  Used by host unit tests and runtime diagnostics.
 */
bool ble_privacy_is_enabled(void);

#endif /* _BLE_PRIVACY_H_ */
