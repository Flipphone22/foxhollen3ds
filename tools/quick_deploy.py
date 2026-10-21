import argparse,ftplib,io,sys,time
from pathlib import Path
from console_hosts import console_host

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('console',choices=['old','new'])
    parser.add_argument('--app',type=Path,default=Path(__file__).resolve().parents[1]/'build/app')
    args=parser.parse_args()
    ftp=ftplib.FTP()
    ftp.connect(console_host(args.console),5000,timeout=15)
    ftp.login();ftp.set_pasv(True);ftp.cwd('/3ds/foxhollow')
    for name in ('foxhollow.3dsx','foxhollow.cia'):
        data=(args.app/name).read_bytes();sent=[0];start=time.time()
        def progress(block):
            sent[0]+=len(block)
            if sent[0]%(1<<20)<len(block):print(f'{name} {sent[0]>>20} MB',flush=True)
        ftp.storbinary('STOR '+name,io.BytesIO(data),8192,progress)
        ftp.voidcmd('TYPE I');size=ftp.size(name)
        if size!=len(data):sys.exit(f'{name}: console has {size} bytes, expected {len(data)}')
        print(f'{name} ok {len(data)} bytes {time.time()-start:.1f}s',flush=True)
    ftp.quit();print('done',flush=True)

if __name__=='__main__':main()
