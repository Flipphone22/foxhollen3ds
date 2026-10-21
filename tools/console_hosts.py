import json,os
from pathlib import Path

CONFIG=Path(__file__).resolve().parents[1]/'private'/'consoles.json'

def console_host(name):
    value=os.environ.get('FH_%s_3DS_HOST'%name.upper())
    if not value and CONFIG.exists():value=json.loads(CONFIG.read_text()).get(name)
    if not value:raise SystemExit('Set FH_%s_3DS_HOST or add "%s" to %s'%(name.upper(),name,CONFIG))
    return value

def _lazy(name):
    try:return console_host(name)
    except SystemExit:return ''

OLD_3DS=_lazy('old')
NEW_3DS=_lazy('new')
