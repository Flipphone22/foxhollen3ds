from console_hosts import NEW_3DS,OLD_3DS
import argparse,ftplib,json,posixpath,sys,time
from pathlib import Path

ROOT='/3ds/foxhollow'
REPO=Path(__file__).resolve().parents[1]
RESULTS=REPO/'artifacts/console-test-app-results'
HOSTS={'old':OLD_3DS,'new':NEW_3DS}
SETS={
    'logs':['native.log','previous-native.log','game.log','sd-io.csv','sd-files.csv','sd-slow.csv','sd-bursts.csv','audio.csv','minimal-render.csv','memory.jsonl','panic.txt','startup-result.txt'],
    'capture':['frame-commands.bin','last-frame-top.ppm','frame-inputs.bin'],
    'timeout':['gpu-timeout.bin','gpu-inputs.bin'],
    'bench':['sd-bench.csv'],
}
FLAGS={'frame_capture.txt','sky_depth.txt','core1_grant.txt','audio_pipeline.txt'}

def console_path(name):
    name=name.strip('/')
    joined=posixpath.normpath(posixpath.join(ROOT,name))
    if not joined.startswith(ROOT+'/'):raise SystemExit('outside %s: %s'%(ROOT,name))
    return joined

def connect(host,passive):
    f=ftplib.FTP();f.connect(host,5000,timeout=20);f.login();f.set_pasv(passive);f.voidcmd('TYPE I');return f

def listing(host,passive,directory=ROOT,attempts=5):
    for attempt in range(attempts):
        try:return listing_once(host,passive,directory)
        except (OSError,EOFError,ftplib.Error):
            if attempt+1==attempts:raise
            time.sleep(2)

def listing_once(host,passive,directory):
    f=connect(host,passive);rows=[]
    try:f.retrlines('LIST '+directory,rows.append)
    finally:
        try:f.quit()
        except Exception:f.close()
    entries={}
    for row in rows:
        parts=row.split(None,8)
        if len(parts)==9 and not row.startswith('d'):entries[parts[8]]=dict(size=int(parts[4]),date=' '.join(parts[5:8]))
    return entries

def pull(host,passive,name,destination,attempts,size):
    remote=console_path(name);destination.parent.mkdir(parents=True,exist_ok=True)
    part=destination.with_name(destination.name+'.part');data=bytearray()
    for attempt in range(attempts):
        try:
            f=connect(host,passive)
            try:
                if len(data)<size:f.retrbinary('RETR '+remote,data.extend,65536,rest=len(data) or None)
            finally:
                try:f.quit()
                except Exception:f.close()
            if len(data)==size:
                part.write_bytes(bytes(data));part.replace(destination);return dict(file=name,bytes=size,attempts=attempt+1)
            if len(data)>size:data.clear()
        except ftplib.error_perm as e:
            return dict(file=name,error=str(e))
        except (OSError,EOFError,ftplib.Error) as e:
            time.sleep(1)
    return dict(file=name,error='incomplete after %d attempts: %d bytes'%(attempts,len(data)))

def push_flag(host,passive,name,value):
    if name not in FLAGS:raise SystemExit('only flag files may be written: '+', '.join(sorted(FLAGS)))
    import io
    f=connect(host,passive)
    try:f.storbinary('STOR '+console_path(name),io.BytesIO(value.encode()))
    finally:
        try:f.quit()
        except Exception:f.close()
    return dict(file=name,value=value)

def main():
    p=argparse.ArgumentParser()
    p.add_argument('console',choices=sorted(HOSTS))
    p.add_argument('--active',action='store_true')
    s=p.add_subparsers(dest='command',required=True)
    s.add_parser('ls')
    q=s.add_parser('pull');q.add_argument('label');q.add_argument('items',nargs='+')
    q.add_argument('--attempts',type=int,default=40)
    w=s.add_parser('flag');w.add_argument('name');w.add_argument('value')
    a=p.parse_args();host=HOSTS[a.console];passive=not a.active
    if a.command=='ls':
        print(json.dumps(listing(host,passive),indent=1));return
    if a.command=='flag':
        print(json.dumps(push_flag(host,passive,a.name,a.value)));return
    if not a.label.replace('-','').replace('_','').isalnum():raise SystemExit('label must be alphanumeric with - or _')
    names=[]
    for item in a.items:names+=SETS.get(item,[item])
    present=listing(host,passive)
    out=RESULTS/a.label;results=[]
    for name in dict.fromkeys(names):
        if posixpath.basename(name) not in present or '/' in name:
            results.append(dict(file=name,skipped='absent'));continue
        r=pull(host,passive,name,out/posixpath.basename(name),a.attempts,present[posixpath.basename(name)]['size'])
        if name in present:r['console_date']=present[name]['date']
        results.append(r);print(json.dumps(r),flush=True)
    (out/'pull.json').write_text(json.dumps(dict(console=a.console,host=host,results=results),indent=1)+'\n')
    print(out)
    if any('error' in r for r in results):sys.exit(1)

if __name__=='__main__':main()
