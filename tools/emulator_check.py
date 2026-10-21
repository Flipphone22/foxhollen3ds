from pathlib import Path
import argparse
import csv
import hashlib
import io
import json
import os
import re
import select
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import time
import zlib

ROOT = Path(__file__).resolve().parents[1]
EMULATOR = Path('/build/azahar-pica-layout/azahar')
EMULATOR_MANIFEST = ROOT / 'artifacts/emulator-build/pica-layout/build.json'
DSP_FIRMWARE = ROOT / 'assets/console-private/dspfirm.cdc'
DISC = ROOT / 'assets/GSAP01-rev1/disc.iso'
EVIDENCE = ROOT / 'artifacts/console-visual/20261013-114932-223'
RECIPE = ROOT / 'native-3ds/data/install-recipe.json'
FIXTURE = ROOT / 'artifacts/research/texture-mips-fixture-v4'

MENU_EVENTS = [(0, 0), (120, 1), (140, 0), (600, 8), (620, 0), (780, 1), (800, 0), (940, 1), (960, 0),
               (1100, 1), (1120, 0), (1400, 1), (1420, 0)]
MENU_TARGETS = [(70, 'ready-menu'), (700, 'menu'), (900, 'profile'), (1300, 'profile-confirm'), (1600, 'loaded')]
DEFAULT_WALK = '1610:0:100,1800:70:70,1950:0:100,2100:-70:70,2250:0:0'
SDBENCH_EVENTS = [(0, 0), (120, 128), (140, 0), (160, 128), (180, 0), (200, 128), (220, 0), (240, 1), (260, 0),
                  (280, 1), (300, 0)]
SELECT = 4


class Interrupted(Exception):
    pass


def sha256(path):
    with open(path, 'rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def config_text(old):
    groups = {
        'UI': {'firstStart': 'false', 'calloutFlags': '4294967295', 'confirm_before_closing': 'false',
               'pause_when_in_background': 'false', 'single_window_mode': 'true'},
        'System': {'is_new_3ds': 'false' if old else 'true'},
        'Core': {'cpu_clock_percentage': '100', 'use_cpu_jit': 'true'},
        'Renderer': {'graphics_api': '1', 'use_vsync': 'false', 'frame_limit': '100', 'resolution_factor': '1',
                     'use_disk_shader_cache': 'false', 'async_shader_compilation': 'false'},
        'Audio': {'output_type': '1', 'volume': '0'},
        'WebService': {'enable_telemetry': 'false'},
        'Miscellaneous': {'check_for_update_on_start': 'false'},
    }
    return '\n'.join('[' + group + ']\n' + '\n'.join(f'{key}={value}\n{key}\\default=false' for key, value in items.items())
                     for group, items in groups.items()) + '\n'


def elf_symbols(path):
    data = Path(path).read_bytes()
    assert data[:4] == b'\x7fELF' and data[4] == 1 and data[5] == 1, 'expected ELF32 little-endian'
    shoff, = struct.unpack_from('<I', data, 0x20)
    shentsize, shnum = struct.unpack_from('<HH', data, 0x2e)
    sections = [struct.unpack_from('<10I', data, shoff + i * shentsize) for i in range(shnum)]
    symtab = next(s for s in sections if s[1] == 2)
    strtab = sections[symtab[6]]
    strings = data[strtab[4]:strtab[4] + strtab[5]]
    rows = []
    for at in range(symtab[4], symtab[4] + symtab[5], 16):
        name, value, size, info, _, shndx = struct.unpack_from('<IIIBBH', data, at)
        end = strings.index(b'\0', name)
        rows.append((strings[name:end].decode(errors='replace'), value, size, info & 15, shndx))
    return data, rows


def read_only_counters(elf):
    data, rows = elf_symbols(elf)
    menu = [value for name, value, _, kind, shndx in rows
            if name == 'fh_startup_input_polls' and shndx != 0 and kind not in (3, 4)]
    assert len(menu) == 1, ('fh_startup_input_polls', menu)
    game = []
    owner = None
    for name, value, size, kind, _ in rows:
        if kind == 4:
            owner = name
        if owner == 'platform_3ds.c' and name == 'retraces' and size == 4:
            game.append(value)
    assert len(game) == 1, ('platform_3ds.c retraces', game)
    return data, menu[0], game[0]


def timeline(args):
    if args.sdbench:
        return [(n, b, 0, 0) for n, b in SDBENCH_EVENTS], [(70, 'ready-menu')], None, None, 240
    events = [(n, b, 0, 0) for n, b in MENU_EVENTS]
    walk = args.walkpath or (DEFAULT_WALK if args.walk else None)
    if walk:
        for item in walk.split(','):
            n, x, y = (int(v) for v in item.split(':'))
            events.append((n, 0, x, y))
        exit_counter = 2600
        targets = MENU_TARGETS + [(n, 'walk-%03d' % n) for n in range(1620, exit_counter, 15)]
        bound = 420
    else:
        exit_counter = 1800
        targets = MENU_TARGETS + [(1750, 'loaded-later')]
        bound = 240
    events += [(exit_counter, SELECT, 0, 0), (exit_counter + 20, 0, 0, 0)]
    assert all(a[0] < b[0] for a, b in zip(events, events[1:])), 'walk path must fit between 1420 and the exit counter'
    assert all(-154 <= v <= 154 for e in events for v in e[2:]), 'circle-pad axis outside -154..154'
    return events, targets, exit_counter, walk, bound


def stop(process):
    if process is not None and process.poll() is None:
        os.killpg(process.pid, signal.SIGTERM)
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait(timeout=5)


class HeadlessDisplay:
    def __init__(self, output):
        self.output = Path(output)
        self.process = None
        self.runtime = None
        self.log = None

    def __enter__(self):
        self.runtime = tempfile.TemporaryDirectory(prefix='foxhollow-runtime-')
        self.log = (self.output / 'xvfb.log').open('wb')
        reader, writer = os.pipe()
        try:
            self.process = subprocess.Popen(['Xvfb', '-displayfd', str(writer), '-screen', '0', '1280x960x24',
                                             '-nolisten', 'tcp', '-noreset'], pass_fds=(writer,), stdout=self.log,
                                            stderr=subprocess.STDOUT, start_new_session=True)
            os.close(writer)
            writer = None
            if not select.select([reader], [], [], 20)[0]:
                raise RuntimeError('Xvfb startup timeout')
            display = os.read(reader, 64).decode().strip()
            if not display.isdigit():
                raise RuntimeError('Xvfb did not provide a display number')
            self.env = dict(os.environ, DISPLAY=':' + display, QT_QPA_PLATFORM='xcb', LIBGL_ALWAYS_SOFTWARE='1',
                            GALLIUM_DRIVER='llvmpipe', LP_NUM_THREADS='4', XDG_RUNTIME_DIR=self.runtime.name,
                            SDL_AUDIODRIVER='dummy')
            for key in ('WAYLAND_DISPLAY', 'PULSE_SERVER', 'QT_PLUGIN_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH'):
                self.env.pop(key, None)
            with (self.output / 'graphics-host.txt').open('w') as graphics:
                subprocess.run(['glxinfo', '-B'], env=self.env, stdout=graphics, stderr=subprocess.STDOUT,
                               check=True, timeout=20)
            return self
        except BaseException:
            self.__exit__(None, None, None)
            raise
        finally:
            os.close(reader)
            if writer is not None:
                os.close(writer)

    def screenshot(self, name):
        subprocess.run(['xwd', '-root', '-silent', '-out', str(self.output / (name + '.xwd'))], env=self.env,
                       check=True, timeout=10)

    def __exit__(self, *unused):
        stop(self.process)
        if self.log:
            self.log.close()
        if self.runtime:
            self.runtime.cleanup()


def spawn(args, cwd, env, log):
    process = subprocess.Popen(args, cwd=cwd, env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
    os.setpriority(os.PRIO_PROCESS, process.pid, 10)
    os.sched_setaffinity(process.pid, sorted(os.sched_getaffinity(0))[:4])
    return process


def png_bytes(width, height, rows):
    def chunk(kind, payload):
        return struct.pack('>I', len(payload)) + kind + payload + struct.pack('>I', zlib.crc32(kind + payload))
    raw = b''.join(b'\0' + bytes(row) for row in rows)
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0)) +
            chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))


def xwd_to_png(source, destination):
    data = Path(source).read_bytes()
    h = struct.unpack('>25I', data[:100])
    width, height, bpp, stride, lsb = h[4], h[5], h[11], h[12], h[7] == 0
    if bpp not in (24, 32) or stride < width * (bpp // 8):
        raise RuntimeError('Unexpected Xvfb pixel storage')
    size = bpp // 8
    offset = h[0] + h[19] * 12
    masks = h[14:17]
    lanes = []
    for mask in masks:
        shift = (mask & -mask).bit_length() - 1
        if mask >> shift != 0xff or shift % 8:
            lanes = None
            break
        lanes.append(shift // 8 if lsb else size - 1 - shift // 8)
    rows = []
    for y in range(height):
        row = data[offset + y * stride:offset + y * stride + width * size]
        out = bytearray(width * 3)
        if lanes:
            for channel, lane in enumerate(lanes):
                out[channel::3] = row[lane::size]
        else:
            for x in range(width):
                value = int.from_bytes(row[x * size:(x + 1) * size], 'little' if lsb else 'big')
                for channel, mask in enumerate(masks):
                    out[x * 3 + channel] = (value & mask) >> ((mask & -mask).bit_length() - 1)
        rows.append(out)
    Path(destination).write_bytes(png_bytes(width, height, rows))


def scene_admission(sd):
    log_bytes = (sd / 'native.log').read_bytes()
    log = log_bytes.decode(errors='replace')
    inventories = {}
    for mode in ('performance', 'original'):
        packs = []
        for path in (sd / 'prepared' / mode).rglob('asset-*.bin'):
            with path.open('rb') as f:
                header = f.read(16)
                if header[:8] != b'FHNSSET1':
                    continue
                count = struct.unpack_from('<I', header, 8)[0]
                if not 0 < count <= 4096:
                    raise ValueError('Invalid native scene directory count')
                directory = f.read(count * 32)
            if len(directory) != count * 32:
                raise ValueError('Truncated native scene directory')
            entries = [struct.unpack_from('<8I', directory, i * 32) for i in range(count)]
            pack_bytes = path.stat().st_size
            for e in entries:
                if e[3] < 16 + len(directory) or e[3] + e[4] > pack_bytes:
                    raise ValueError('Scene directory extent outside pack')
            packs.append((path, entries, header + directory))
        if len(packs) != 1:
            raise ValueError(f'{mode}: expected one scene inventory, got {len(packs)}')
        inventories[mode] = packs[0]
    pattern = re.compile(r'PICA actor source not admitted: id=(\d+) bytes=(\d+) crc=([0-9a-f]{8}) size_matched=([01])$')
    rejections = []
    for line in log.splitlines():
        if 'PICA actor source not admitted:' not in line:
            continue
        m = pattern.search(line)
        if not m:
            raise ValueError('Unrecognized admission diagnostic: ' + line)
        ident, size, crc, size_matched = m.groups()
        key = (int(ident), int(size), int(crc, 16))
        matches = {mode: sum(e[:3] == key and e[6] == 0 for e in entries)
                   for mode, (_, entries, _) in inventories.items()}
        rejections.append(dict(id=key[0], source_bytes=key[1], crc32=crc, size_matched=bool(int(size_matched)),
                               exact_inventory_matches=matches))
    if 'Native scene storage:' not in log:
        raise ValueError('Missing terminal scene storage report')
    coverage = []
    for line in log.splitlines():
        if 'Native coverage failure:' not in line:
            continue
        m = re.search(r'Native coverage failure: site=(\d+) asset=(\d+) detail=(\d+) count=(\d+)$', line)
        if not m:
            raise ValueError('Unrecognized draw coverage diagnostic: ' + line)
        coverage.append(dict(zip(('site', 'asset', 'detail', 'count'), map(int, m.groups()))))
    overflow = [line for line in log.splitlines() if 'coverage' in line.lower() and 'overflow' in line.lower()]
    return dict(schema=1, native_log_sha256=hashlib.sha256(log_bytes).hexdigest(),
                inventories={mode: dict(path=str(p.relative_to(sd)), directory_sha256=hashlib.sha256(d).hexdigest(),
                                        entries=len(entries)) for mode, (p, entries, d) in inventories.items()},
                admitted_asset_ids=sorted(set(map(int, re.findall(r'Scene asset id=(\d+) ', log)))),
                rejections=rejections,
                rejected_ids_with_exact_inventory_entry=sorted(set(v['id'] for v in rejections
                                                                   if any(v['exact_inventory_matches'].values()))),
                draw_coverage_failures=coverage, coverage_overflow_records=overflow,
                missing_actor_draw_attempts=sum(v['count'] for v in coverage if v['site'] == 36),
                recorded_draw_coverage_pass=not coverage and not overflow,
                complete_rendering_performance_eligible=not rejections and not coverage and not overflow)


def finish_game(report, sd, out, current, exit_counter, targets, build_id, elf_data):
    drained = json.loads((sd / 'log-io-summary.json').read_text())
    assert current >= exit_counter and not targets, ('early drain', current, [t[1] for t in targets])
    assert drained['shutdown_result'] == 0 and drained['queued'] == 0 and drained['errors'] == 0, drained
    native = (sd / 'native.log').read_text()
    assert re.search(r'\d+ shutdown\b', native), 'no shutdown line'
    assert 'THP decode error=' not in native, 'THP decode error'
    actors = re.search(r'PICA actor geometry: actors=(\d+) draws=(\d+)', native)
    assert actors and int(actors[1]) and int(actors[2]), 'No actual native actor rendering'
    assert 'PICA image load failure' not in native and 'PICA missing material' not in native, 'PICA load failure'
    material_line = re.search(r'PICA materials: ([^\n]+)', native)
    assert material_line, 'no PICA materials line'
    materials = {k: int(v) for k, v in re.findall(r'(\w+)=(\d+)', material_line[1])}
    assert materials['loads'] > 0 and materials['errors'] == 0 and materials['gpu'] == 0 and materials['cpu'] == 0, materials
    assert materials['gpu_peak'] <= 4 * 1024 * 1024, materials
    fields = dict(s.split('=', 1) for s in (sd / 'startup-result.txt').read_text().splitlines())
    assert fields['result'] == '1' and fields['lcd_retired'] == '1', fields
    if build_id:
        assert fields['build'] == build_id[:12], (fields['build'], build_id)
    else:
        assert re.fullmatch(r'[0-9a-f]{12}', fields['build']) and fields['build'].encode() in elf_data, fields
    memory = [json.loads(row) for row in (sd / 'memory.jsonl').read_text().splitlines()]
    assert memory and all(v['heap_used'] <= v['heap_reserved'] and 0 < v['linear_free'] <= v['linear_reserved']
                          and v['process_commit'] <= v['process_limit'] for v in memory), 'memory budget'
    measured = [v for v in memory if v.get('main_stack_measured')]
    assert measured and all(v['main_stack_peak_upper_bound'] <= v['main_stack_bytes'] for v in measured), 'stack budget'
    active = [v for v in memory if v.get('renderer_initialized')]
    assert active, 'renderer never initialized'
    budgets = dict(heap_peak=max(v['heap_used'] for v in memory), heap_reserve=memory[-1]['heap_reserved'],
                   linear_min_free=min(v['linear_free'] for v in memory), linear_reserved=memory[-1]['linear_reserved'],
                   vram_min_free=min(v['vram_free'] for v in active), vram_uninitialized_samples=len(memory) - len(active),
                   stack_peak_upper_bound=max(v['main_stack_peak_upper_bound'] for v in measured),
                   stack_reserved=measured[-1]['main_stack_bytes'], process_peak=max(v['process_commit'] for v in memory),
                   process_limit=memory[-1]['process_limit'])
    audit = json.loads((out / 'pica-layout.json').read_text())
    assert audit['status'] in ('checking', 'completed') and audit['draws'] > 0 and audit['fault']['reason'] == 0, audit
    report.update(status='pass', startup=fields, drained=drained, actor_draws=int(actors[2]), memory=memory[-1],
                  observed_budgets=budgets, materials=materials, pica_layout=audit, capture_review_required=True)


def sd_bench_complete(path):
    if not path.exists():
        return None
    text = path.read_text(errors='replace')
    lines = text.splitlines()
    if not lines or not lines[0].startswith('# '):
        return None
    last = 'parallel2_aligned_32k,' if 'texture=none' in lines[0] else 'texture_load_offset_sorted,'
    if not any(line.startswith(last) for line in lines):
        return None
    return dict(header=lines[0][2:], rows=list(csv.DictReader(io.StringIO('\n'.join(lines[1:])))),
                sha256=hashlib.sha256(text.encode()).hexdigest())


def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument('--app', type=Path, required=True)
    p.add_argument('--model', choices=['old', 'new'], required=True)
    p.add_argument('--fixture', type=Path, default=FIXTURE)
    p.add_argument('--save', type=Path, default=EVIDENCE / 'saves')
    p.add_argument('--settings', type=Path, default=EVIDENCE)
    p.add_argument('--sdfile', action='append', default=[])
    mode = p.add_mutually_exclusive_group()
    mode.add_argument('--walk', action='store_true')
    mode.add_argument('--walkpath')
    mode.add_argument('--sdbench', action='store_true')
    p.add_argument('--out', type=Path)
    p.add_argument('--bound', type=float)
    p.add_argument('--recipe', type=Path, default=RECIPE)
    p.add_argument('--build-id')
    p.add_argument('--keep-xwd', action='store_true')
    a = p.parse_args()
    for item in a.sdfile:
        name = item.split('=', 1)[0]
        if '=' not in item or not name or Path(name).is_absolute() or '..' in Path(name).parts:
            p.error('--sdfile expects relative-name=value')
    return a


def main():
    a = parse_args()
    old = a.model == 'old'
    app = a.app.resolve()
    binary = app / ('foxhollow.cxi' if old else 'foxhollow.3dsx')
    elf = app / 'foxhollow.elf'
    for path in (binary, elf):
        if not path.is_file():
            raise SystemExit(f'missing {path}')
    events, targets, exit_counter, walk, bound = timeline(a)
    bound = a.bound or bound
    fixture_dir = a.fixture.resolve()
    fixture = json.loads((fixture_dir / 'result.json').read_text())
    assert fixture['passed'] and fixture['synthetic_integration_generation'], 'fixture not a passed synthetic fixture'
    assert isinstance(fixture.get('files'), list) and fixture['files'], 'fixture records no runtime files'
    for item in fixture['files']:
        relative = Path(item['relative_path'])
        assert not relative.is_absolute() and '..' not in relative.parts and len(item['sha256']) == 64
    prepared = Path(fixture['fixture'])
    if not prepared.is_absolute():
        prepared = ROOT / prepared
    if not prepared.is_dir():
        prepared = fixture_dir / 'prepared'
    assert json.loads(a.recipe.read_text()) == fixture['spec'], 'app install recipe differs from fixture spec'
    manifest = json.loads(EMULATOR_MANIFEST.read_text())
    emulator_sha = sha256(EMULATOR)
    assert manifest.get('binary') == str(EMULATOR) and manifest.get('binary_sha256', manifest.get('sha256')) == emulator_sha, \
        'emulator does not match its recorded build'
    assert manifest['script_additive_readonly_counter'], 'emulator lacks additive read-only counter'
    elf_data, menu, game = read_only_counters(elf)
    save_source = sorted(a.save.glob('*.gci*'))
    assert a.sdbench or save_source, f'no *.gci* saves in {a.save}'
    save_source_hashes = {p.name: sha256(p) for p in save_source}
    settings_source = sorted(a.settings.glob('graphics-*.cfg'))
    settings_source_hashes = {p.name: sha256(p) for p in settings_source}

    label = 'emucheck-' + a.model + ('-sdbench' if a.sdbench else '-walk' if walk else '-play') + time.strftime('-%Y%m%d-%H%M%S')
    out = (a.out or ROOT / 'artifacts/native' / label).resolve()
    out.mkdir(parents=True, exist_ok=False)
    sd = out / 'user/sdmc/3ds/foxhollow'
    sd.mkdir(parents=True)
    shutil.copy2(DSP_FIRMWARE, sd.parent / 'dspfirm.cdc')
    config = out / 'user/config'
    config.mkdir(parents=True)
    (config / 'qt-config.ini').write_text(config_text(old))
    shutil.copytree(prepared, sd / 'prepared')
    save = sd / 'saves'
    save.mkdir()
    sentinel = save / 'preserve-test.txt'
    sentinel.write_bytes(b'isolated installation handoff\n')
    save_hash = sha256(sentinel)
    for item in a.sdfile:
        name, value = item.split('=', 1)
        (sd / name).parent.mkdir(parents=True, exist_ok=True)
        (sd / name).write_text(value)
    for path in save_source:
        shutil.copy2(path, save / path.name)
    for path in settings_source:
        shutil.copy2(path, sd / path.name)
    script = out / 'input.txt'
    script.write_text(''.join(' '.join(map(str, e)) + '\n' for e in events))
    trace = out / 'hid.jsonl'
    disc_before = DISC.stat()
    linked = sd / 'disc.iso'
    report = dict(status='failed', model=a.model, hardware=a.model, app=str(app), binary=str(binary),
                  binary_sha256=sha256(binary), elf_sha256=hashlib.sha256(elf_data).hexdigest(),
                  fixture=str(fixture_dir), save=str(a.save), settings=str(a.settings), sd_files=a.sdfile,
                  walk_path=walk, sdbench=a.sdbench, scripted_exit_counter=exit_counter, wall_bound_seconds=bound,
                  dsp_firmware_sha256=sha256(DSP_FIRMWARE), read_only_counters=dict(menu=hex(menu), game=hex(game)),
                  emulator=dict(kind=manifest.get('kind'), binary=str(EMULATOR), sha256=emulator_sha),
                  captures=[], timing_comparison_valid=False)
    process = None

    def interrupt(signum, frame):
        raise Interrupted(signal.Signals(signum).name)

    for signum in (signal.SIGTERM, signal.SIGHUP, signal.SIGINT):
        signal.signal(signum, interrupt)
    started = time.monotonic()
    try:
        os.link(DISC, linked)
        with HeadlessDisplay(out) as display, (out / 'emulator.log').open('wb') as log:
            env = dict(display.env, FH_PICA_LAYOUT_REPORT=str(out / 'pica-layout.json'), FH_HID_SCRIPT=str(script),
                       FH_HID_ADDRESS=hex(menu), FH_HID_COUNTER_ADD_ADDRESS=hex(game), FH_HID_TRACE=str(trace))
            process = spawn([str(EMULATOR), str(binary)], out, env, log)
            current = 0
            while time.monotonic() - started < bound:
                panic = sd / 'panic.txt'
                if panic.exists() and panic.stat().st_size:
                    raise RuntimeError(panic.read_text(errors='replace'))
                if trace.exists():
                    with trace.open('rb') as f:
                        f.seek(max(0, trace.stat().st_size - 4096))
                        lines = f.read().decode(errors='replace').splitlines()
                    for line in reversed(lines):
                        try:
                            current = json.loads(line)['retrace']
                            break
                        except (json.JSONDecodeError, KeyError):
                            continue
                if targets and current >= targets[0][0]:
                    _, name = targets.pop(0)
                    display.screenshot(name)
                    report['captures'].append(dict(name=name, counter=current))
                if a.sdbench:
                    bench = sd_bench_complete(sd / 'sd-bench.csv')
                    if bench and not targets:
                        report.update(status='pass', sd_bench=bench)
                        break
                elif (sd / 'log-io-summary.json').exists():
                    try:
                        json.loads((sd / 'log-io-summary.json').read_text())
                    except json.JSONDecodeError:
                        time.sleep(.1)
                        continue
                    finish_game(report, sd, out, current, exit_counter, targets, a.build_id, elf_data)
                    break
                if process.poll() is not None:
                    raise RuntimeError('Emulator exited before drained game shutdown')
                time.sleep(.1)
            else:
                raise RuntimeError(f'exceeded {bound}-second bound')
    except (Exception, Interrupted) as error:
        report['error'] = repr(error)
    finally:
        stop(process)
        report['owned_process_exited'] = process is None or process.poll() is not None
        if linked.exists():
            assert os.path.samefile(linked, DISC)
            linked.unlink()
        after = DISC.stat()
        report['source_stat_unchanged'] = (after.st_size, after.st_mtime_ns) == (disc_before.st_size, disc_before.st_mtime_ns)
        report['save_sentinel_unchanged'] = sha256(sentinel) == save_hash
        report['isolated_game_save_files'] = {q.name: sha256(q) for q in save.iterdir() if q.is_file() and q != sentinel}
        report['input_saves_unchanged'] = {p.name: sha256(p) for p in save_source} == save_source_hashes
        report['input_settings_unchanged'] = {p.name: sha256(p) for p in settings_source} == settings_source_hashes
        report['fixture_unchanged'] = all(sha256(sd / 'prepared' / f['relative_path']) == f['sha256'] and
                                          sha256(prepared / f['relative_path']) == f['sha256'] for f in fixture['files'])
        checks = ('owned_process_exited', 'source_stat_unchanged', 'save_sentinel_unchanged', 'input_saves_unchanged',
                  'input_settings_unchanged', 'fixture_unchanged')
        if not all(report[k] for k in checks):
            report['status'] = 'failed'
    report['execution_status'] = report['status']
    if not a.sdbench:
        try:
            coverage = scene_admission(sd)
            (out / 'scene-admission.json').write_text(json.dumps(coverage, indent=2) + '\n')
            report['render_coverage_pass'] = coverage['recorded_draw_coverage_pass']
            report['missing_actor_draw_attempts'] = coverage['missing_actor_draw_attempts']
            report['complete_rendering_performance_eligible'] = coverage['complete_rendering_performance_eligible']
            if not report['render_coverage_pass'] and report['execution_status'] == 'pass':
                report['status'] = 'failed-render-coverage'
        except Exception as error:
            report['render_coverage_pass'] = False
            report['render_coverage_error'] = str(error)
            if report['execution_status'] == 'pass':
                report['status'] = 'failed-render-coverage-audit'
    report['elapsed_seconds'] = time.monotonic() - started
    for capture in report['captures']:
        source = out / (capture['name'] + '.xwd')
        try:
            xwd_to_png(source, out / (capture['name'] + '.png'))
            capture['image'] = capture['name'] + '.png'
            if not a.keep_xwd:
                source.unlink()
        except Exception as error:
            capture['image_error'] = repr(error)
    report['output'] = str(out)
    (out / 'summary.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k not in ('memory', 'pica_layout', 'sd_bench')}, indent=2))
    return 0 if report['status'] == 'pass' else 1


if __name__ == '__main__':
    sys.exit(main())
