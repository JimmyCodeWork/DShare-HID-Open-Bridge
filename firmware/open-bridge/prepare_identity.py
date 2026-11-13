"""Embed the developer's private key in this local firmware build only."""
import os
from pathlib import Path

Import("env")  # type: ignore[name-defined]  # supplied by PlatformIO

key_path = os.environ.get("DSH_BRIDGE_PRIVATE_KEY")
if not key_path:
    raise RuntimeError("Set DSH_BRIDGE_PRIVATE_KEY to your private-key.pem before building")
key = Path(key_path).read_bytes()
if b"-----BEGIN PRIVATE KEY-----" not in key:
    raise RuntimeError("Expected an unencrypted PKCS#8 private PEM key")
build_dir = Path(env.subst("$BUILD_DIR"))  # type: ignore[name-defined]
build_dir.mkdir(parents=True, exist_ok=True)
header = build_dir / "bridge_identity.h"
header.write_text("#pragma once\nstatic const unsigned char bridge_private_key[] = {" +
                  ",".join(str(x) for x in key + b"\0") + "};\n")
env.Append(CPPPATH=[str(build_dir)])  # type: ignore[name-defined]
