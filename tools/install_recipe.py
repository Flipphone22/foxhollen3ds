"""Build conversion-only recipe keys; renderer/build IDs are not asset versions."""
from pathlib import Path
import hashlib,json,re,struct,zlib

SOURCE_SHA='f5e5a9a29147f80648dbef00ed68cd84d0f08c7120645e6f653333296944facf'
COMMON=('install_disc.c','install_record.c','install_transaction.c')
GEOMETRY=('install_geometry_iso.c','install_scene.c','install_groups.c','install_compact.c','install_skin.c','install_vertex.c','install_descriptors.c','install_triangles.c','native_scene.c')
GROUPS={
 'original':COMMON+GEOMETRY+('install_geometry_prepare.c','install_geometry_pack.c','install_cache.c'),
 'performance':COMMON+GEOMETRY+('install_performance_prepare.c','install_performance_pack.c','install_model_reduce.c','model_lod.c','install_reduce.c','install_lod_guards.c','install_surface.c','install_fidelity.c','install_list_pack.c','install_simplify.cpp','install_cache.c'),
 'textures':COMMON+('install_texture_iso.c','install_texture_layout.c','install_texture_prepare_all.c','install_texture_search.c','install_texture_hints.c','install_font.c','install_texture_pack.c','install_texture_padded.c','install_texture_exact.c','platform_texture_pack.c','install_texture.cpp','install_worker.c'),
 'movie':('install_disc.c','install_transaction.c','install_movie_pack.c','install_movie_source.c','install_movie_decode.c','install_movie_resize.c','install_movie_frame.cpp','install_movie_rgb8.c','install_texture.cpp','install_worker.c'),
}
EXTRA={
 'original':(),
 'performance':('native-3ds/vendor/meshoptimizer/simplifier.cpp',),
 'textures':('native-3ds/vendor/rg_etc1/rg_etc1.cpp',),
 'movie':('native-3ds/vendor/rg_etc1/rg_etc1.cpp',),
}
LIBRARIES={'original':('z','lz4','mbedcrypto'),'performance':('z','lz4','mbedcrypto'),'textures':('z','mbedcrypto'),'movie':('z','lz4','mbedcrypto','jpeg')}

# Explicit byte-output compatibility for the reviewed progress-only edit.
# Actual compiled dependency hashes remain in `sources`; `recipe_sources`
# records the identity used by the journal key. This is NOT a wildcard that
# excludes progress files: any other edit to either file gets a new recipe.
# The header only adds an event number; the producer only adds cancellable
# metadata callbacks around the unchanged successful conversion path.
TEXTURE_PROGRESS_COMPATIBILITY={
 'native-3ds/include/fh_install_texture_iso.h':(
  '9e67ac06dfb5ab718ec3e99e3fe8438f62ed5f03cc4f6489d703103a80870d73',
  'bdf0633a60d16bff5e26c0fb35186a1d1feac776b54344ead01c1879de73634e'),
 'native-3ds/source/install_texture_prepare_all.c':(
  '89444fc2b2db07f43dcd4827cdf29503e257214867bbf57d56a6b45614a36409',
  '85634aac345039f3989e60fae3223b79ffa1dcd10ef5cd740c02a496fc5eec4e'),
}
MOVIE_PROGRESS_COMPATIBILITY={
 'native-3ds/source/install_movie_rgb8.c':(
  '7c9e2eac874451d7397a9d019402bfbc46de5657834027cba4197ff0c8bb1f3f',
  'acd5e748fd01c204558981b5c60c85c9cc6be46806a2f2f291300c0df51dc82f'),
}
MOVIE_CHECKPOINT_COMPATIBILITY={
 'native-3ds/source/install_movie_pack.c':(
  '7f2b2e14e129c3da1b7b854afdee6baba7c2599f569cce805ee626b8e93def7c',
  '530ed71f725de90caad36f3c52f63d0eeb0b38c5dfcf0b4d383744ac5430962a'),
}
MOVIE_READBACK_COMPATIBILITY={
 'native-3ds/source/install_movie_pack.c':(
  '0b55e81b1a77631ef7ff7124a78b07ff3a12073315039886290c874c6460c6f0',
  '530ed71f725de90caad36f3c52f63d0eeb0b38c5dfcf0b4d383744ac5430962a'),
}
# Exact append-only transaction extension: existing entry points are unchanged
# except line endings. Its new header is absent from the canonical dependency
# set. Both hashes must match; neither file is generally ignored.
READBACK_TRANSACTION_COMPATIBILITY={
 'native-3ds/source/install_transaction.c':(
  '39b24ea4863f67c44d4bf67ef04d34cfbb1c7590f560478092a32bf763bc1c01',
  '93204da0002905e9ea07832113c5328aa6b9d10d66748da1cd69740fe29f48f9'),
 'native-3ds/include/fh_install_readback.h':(
  'bf6e1e5ad8ab185d0a70b4a5fd01167006c2fec461ef16c9c3f1bf742490be2d',None),
}

def recipe_identity(payload):
 actual=payload
 extended=all(payload['sources'].get(n)==v[0] for n,v in READBACK_TRANSACTION_COMPATIBILITY.items())
 if extended:
  sources=dict(payload['sources'])
  for name,(_,previous) in READBACK_TRANSACTION_COMPATIBILITY.items():
   if previous is None:del sources[name]
   else:sources[name]=previous
  payload=dict(payload,sources=sources)
 basis=payload
 compatibility={}
 if payload['stage']=='textures' and all(payload['sources'].get(n)==v[0] for n,v in TEXTURE_PROGRESS_COMPATIBILITY.items()):
  sources=dict(payload['sources'])
  for name,(_,previous) in TEXTURE_PROGRESS_COMPATIBILITY.items():sources[name]=previous
  basis=dict(payload,sources=sources)
  compatibility=dict(recipe_sources=sources,recipe_compatibility='texture-progress-only-v1')
 elif payload['stage']=='movie':
  # Independent exact reviewed edits. Preserve actual compiled hashes;
  # unknown edits to these files or any other dependency still change the key.
  sources=dict(payload['sources'])
  label=None
  for mapping,tag in ((MOVIE_PROGRESS_COMPATIBILITY,'movie-progress-batch-v1'),(MOVIE_CHECKPOINT_COMPATIBILITY,'movie-checkpoint-batch-v1'),(MOVIE_READBACK_COMPATIBILITY,'movie-readback-v1')):
   if all(sources.get(n)==v[0] for n,v in mapping.items()):
    for name,(_,previous) in mapping.items():sources[name]=previous
    label=tag
  if label:
   basis=dict(payload,sources=sources)
   compatibility=dict(recipe_sources=sources,recipe_compatibility=label)
 if extended:
  compatibility=dict(recipe_sources=basis['sources'],recipe_compatibility='readback-transaction-v1'+(':'+compatibility['recipe_compatibility'] if compatibility else ''))
 key=hashlib.sha256(json.dumps(basis,sort_keys=True,separators=(',',':')).encode()).hexdigest()
 return dict(recipe=key,**actual,**compatibility)

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def closure(root,paths):
 found=set();pending=list(paths)
 while pending:
  p=pending.pop().resolve()
  if p in found:continue
  found.add(p)
  for inc in re.findall(r'^\s*#\s*include\s*"([^"]+)"',p.read_text(),re.M):
   candidates=(p.parent/inc,root/'native-3ds/include'/inc,root/'native-3ds/vendor/meshoptimizer'/inc)
   q=next((q.resolve() for q in candidates if q.is_file()),None)
   if q is None:raise ValueError(('Missing recipe dependency',p,inc))
   pending.append(q)
 return {p.relative_to(root).as_posix().replace('native-3ds/vendor/meshoptimizer/','reference/meshoptimizer/src/'):sha(p) for p in sorted(found)}

def options(root):
 text=(root/'native-3ds/include/fh_install_task.h').read_text()
 def integer(name):
  match=re.search(r'\b'+name+r'\s*=\s*(\d+)\b',text);assert match,name;return int(match[1])
 def real(name):
  match=re.search(r'^#define\s+'+name+r'\s+([0-9.]+)f\s*$',text,re.M);assert match,name;return float(match[1])
 return dict(edge=integer('FH_INSTALL_TASK_TEXTURE_EDGE'),scale=integer('FH_INSTALL_TASK_MOVIE_SCALE'),cache_level=integer('FH_INSTALL_TASK_CACHE_LEVEL'),ratio=real('FH_INSTALL_TASK_RATIO'),error=real('FH_INSTALL_TASK_ERROR'))

def recipes(root,sdk):
 root=Path(root).resolve();sdk=Path(sdk);o=options(root)
 policy={
  'original':dict(artifacts=['spool','scene','cache','empty-topology'],cache=o['cache_level']),
  'performance':dict(artifacts=['spool','topology','scene','cache'],ratio=o['ratio'],error=o['error'],cache=o['cache_level']),
  'textures':dict(artifacts=['spool','typed-textures'],scope='all-base-planes-padded-v4-visible-rmse14',edge=o['edge']),
  'movie':dict(artifacts=['spool','movie'],scale=o['scale'],format='RGB8-LZ4' if 'else result=fh_install_movie_prepare_rgb8(' in (root/'native-3ds/source/install_task.c').read_text() else 'ETC1-LZ4'),
 }
 result={}
 for stage,names in GROUPS.items():
  units=[root/'native-3ds/source'/n for n in names]+[root/p for p in EXTRA[stage]]
  sources=closure(root,units)
  if stage=='textures':sources['native-3ds/data/install-texture-hints.zlib']=sha(root/'native-3ds/data/install-texture-hints.zlib')
  # Archive content captures codec/SDK revisions, independent of checkout path.
  libraries={name:sha(sdk/'portlibs/3ds/lib'/('lib'+name+'.a')) for name in LIBRARIES[stage]}
  payload=dict(schema='fh-native-install-stage-v1',stage=stage,policy=policy[stage],sources=sources,libraries=libraries,arithmetic='ARMv6K-vfp-hard-fp-contract-off-no-strict-aliasing-native-enums-JPEG-private-short-enums')
  result[stage]=recipe_identity(payload)
 return result

def generate(root,sdk,destination):
 root=Path(root).resolve();destination=Path(destination)
 iso=root/'assets/GSAP01-rev1/disc.iso';before=iso.stat()
 with iso.open('rb') as f:
  identity=hashlib.file_digest(f,'sha256').hexdigest();assert identity==SOURCE_SHA,'Unsupported or changed source disc'
  f.seek(0);header=f.read(0x440);assert header[:8]==b'GSAP01\0\1'
  at,n=struct.unpack_from('>II',header,0x424);assert 13<=n<=2*1024*1024 and at+n<=before.st_size
  f.seek(at);fst=f.read(n);assert len(fst)==n
 assert (iso.stat().st_size,iso.stat().st_mtime_ns)==(before.st_size,before.st_mtime_ns)
 stages=recipes(root,sdk);spec=dict(stages=stages,source_sha256=identity,disc_bytes=before.st_size,fst_crc=zlib.crc32(fst))
 def array(value):return '{'+','.join('0x'+value[i:i+2] for i in range(0,64,2))+'}'
 text='#ifndef FH_GENERATED_INSTALL_RECIPE_H\n#define FH_GENERATED_INSTALL_RECIPE_H\n#include "fh_install_bundle.h"\nstatic const FhInstallBundleSpec fh_install_game_spec={\n .stage_recipe={'+',\n'.join(array(s['recipe']) for s in stages.values())+'},\n .source='+array(identity)+',\n .disc_bytes='+str(before.st_size)+', .fst_crc='+str(spec['fst_crc'])+'\n};\n#endif\n'
 (destination/'include/install_recipe.h').write_text(text)
 spec['header_sha256']=sha(destination/'include/install_recipe.h');(destination/'data/install-recipe.json').write_text(json.dumps(spec,indent=2)+'\n')
 return spec

if __name__=='__main__':
 import argparse
 p=argparse.ArgumentParser();p.add_argument('--destination',type=Path,default=Path(__file__).resolve().parents[1]/'native-3ds');p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1]);p.add_argument('--sdk',type=Path,default=Path('/sdk'));a=p.parse_args()
 result=generate(a.root,a.sdk,a.destination);print(json.dumps({k:v for k,v in result.items() if k!='stages'},indent=2))
