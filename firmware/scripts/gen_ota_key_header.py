#!/usr/bin/env python3
"""
gen_ota_key_header.py — Build-time code generation for OTA Secure Boot public key.

Reads an RSA-3072 PEM signing key, extracts the public key in DER format,
and emits a C header with the key bytes for embedding into the firmware.

Usage:
    python3 gen_ota_key_header.py <signing_key.pem> <output_header.h>

The generated header defines:
    static const uint8_t ota_public_key_der[]

This key is used by ota_secure_boot.c for application-layer RSA signature
verification before reboot. The bootloader performs its own independent
Secure Boot V2 verification using the eFuse-embedded key digest.

Exit codes:
    0 — success
    1 — invalid arguments
    2 — openssl not found
    3 — key extraction failed
    4 — header write failed
"""

import os
import subprocess
import sys


HEADER_TEMPLATE = """\
/*
 * ota_public_key_der.h — AUTO-GENERATED, DO NOT EDIT
 *
 * RSA-3072 public key (DER-encoded) for OTA signature verification.
 * Generated at build time from: {key_path}
 *
 * This file is produced by scripts/gen_ota_key_header.py.
 * Edit the signing key, not this header.
 */

#ifndef _OTA_PUBLIC_KEY_DER_H_
#define _OTA_PUBLIC_KEY_DER_H_

#include <stdint.h>

__attribute__((aligned(4)))
static const uint8_t ota_public_key_der[] = {{
{body}
}};

#define OTA_PUBLIC_KEY_DER_LEN  ({length}u)

#endif /* _OTA_PUBLIC_KEY_DER_H_ */
"""


def extract_public_der(pem_path: str) -> bytes:
    """Extract the RSA public key in DER format from a PEM signing key."""
    result = subprocess.run(
        [
            "openssl", "rsa",
            "-in", pem_path,
            "-pubout",
            "-outform", "DER",
        ],
        capture_output=True,
        check=False,
    )
    if result.returncode != 0:
        sys.stderr.write(
            f"openssl rsa failed (exit {result.returncode}):\n"
            f"{result.stderr.decode('utf-8', errors='replace')}"
        )
        sys.exit(3)
    return result.stdout


def format_der_bytes(data: bytes, bytes_per_line: int = 12) -> str:
    """Format DER bytes as a C array initializer."""
    lines = []
    for i in range(0, len(data), bytes_per_line):
        chunk = data[i : i + bytes_per_line]
        hex_vals = ", ".join(f"0x{b:02x}" for b in chunk)
        lines.append(f"    {hex_vals},")
    return "\n".join(lines)


def validate_rsa_key_size(data: bytes) -> None:
    """Warn if the key size doesn't look like RSA-3072 (DER ~ 422 bytes)."""
    if len(data) < 300 or len(data) > 600:
        sys.stderr.write(
            f"WARNING: DER public key is {len(data)} bytes — "
            f"expected ~422 for RSA-3072. Verify the signing key.\n"
        )


def main() -> None:
    if len(sys.argv) != 3:
        sys.stderr.write(
            f"Usage: {sys.argv[0]} <signing_key.pem> <output_header.h>\n"
        )
        sys.exit(1)

    pem_path = sys.argv[1]
    out_path = sys.argv[2]

    if not os.path.isfile(pem_path):
        sys.stderr.write(f"Signing key not found: {pem_path}\n")
        sys.exit(3)

    der_bytes = extract_public_der(pem_path)
    validate_rsa_key_size(der_bytes)

    body = format_der_bytes(der_bytes)
    header = HEADER_TEMPLATE.format(
        key_path=os.path.basename(pem_path),
        body=body,
        length=len(der_bytes),
    )

    out_dir = os.path.dirname(out_path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    try:
        with open(out_path, "w") as f:
            f.write(header)
    except OSError as exc:
        sys.stderr.write(f"Failed to write header: {exc}\n")
        sys.exit(4)

    print(
        f"Generated {out_path}: {len(der_bytes)} bytes "
        f"from {os.path.basename(pem_path)}"
    )


if __name__ == "__main__":
    main()
