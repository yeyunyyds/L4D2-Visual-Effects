"""Static PE validation of the supported game build; never loads/executes a DLL.
PE reader adapted from LDBDangoMusic verify_source_binaries.py (GPL-3.0).
Uses only Python standard library. Game DLLs are not bundled.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path


class PE:
    def __init__(self, path):
        self.data = Path(path).read_bytes()
        if self.data[:2] != b'MZ':
            raise ValueError('Not a PE')
        pe = self.u32(0x3c)
        if self.data[pe:pe + 4] != b'PE\0\0' or self.u16(pe + 4) != 0x14c:
            raise ValueError('Expected x86 PE')
        self.timestamp = self.u32(pe + 8)
        optional = pe + 24
        self.base = self.u32(optional + 28)
        self.image_size = self.u32(optional + 56)
        first = optional + self.u16(pe + 20)
        self.sections = []
        for i in range(self.u16(pe + 6)):
            offset = first + i * 40
            virtual_size, rva, size, raw = struct.unpack_from('<4I', self.data, offset + 8)
            self.sections.append((rva, max(virtual_size, size), raw, size))

    def u16(self, offset):
        return struct.unpack_from('<H', self.data, offset)[0]

    def u32(self, offset):
        return struct.unpack_from('<I', self.data, offset)[0]

    def read(self, rva, size):
        for start, length, raw, raw_size in self.sections:
            if start <= rva and rva + size <= start + raw_size:
                return self.data[raw + rva - start:raw + rva - start + size]
        raise ValueError(f'Unmapped file RVA {rva:x}')

    def pointer(self, rva):
        return struct.unpack('<I', self.read(rva, 4))[0]

