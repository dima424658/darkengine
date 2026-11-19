#!/usr/bin/env python3
"""Linux: exercise DDS/PBR discovery through real Gold terrain and model resources.

Creates a private overlay and saves; never writes into the supplied game tree.
"""
import argparse
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import zlib

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('game_path', type=Path)
parser.add_argument('--build', type=Path, default=root/'build-linux')
parser.add_argument('--log', type=Path)
options = parser.parse_args()

def dds(width, height, format_id, pixels):
    header = bytearray(148)
    header[:4] = b'DDS '
    for offset, value in ((4,124),(8,0x1007),(12,height),(16,width),(76,32),
                          (80,4),(84,0x30315844),(108,0x1000),(128,format_id),
                          (132,3),(140,1)):
        struct.pack_into('<I', header, offset, value)
    return header+pixels

def png(pixel):
    def chunk(kind, data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    return (b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>2I5B',4,4,8,6,0,0,0))+
            chunk(b'IDAT',zlib.compress((b'\0'+bytes(pixel)*4)*4))+chunk(b'IEND',b''))

with tempfile.TemporaryDirectory(prefix='dark-native-pbr-') as temporary:
    folder=Path(temporary);game=folder/'game';game.mkdir();(folder/'saves').mkdir()
    for entry in options.game_path.resolve().iterdir():
        destination=game/entry.name
        if entry.suffix.lower()=='.cfg' and entry.is_file():
            destination.write_bytes(entry.read_bytes())
        else:
            destination.symlink_to(entry.resolve(),target_is_directory=entry.is_dir())

    def write(relative, data):
        destination=game
        for component in Path(relative).parts[:-1]:
            destination/=component
            if destination.is_symlink():
                target=destination.resolve();destination.unlink();destination.mkdir()
                if target.is_dir():
                    for entry in target.iterdir():
                        (destination/entry.name).symlink_to(entry,target_is_directory=entry.is_dir())
            else:
                destination.mkdir(exist_ok=True)
        destination/=Path(relative).name
        if destination.is_symlink():destination.unlink()
        destination.write_bytes(data)

    # BC7 mode 6 constant endpoints, including per-endpoint precision bits.
    fields=[(64,7)]+[(value,7) for value in (64,64,64,64,64,64,127,127)]+[(1,1),(1,1),(0,3)]+[(0,4)]*15
    bits=0;shift=0
    for value,count in fields:bits|=value<<shift;shift+=count
    block=bits.to_bytes(16,'little')
    write('patches/fam/Core/COBL2.dds',dds(64,64,98,block*(16*16)))
    normal=bytes((128,0,0,0,0,0,0,0))*2
    write('patches/fam/Core/COBL2_normal.dds',dds(4,4,83,normal))
    for suffix,value in (('_roughness',64),('_metallic',192),('_ao',128)):
        write('patches/fam/Core/COBL2'+suffix+'.dds',dds(4,4,80,bytes((value,0,0,0,0,0,0,0))))
    write('patches/fam/Core/COBL2_emissive.dds',dds(4,4,28,bytes((8,8,8,255))*16))
    write('patches/fam/Core/COBL2_orm.dds',dds(4,4,28,bytes((255,128,0,255))*16))
    write('patches/obj/txt/FRAME_normal.png',png((128,128,255,255)))
    write('patches/obj/txt/FRAME_roughness.png',png((180,180,180,255)))
    environment=os.environ.copy();environment['DARK_RENDER_TRACE']=str(folder/'trace')
    command=[str(options.build.resolve()/'thief-runner'),f'game_path={game}',
             f'save_path={folder/"saves"}','content=configured',f'mod_path={game/"patches"}',
             'uber_mod_path=','quickstart=miss2.mis',
             'auto_stop=16','renderer_mode=modern','SlowFrame=0','min_frame_time=0','vsync=0']
    with (folder/'log').open('w') as log:
        result=subprocess.run(command,env=environment,stdout=log,stderr=subprocess.STDOUT,timeout=120)
    text=(folder/'log').read_text(errors='replace')
    trace=(folder/'trace').read_text(errors='replace') if (folder/'trace').exists() else ''
    if options.log:options.log.write_text(text+'\n'+trace)
    terrain=re.search(r'world-pbr source=fam\\Core\\COBL2 source_revision=\d+ maps=6',trace,re.I)
    model=re.search(r'world-pbr source=obj\\txt\\FRAME source_revision=\d+ maps=2',trace,re.I)
    errors=re.search(r'Validation Error|VUID-|SYNC-HAZARD|FATAL:|Modern world disabled:|DDS: unsupported',text+trace)
    if result.returncode or not terrain or not model or errors or trace.count('world-lighting slot=')<12:
        print(text);print(trace)
        raise SystemExit('native DDS/PBR resource discovery failed')
    print(terrain[0]);print(model[0])
    print('PASS native BC7 base, BC4/BC5 and RGBA DDS maps, PNG model maps, terrain mip provenance and clean SDK rendering')
