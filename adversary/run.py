"""Build and validate adversarial traces; Python standard library only.

Downloads nothing. A contribution checkout needs the pinned Git objects;
the offline kit supplies the exact same dependency files under reference/.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build' / 'adversary'
PINS = {
    'prince-chavda': '2db28739e291a07bf65c68b44db6a86fbd187b91',
    'Saksham': 'de0ab032ce8599d1ef7c9be048377c23cd571327',
    'om-oracle': 'fc2a10c93514b18d1cb87adfbcc0c71205afec1a',
}

def export(branch, source, target):
    local = ROOT / 'reference' / branch / source
    if local.is_file():
        data = local.read_bytes()
    else:
        result = subprocess.run(['git', 'show', f'{PINS[branch]}:{source}'],
                                cwd=ROOT, capture_output=True)
        if result.returncode:
            raise RuntimeError('Missing pinned dependency. Use the offline ZIP, or '
                               'run git fetch origin in your checkout.\n' +
                               result.stderr.decode(errors='replace'))
        data = result.stdout
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(data)
    return {'branch': branch, 'commit': PINS[branch], 'path': source,
            'sha256': hashlib.sha256(data).hexdigest()}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--ops', type=int, default=10000)
    parser.add_argument('--scale', type=int, default=10000,
                        help='existing oracle scale test, 2..1000000 players')
    args = parser.parse_args()
    if not 0 <= args.ops <= 10000000 or not 2 <= args.scale <= 1000000:
        parser.error('ops must be 0..10000000; scale 2..1000000')
    BUILD.mkdir(parents=True, exist_ok=True)
    records = []
    def run(command, **kwargs):
        result = subprocess.run([str(x) for x in command], cwd=BUILD,
                                capture_output=True, text=True, **kwargs)
        record = {'command': [str(x) for x in command],
                  'exit_code': result.returncode, 'stderr': result.stderr}
        if Path(str(command[0])).stem == 'generator':
            record.update({'output_lines': len(result.stdout.splitlines()),
                           'output_sha256': hashlib.sha256(
                               result.stdout.encode()).hexdigest()})
        else:
            record['stdout'] = result.stdout
        records.append(record)
        if result.returncode:
            raise RuntimeError('Command failed: ' + ' '.join(map(str, command)) +
                               '\n' + result.stdout + result.stderr)
        return result.stdout
    suffix = '.exe' if os.name == 'nt' else ''
    exe = lambda name: BUILD / (name + suffix)
    deps = BUILD / 'deps'
    provenance = []
    try:
        for branch, source, target in [
            ('prince-chavda', 'avl.cpp', deps/'avl.cpp'),
            ('Saksham', 'Fenvick.cpp', deps/'Fenvick.cpp'),
            *[('om-oracle', 'oracle/'+p, deps/'oracle'/p) for p in
              ['differential.cpp','legacy_adapters.hpp','replay.hpp','sorted_array.hpp','main.cpp']],
            ('om-oracle','include/leaderboard.hpp',deps/'include'/'leaderboard.hpp'),
        ]:
            provenance.append(export(branch, source, target))
        flags = ['-std=c++17','-O2','-Wall','-Wextra','-Wpedantic']
        run([args.compiler, *flags, ROOT/'adversary/generator.cpp', '-o',exe('generator')])
        run([args.compiler, *flags, deps/'oracle/main.cpp', '-o',exe('oracle')])
        run([args.compiler, *flags, '-I'+str(deps), deps/'oracle/differential.cpp',
             '-o',exe('differential')])
        # Independent hand-calculated fixture protects against a shared misconception.
        expected = (ROOT/'adversary/tests/ties.expected').read_text().splitlines()
        actual = run([exe('oracle'),ROOT/'adversary/tests/ties.trace']).splitlines()
        if actual != expected:
            raise RuntimeError('Hand-calculated ties fixture does not match')
        run([exe('differential'),'--trace',ROOT/'adversary/tests/ties.trace'])
        traces = BUILD/'traces'
        traces.mkdir(exist_ok=True)
        for name in ['rotations','ascending','descending','ties','churn','extremes',
                     'seasons','unequal','errors','mixed']:
            seeds = [253,42,20261115] if name == 'mixed' else [253]
            for seed in seeds:
                command = [exe('generator'),'--case',name,'--players',64,
                           '--ops',args.ops,'--seed',seed]
                output = run(command)
                if output != run(command):
                    raise RuntimeError('Non-reproducible generator output')
                if name == 'mixed' and len(output.splitlines()) != args.ops:
                    raise RuntimeError('Mixed operation count differs')
                trace = traces/f'{name}-{seed}.trace'
                trace.write_text(output, encoding='utf-8')
                print(run([exe('differential'),'--trace',trace]).strip(), flush=True)
        # Full-width IDs belong to the oracle contract, beyond legacy demo support.
        wide = run([exe('oracle'),ROOT/'adversary/tests/wide.trace']).splitlines()
        if wide != (ROOT/'adversary/tests/wide.expected').read_text().splitlines():
            raise RuntimeError('64-bit oracle fixture failed')
        for invalid in [['--players','0'],['--case','unknown'],['--seed','-1']]:
            result = subprocess.run([str(exe('generator')),*invalid],capture_output=True)
            if result.returncode != 2:
                raise RuntimeError('Generator accepted invalid CLI arguments')
        # Deliberate failure proves the imported runner detects/reduces a mismatch.
        failure = subprocess.run([str(exe('differential')),'--trace',
                      str(ROOT/'adversary/tests/ties.trace'),'--inject-fault'],
                      cwd=BUILD,capture_output=True,text=True)
        records.append({'test':'intentional fault injection', 'exit_code':failure.returncode,
                        'stdout':failure.stdout,'stderr':failure.stderr})
        if failure.returncode != 1 or not (BUILD/'oracle-failure.min.trace').is_file():
            raise RuntimeError('Expected fault detection/reproducer was absent')
        print(run([exe('differential'),'--scale',args.scale]).strip(), flush=True)
        print('PASS adversary suite; JSON evidence in build/adversary/validation.json')
    finally:
        (BUILD/'validation.json').write_text(json.dumps({
            'dependencies':provenance,'commands':records},indent=2),encoding='utf-8')

if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, OSError) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
