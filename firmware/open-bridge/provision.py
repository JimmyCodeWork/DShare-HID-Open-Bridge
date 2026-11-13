#!/usr/bin/env python3
"""Provision one freely flashable ESP32-C3; retain only its verified public key."""
import argparse
import os
from pathlib import Path
import re
import secrets
import shutil
import struct
import subprocess
import sys
import tempfile
import time

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec, utils
import serial

HERE = Path(__file__).resolve().parent
TRUST = Path.home() / '.dshare-hid' / 'bridge-keys'


def read_exact(port, length):
    data = bytearray()
    deadline = time.monotonic() + 5
    while len(data) < length and time.monotonic() < deadline:
        data.extend(port.read(length - len(data)))
    if len(data) != length:
        raise RuntimeError('Timed out waiting for the bridge; check the port and reset the board')
    return bytes(data)


def send(port, payload):
    port.write(b'\x5a\xc3\x01\x80\x00\x00' + struct.pack('<H', len(payload)) + payload)
    port.flush()


def receive(port):
    deadline = time.monotonic() + 5
    previous = b''
    while time.monotonic() < deadline:
        current = port.read(1)
        if not current:
            continue
        if previous == b'\x5a' and current == b'\xc3':
            header = read_exact(port, 6)
            length = int.from_bytes(header[4:6], 'little')
            if header[:4] != b'\x01\x80\x00\x00' or length > 256:
                raise RuntimeError('Unexpected bridge frame')
            return read_exact(port, length)
        previous = current
    raise RuntimeError('No response from bridge')


def verify_and_identify(port_name, public_key):
    with serial.Serial(port_name, 115200, timeout=0.2, write_timeout=2) as port:
        time.sleep(1)
        port.reset_input_buffer()
        nonce = secrets.token_bytes(32)
        send(port, b'\x01\x01\x02' + nonce + bytes(64))
        ack = receive(port)
        if len(ack) != 113 or ack[0] != 0x81 or ack[1] != 1:
            raise RuntimeError('Unexpected handshake response')
        signature = utils.encode_dss_signature(int.from_bytes(ack[49:81], 'big'),
                                               int.from_bytes(ack[81:113], 'big'))
        public_key.verify(signature, nonce + ack[17:49], ec.ECDSA(hashes.SHA256()))
        send(port, b'\x04')
        reply = receive(port)
        if reply[:3] != b'\x82\x04\x00':
            raise RuntimeError('Bridge did not return a serial number')
        name = reply[3:].decode('ascii')
        if not re.fullmatch(r'Open-[0-9A-F]{12}', name):
            raise RuntimeError('Invalid bridge serial number')
        return name


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', required=True, help='ESP32 serial port, for example COM3')
    args = parser.parse_args()
    private = ec.generate_private_key(ec.SECP256R1())
    public = private.public_key()
    public_bytes = public.public_bytes(serialization.Encoding.X962, serialization.PublicFormat.UncompressedPoint)
    # A private staging directory holds source, key and build outputs and is removed on exit.
    with tempfile.TemporaryDirectory(prefix='dshare-provision-') as tmp:
        root = Path(tmp)
        project = root / 'firmware'
        shutil.copytree(HERE, project, ignore=shutil.ignore_patterns('.pio', '__pycache__', 'tests'))
        key_file = root / 'private-key.pem'
        key_file.write_bytes(private.private_bytes(serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8,
                                                   serialization.NoEncryption()))
        env = os.environ.copy()
        env['DSH_BRIDGE_PRIVATE_KEY'] = str(key_file)
        command = [sys.executable, '-m', 'platformio', 'run', '-d', str(project)]
        print('Building firmware...', flush=True)
        subprocess.run(command, check=True, env=env)
        print('Flashing the selected board...', flush=True)
        subprocess.run(command + ['-t', 'upload', '--upload-port', args.port], check=True, env=env)
        print('Verifying the new device identity...', flush=True)
        serial_number = verify_and_identify(args.port, public)
        TRUST.mkdir(parents=True, exist_ok=True)
        # Store the new trust anchor atomically only after the signed handshake succeeds.
        with tempfile.NamedTemporaryFile(dir=TRUST, prefix='.new-', delete=False) as pending:
            pending_path = Path(pending.name)
            try:
                pending.write(public_bytes)
                pending.flush()
                os.fsync(pending.fileno())
            except BaseException:
                pending.close()
                pending_path.unlink(missing_ok=True)
                raise
        try:
            os.replace(pending_path, TRUST / (serial_number + '.bin'))
        finally:
            pending_path.unlink(missing_ok=True)
        print(f'Verified {serial_number}; public key saved. Temporary build files will be removed.')
    print('Provisioning complete. Ordinary file deletion does not guarantee secure erasure.')


if __name__ == '__main__':
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError, ValueError, InvalidSignature) as exc:
        print(f'Provisioning failed: {exc}. Existing trusted keys were not intentionally changed.', file=sys.stderr)
        sys.exit(1)
