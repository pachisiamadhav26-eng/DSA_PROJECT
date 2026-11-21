"""Build Bianca's generator and test the merged backends with pinned team modules."""
import argparse
import hashlib
from itertools import zip_longest
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
ORACLE = 'f3f8c29cb357a29a46ad5a654f7d5fac390aceea'
ARCHITECT = 'a4dc4382b11d60ed85697fb9560574d24a8d2366'


def expected_scale(players):
    # This run revises each ID once, in ascending order, from id to players + id.
    def players_line(first_id, count):
        entries = ''.join(f' {id_} {players + id_}'
                        for id_ in range(first_id, first_id - count, -1))
        return f'PLAYERS {count}{entries}'

    def page(low_id, high_id):
        count = high_id - low_id + 1
        yield f'SIZE {count}'
        yield f'PLAYER {high_id} {players + high_id}'
        yield f'PLAYER {low_id} {players + low_id}'
        yield players_line(high_id, min(count, 12))

    for _ in range(players):
        yield 'OK'
    position = players // 2 + 1
    for updated in range(1, players + 1):
        yield 'OK'
        if updated % 1000 == 0:
            yield 'RANK 1'
            yield f'RANK {updated}'
            if position <= updated:
                id_ = updated - position + 1
                score = players + id_
            else:
                id_ = players - (position - updated) + 1
                score = id_
            yield f'PLAYER {id_} {score}'
    yield from page(1, players)
    if players >= 50:
        yield players_line(players - 39, 11)
    yield 'OK'  # SPLIT
    yield from page(players // 2 + 1, players)
    yield from page(1, players // 2)
    yield 'OK'  # MERGE
    yield from page(1, players)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='g++', help='GCC 7+ or another C++17 compiler')
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build' / 'adversary')
    parser.add_argument('--ops', type=int, default=10000)
    parser.add_argument('--scale', type=int, default=10000)
    args = parser.parse_args()
    if not 0 <= args.ops <= 10000000 or not 2 <= args.scale <= 1000000:
        parser.error('ops must be 0..10000000; scale 2..1000000')
    build = args.build_dir.resolve()
    build.mkdir(parents=True, exist_ok=True)
    deps = build / 'deps'
    records = []
    sources = {}
    suffix = '.exe' if os.name == 'nt' else ''
    def exe(name):
        return build / (name + suffix)
    def run(command, expected_exit=0):
        result = subprocess.run([str(x) for x in command], cwd=build,
                                text=True, capture_output=True)
        records.append({'command': list(map(str, command)), 'exit_code': result.returncode,
                        'output_lines': len(result.stdout.splitlines()),
                        'output_sha256': hashlib.sha256(result.stdout.encode()).hexdigest(),
                        'stderr': result.stderr})
        if result.returncode != expected_exit:
            raise RuntimeError('Command failed: ' + ' '.join(map(str, command)) +
                               '\n' + result.stdout + result.stderr)
        return result.stdout
    def export(commit, name):
        result = subprocess.run(['git', '-C', str(ROOT), 'show', commit + ':' + name],
                                capture_output=True)
        if result.returncode:
            raise RuntimeError('Missing pinned team source ' + commit + ':' + name +
                               '. Run git fetch origin in a full clone. Shallow clones need git fetch --unshallow.')
        target = deps / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(result.stdout)
        sources[commit + ':' + name] = hashlib.sha256(result.stdout).hexdigest()
    passed = False
    try:
        # Compile a feature probe before the full build to diagnose old compilers.
        probe = build / 'cxx17.cpp'
        probe.write_text('#include <utility>\nint main(){auto [a,b]=std::pair<int,int>{1,2};return a+b==3?0:1;}\n')
        flags = ['-std=c++17', '-pedantic-errors', '-O2', '-Wall', '-Wextra', '-UNDEBUG']
        try:
            run([args.compiler, *flags, probe, '-o', exe('cxx17')])
        except RuntimeError as error:
            raise RuntimeError('A C++17 compiler is required. Use GCC 7+; GCC 6.3 is unsupported.\n' + str(error)) from error
        run([exe('cxx17')])
        for name in ('main.cpp', 'differential.cpp', 'replay.hpp', 'sorted_array.hpp',
                     'backend_checks.hpp', 'trace.hpp'):
            export(ORACLE, 'oracle/' + name)
        for name in ('leaderboard.cpp', 'replay.cpp', 'replay.hpp', 'player_table.cpp', 'player_table.hpp'):
            export(ARCHITECT, 'app/' + name)
        # Use the merged shared headers and structures directly, never branch copies.
        for name in ('leaderboard.hpp', 'backend_tools.hpp'):
            target = deps / 'include' / name
            target.parent.mkdir(parents=True, exist_ok=True)
            header = (ROOT / 'include' / name).as_posix()
            target.write_text('#include "' + header + '"\n', encoding='utf-8')
        includes = ['-I' + str(ROOT / 'include'), '-I' + str(deps / 'app')]
        backends = [ROOT / 'src/structA/avl.cpp', ROOT / 'src/structB/fenwick.cpp']
        jobs = {
            'generator': [ROOT / 'adversary/generator.cpp'],
            'oracle': [deps / 'oracle/main.cpp'],
            'differential': [deps / 'oracle/differential.cpp', *backends],
            'leaderboard': [deps / 'app/leaderboard.cpp', deps / 'app/replay.cpp',
                            deps / 'app/player_table.cpp', *backends],
        }
        for name, files in jobs.items():
            run([args.compiler, *flags, *includes, *files, '-o', exe(name)])
        run([exe('differential'), '--self-test'])
        for trace in sorted((ROOT / 'adversary/tests').glob('*.trace')):
            expected = trace.with_suffix('.expected').read_text().splitlines()
            assert run([exe('oracle'), trace]).splitlines() == expected, str(trace)
            for backend in ('avl', 'fenwick'):
                assert run([exe('leaderboard'), backend, trace]).splitlines() == expected, str(trace)
            run([exe('differential'), '--trace', trace])
            print('PASS fixture ' + trace.stem, flush=True)
        for case in ('rotations','cascade','ascending','descending','ties','churn',
                     'extremes','seasons','unequal','errors','mixed'):
            for seed in ([253,42,20261115] if case == 'mixed' else [253]):
                command = [exe('generator'),'--case',case,'--players',88 if case == 'cascade' else 64,
                           '--ops',args.ops,'--seed',seed]
                output = run(command)
                assert output == run(command), 'Generator is not repeatable'
                if case == 'mixed':
                    assert len(output.splitlines()) == args.ops
                trace = build / (case + '-' + str(seed) + '.trace')
                trace.write_text(output, encoding='utf-8')
                run([exe('differential'),'--trace',trace])
                print('PASS ' + case + ' seed=' + str(seed), flush=True)
        run([exe('differential'),'--scale',args.scale])
        trace = build / 'scale.trace'
        trace.write_text(run([exe('generator'),'--case','scale','--players',args.scale,
                              '--ops',args.scale]), encoding='utf-8')
        for backend in ('avl','fenwick'):
            output = run([exe('leaderboard'),backend,trace]).splitlines()
            for line, pair in enumerate(zip_longest(output, expected_scale(args.scale)),1):
                if pair[0] != pair[1]:
                    raise RuntimeError('Scale mismatch ' + backend + ' line ' + str(line))
        for name in ('adversary/generator.cpp','adversary/run.py','include/leaderboard.hpp',
                     'include/backend_tools.hpp','src/structA/avl.cpp','src/structB/fenwick.cpp'):
            sources[name] = hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
        passed = True
    finally:
        (build / 'validation.json').write_text(json.dumps({
            'passed': passed, 'ops': args.ops, 'scale': args.scale,
            'sources': sources, 'commands': records}, indent=2), encoding='utf-8')
    print('PASS adversary suite and both merged backends; evidence ' + str(build / 'validation.json'))


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, OSError, AssertionError) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
