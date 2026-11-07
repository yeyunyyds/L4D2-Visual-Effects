"""Verify uploaded game files offline. Never loads or executes the DLLs."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from pe_reader import PE

parser = argparse.ArgumentParser(description=__doc__)
for name in ('client', 'engine', 'studio'):
    parser.add_argument('--' + name)
args = parser.parse_args()
profiles = json.loads((Path(__file__).resolve().parents[1] / 'docs/research/render-profile.json').read_text())
if not any(vars(args).values()):
    parser.error('Provide at least one DLL path')
for name, profile in profiles.items():
    if not getattr(args, name):
        continue
    if name == 'studio':
        original = json.loads((Path(__file__).resolve().parents[1] / 'docs/research/studio-original-profile.json').read_text())
        if hashlib.sha256(Path(args.studio).read_bytes()).hexdigest() == original['sha256']:
            profile = original
    image = PE(getattr(args, name))
    assert hashlib.sha256(image.data).hexdigest() == profile['sha256'], name + ' SHA mismatch'
    assert (image.timestamp, image.image_size) == (profile['timestamp'], profile['image_size'])
    for sample in profile['samples']:
        assert image.read(sample['rva'], len(bytes.fromhex(sample['bytes']))) == bytes.fromhex(sample['bytes'])
    if 'factory_rva' in profile:
        assert image.read(profile['factory_rva'], 6) == b'\xb8' + struct.pack('<I', image.base + profile['object_rva']) + b'\xc3'
        assert image.pointer(profile['vtable_rva'] + profile['slot'] * 4) == image.base + profile['method_rva']
        # Both global objects acquire their vtable in a runtime constructor.
        code = image.read(profile['constructor_rva'], 6)
        assert code[:2] in (b'\xc7\x00', b'\xc7\x06')
        assert struct.unpack('<I', code[2:])[0] == image.base + profile['vtable_rva']
    if 'compatibility' in profile:
        c = profile['compatibility']
        assert image.read(c['name_rva'], 17) == b'cl_modelfastpath\0'
        assert image.pointer(c['vtable_rva'] + c['setter_slot'] * 4) == image.base + c['setter_rva']
        assert image.read(c['registration_rva'] + 7, 5) == b'\x68' + struct.pack('<I', image.base + c['name_rva'])
        assert image.read(c['consumer_rva'], 6) == b'\x8b\x15' + struct.pack('<I', image.base + c['object_rva'] + 28)
        assert image.read(c['setter_rva'] + 0x17, 3) == b'\x3b\x7e\x30'
    print('PASS:', name, 'SHA/PE/layout/instruction evidence')
