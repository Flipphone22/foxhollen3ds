"""Losslessly prepare a private random-access RAM cache; original file is untouched."""
import argparse,ctypes,hashlib,json,struct,zlib
from pathlib import Path
import ctypes.util
from functools import lru_cache

@lru_cache(maxsize=1)
def lz4_library():
    path=ctypes.util.find_library('lz4')
    if not path:raise RuntimeError('Run LZ4 preparation in the Linux toolchain with liblz4')
    library=ctypes.CDLL(path)
    library.LZ4_compress_default.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int]
    library.LZ4_decompress_safe.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int]
    library.LZ4_versionNumber.restype=ctypes.c_int
    return library

CHUNK=65536
def encode(raw,level=0):
    if not raw or len(raw)>64*1024*1024:raise ValueError('Prepared source size outside bounds')
    if level not in (0,*range(3,13)):raise ValueError('LZ4 level outside supported range')
    lib=lz4_library();directory=bytearray();payload=bytearray()
    if level:
        lib.LZ4_compress_HC.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_int]
        lib.LZ4_compress_HC.restype=ctypes.c_int
    count=(len(raw)+CHUNK-1)//CHUNK;base=32+count*16
    scratch=ctypes.create_string_buffer(CHUNK+512)
    for at in range(0,len(raw),CHUNK):
        block=raw[at:at+CHUNK]
        n=lib.LZ4_compress_HC(block,scratch,len(block),len(scratch),level) if level else lib.LZ4_compress_default(block,scratch,len(block),len(scratch))
        if not n:raise RuntimeError('LZ4 compression failed')
        mode=int(n<len(block));data=scratch.raw[:n] if mode else block
        directory+=struct.pack('<4I',base+len(payload),len(data),zlib.crc32(block),mode)
        payload+=data
    return b'FHBCACH1'+struct.pack('<6I',len(raw),count,base+len(payload),zlib.crc32(directory),zlib.crc32(raw),0)+directory+payload

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);p.add_argument('output',type=Path)
    p.add_argument('--level',type=int,choices=(0,*range(3,13)),default=0,help='Offline HC effort; same bounded runtime block decoder, zero selects fast encoder')
    a=p.parse_args()
    if a.source.resolve()==a.output.resolve():p.error('Cache must not replace the source file')
    raw=a.source.read_bytes();cache=encode(raw,a.level);a.output.parent.mkdir(parents=True,exist_ok=True)
    tmp=a.output.with_suffix(a.output.suffix+'.tmp');tmp.write_bytes(cache);tmp.replace(a.output)
    report={'source':str(a.source),'source_bytes':len(raw),'source_sha256':hashlib.sha256(raw).hexdigest(),
            'cache_bytes':len(cache),'allocation_bytes':len(cache)+CHUNK,'cache_sha256':hashlib.sha256(cache).hexdigest(),
            'lz4_version':lz4_library().LZ4_versionNumber(),'compression_level':a.level,'scope':__doc__}
    a.output.with_suffix(a.output.suffix+'.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
