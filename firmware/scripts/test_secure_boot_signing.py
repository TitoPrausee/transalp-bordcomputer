#!/usr/bin/env python3
"""
test_secure_boot_signing.py — Validate the RSA-3072 Secure Boot signing pipeline.

Tests the actual key pair (not mocks):
  1. Public key (DER) matches the private key
  2. Signed image verifies OK
  3. Tampered image is rejected
  4. Signature is 384 bytes (RSA-3072)
  5. Embedded header key matches the DER file

Usage:
    python3 scripts/test_secure_boot_signing.py [firmware_root]

Exit codes:
    0 — all checks passed
    1 — one or more checks failed
"""

import os
import subprocess
import sys
import tempfile


def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, check=False, **kw)


def main():
    fw_root = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    key_dir = os.path.join(fw_root, "secure_boot")
    pem_key = os.path.join(key_dir, "ota_signing_key.pem")
    der_key = os.path.join(key_dir, "ota_public_key.der")
    header = os.path.join(fw_root, "components", "ota", "include", "ota_public_key_der.h")

    failures = []

    def check(name, condition, detail=""):
        status = "PASS" if condition else "FAIL"
        print(f"  [{status}] {name}" + (f" — {detail}" if detail else ""))
        if not condition:
            failures.append(name)

    print("=== Secure Boot Signing Pipeline Test ===")

    if not os.path.isfile(pem_key):
        print(f"  SKIP — private key not found: {pem_key}")
        print("  (CI without the signing key secret uses the committed header)")
        return 0

    check("Private key exists", os.path.isfile(pem_key))
    check("DER public key exists", os.path.isfile(der_key))
    check("Header file exists", os.path.isfile(header))

    # 1. Key is RSA-3072
    r = run(["openssl", "rsa", "-in", pem_key, "-text", "-noout"])
    check("Private key is valid RSA", r.returncode == 0)
    check("Key is 3072-bit", b"3072 bit" in r.stdout, r.stderr.decode(errors="replace")[:200])

    # 2. DER matches private key
    r_pub = run(["openssl", "rsa", "-in", pem_key, "-pubout", "-outform", "DER"])
    check("Public key extraction succeeds", r_pub.returncode == 0)
    if r_pub.returncode == 0 and os.path.isfile(der_key):
        with open(der_key, "rb") as f:
            der_bytes = f.read()
        check("DER file matches private key", der_bytes == r_pub.stdout,
              f"file={len(der_bytes)}B extracted={len(r_pub.stdout)}B")
        check("DER is ~422 bytes (RSA-3072)", 400 <= len(der_bytes) <= 450,
              f"{len(der_bytes)} bytes")

    # 3. Header matches DER
    if os.path.isfile(header) and os.path.isfile(der_key):
        import re
        with open(header) as f:
            txt = f.read()
        hex_vals = re.findall(r"0x([0-9a-f]{2})", txt)
        header_bytes = bytes(int(h, 16) for h in hex_vals)
        with open(der_key, "rb") as f:
            der_bytes = f.read()
        check("Header bytes match DER file", header_bytes == der_bytes,
              f"header={len(header_bytes)}B der={len(der_bytes)}B")

    # 4. Sign + verify pipeline
    # Extract public key to a temp PEM file for verification
    r_pubout = run(["openssl", "rsa", "-in", pem_key, "-pubout", "-outform", "PEM"])
    check("Public key PEM extraction succeeds", r_pubout.returncode == 0)
    pub_pem_fd, pub_pem_path = tempfile.mkstemp(suffix=".pub")
    if r_pubout.returncode == 0:
        with os.fdopen(pub_pem_fd, "wb") as f:
            f.write(r_pubout.stdout)
    else:
        os.close(pub_pem_fd)

    img_fd, img_path = tempfile.mkstemp(suffix=".bin")
    with os.fdopen(img_fd, "wb") as f:
        f.write(b"test firmware image payload for RSA-3072 signing")
    sig_path = img_path + ".sig"
    try:
        r_sign = run(["openssl", "dgst", "-sha256", "-sign", pem_key,
                       "-out", sig_path, img_path])
        check("Signing succeeds", r_sign.returncode == 0)

        sig_size = os.path.getsize(sig_path) if os.path.isfile(sig_path) else 0
        check("Signature is 384 bytes (RSA-3072)", sig_size == 384,
              f"{sig_size} bytes")

        r_verify = run(["openssl", "dgst", "-sha256",
                         "-verify", pub_pem_path,
                         "-signature", sig_path, img_path])
        check("Valid signature verifies OK", r_verify.returncode == 0,
              r_verify.stderr.decode(errors="replace")[:200])

        # 5. Tampered image rejected
        with open(img_path, "w") as f:
            f.write("tampered payload")
        r_tamper = run(["openssl", "dgst", "-sha256",
                         "-verify", pub_pem_path,
                         "-signature", sig_path, img_path])
        check("Tampered image rejected", r_tamper.returncode != 0,
              "tampered image was incorrectly accepted!" if r_tamper.returncode == 0 else "")
    finally:
        for p in (img_path, sig_path, pub_pem_path):
            if os.path.exists(p):
                os.unlink(p)

    print(f"\n{'='*40}")
    if failures:
        print(f"FAIL — {len(failures)} check(s) failed: {', '.join(failures)}")
        return 1
    print("ALL CHECKS PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
