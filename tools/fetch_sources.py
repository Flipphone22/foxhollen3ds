import argparse,json,subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]

def git(*args,cwd=None):
    return subprocess.run(['git',*args],cwd=cwd,check=True,capture_output=True,text=True).stdout.strip()

def checkout(url,commit,destination):
    if not (destination/'.git').exists():
        destination.parent.mkdir(parents=True,exist_ok=True)
        subprocess.run(['git','clone','--no-checkout',url,str(destination)],check=True)
    try:current=git('-c','safe.directory=*','rev-parse','HEAD',cwd=destination)
    except subprocess.CalledProcessError:current=None
    if current!=commit:
        try:git('-c','safe.directory=*','cat-file','-e',commit+'^{commit}',cwd=destination)
        except subprocess.CalledProcessError:git('fetch','origin',commit,cwd=destination)
        git('-c','advice.detachedHead=false','checkout','--force',commit,cwd=destination)
    actual=git('-c','safe.directory=*','rev-parse','HEAD',cwd=destination)
    if actual!=commit:raise SystemExit('%s is at %s, expected %s'%(destination,actual,commit))
    return actual

def main():
    parser=argparse.ArgumentParser()
    parser.parse_args()
    sources=json.loads((ROOT/'sources.lock.json').read_text())['sources']
    for name,entry in sources.items():
        if 'path' in entry:print(name,entry['path'],checkout(entry['url'],entry['commit'],ROOT/entry['path']),flush=True)

if __name__=='__main__':main()
