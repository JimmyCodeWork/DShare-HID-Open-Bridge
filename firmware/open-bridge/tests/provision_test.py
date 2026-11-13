import importlib.util
from pathlib import Path
import unittest
import tempfile
from unittest.mock import patch

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ec, utils

spec = importlib.util.spec_from_file_location('provision', Path(__file__).resolve().parents[1] / 'provision.py')
provision = importlib.util.module_from_spec(spec)
spec.loader.exec_module(provision)

class FakeSerial:
    def __init__(self, key, *args, **kwargs):
        self.key = key
        self.buffer = bytearray()
    def __enter__(self): return self
    def __exit__(self, *args): pass
    def reset_input_buffer(self): pass
    def flush(self): pass
    def read(self, size):
        out = self.buffer[:size]
        del self.buffer[:size]
        return bytes(out)
    def write(self, packet):
        body = packet[8:]
        if body[0] == 1:
            nonce = b'x' * 32
            signature = self.key.sign(body[3:35] + nonce, ec.ECDSA(hashes.SHA256()))
            r, s = utils.decode_dss_signature(signature)
            reply = bytes([0x81, 1]) + bytes(15) + nonce + r.to_bytes(32, 'big') + s.to_bytes(32, 'big')
        else:
            reply = b'\x82\x04\x00Open-0123456789AB'
        self.buffer.extend(b'\x5a\xc3\x01\x80\x00\x00' + len(reply).to_bytes(2, 'little') + reply)
        return len(packet)

class ProvisionTest(unittest.TestCase):
    def test_matching_key_and_serial(self):
        key = ec.generate_private_key(ec.SECP256R1())
        with patch.object(provision.serial, 'Serial', lambda *a, **kw: FakeSerial(key)), patch.object(provision.time, 'sleep'):
            self.assertEqual(provision.verify_and_identify('COM3', key.public_key()), 'Open-0123456789AB')
    def test_wrong_key_rejected(self):
        key = ec.generate_private_key(ec.SECP256R1())
        other = ec.generate_private_key(ec.SECP256R1())
        with patch.object(provision.serial, 'Serial', lambda *a, **kw: FakeSerial(key)), patch.object(provision.time, 'sleep'):
            with self.assertRaises(InvalidSignature):
                provision.verify_and_identify('COM3', other.public_key())
    def test_rotation_commits_only_after_verification(self):
        with tempfile.TemporaryDirectory() as tmp:
            trust = Path(tmp) / 'trust'
            trust.mkdir()
            existing = trust / 'Open-0123456789AB.bin'
            existing.write_bytes(b'old key')
            with patch.object(provision, 'TRUST', trust), patch.object(provision.sys, 'argv', ['provision.py', '--port', 'COM3']), \
                 patch.object(provision.subprocess, 'run'), patch.object(provision, 'verify_and_identify', side_effect=RuntimeError('bad handshake')):
                with self.assertRaises(RuntimeError): provision.main()
                self.assertEqual(existing.read_bytes(), b'old key')
            with patch.object(provision, 'TRUST', trust), patch.object(provision.sys, 'argv', ['provision.py', '--port', 'COM3']), \
                 patch.object(provision.subprocess, 'run'), patch.object(provision, 'verify_and_identify', return_value='Open-0123456789AB'):
                provision.main()
                self.assertEqual(len(existing.read_bytes()), 65)
                self.assertEqual(existing.read_bytes()[0], 4)

if __name__ == '__main__': unittest.main()
