"""Validate merged factory boot layout before releasing a build."""
from pathlib import Path
import hashlib
import struct
root = Path(__file__).resolve().parents[1]
merged = root / 'dist/GRID-OS-ES3C28P-v0.1.0-alpha.bin'
app = root / '.pio/build/grid_es3c28p/firmware.bin'
data = merged.read_bytes()
firmware = app.read_bytes()

def require(condition, message):
    if not condition:
        raise RuntimeError(message)

require(data[0] == 0xe9, 'Missing bootloader at 0x0000')
require(data[12:14] == b'\x09\x00', 'Bootloader is not ESP32-S3')
require(data[0xe000:0x10000] == b'\xff' * 8192, 'OTA data would bypass factory launcher')
require(data[0x10000:0x10000+len(firmware)] == firmware, 'Factory app differs from build')
require(firmware[12:14] == b'\x09\x00', 'Application is not ESP32-S3')
require(firmware[32:36] == b'\x32\x54\xcd\xab', 'Missing app descriptor')
require(len(firmware) <= 0x300000, 'Factory app exceeds reserved space')
entries=[]
for address in range(0x8000,0x8c00,32):
    row=data[address:address+32]
    if len(row)<32 or row[:2]!=b'\xaa\x50':break
    _,kind,subtype,offset,size,name,flags=struct.unpack('<HBBII16sI',row)
    entries.append((name.rstrip(b'\0').decode(),kind,subtype,offset,size))
require(('factory',0,0,0x10000,0x300000) in entries, 'Wrong factory partition')
require(('ota_0',0,16,0x310000,0x600000) in entries, 'Wrong payload partition')
for name,kind,sub,offset,size in entries:
    require(offset+size<=0x1000000, 'Partition exceeds flash: '+name)
for first,second in zip(entries,entries[1:]):
    require(first[3]+first[4]<=second[3], 'Partitions overlap')
digest=hashlib.sha256(data).hexdigest()
(merged.parent/'SHA256SUMS.txt').write_text(digest+'  '+merged.name+'\n')
print('Release layout: PASS; '+str(len(data))+' bytes; SHA256 '+digest)
