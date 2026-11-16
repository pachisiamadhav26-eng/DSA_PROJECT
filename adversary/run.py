"""Build and test the project offline."""
import argparse
import hashlib
from itertools import zip_longest
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]


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
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build')
    parser.add_argument('--skip-build', action='store_true')
    parser.add_argument('--ops', type=int, default=10000)
    parser.add_argument('--scale', type=int, default=10000)
    args = parser.parse_args()
    if not 0 <= args.ops <= 10000000 or not 2 <= args.scale <= 1000000:
        parser.error('ops must be 0..10000000; scale 2..1000000')
    build = args.build_dir.resolve()
    build.mkdir(parents=True, exist_ok=True)
    evidence = build / 'validation.json'
    evidence.unlink(missing_ok=True)
    records = []
    suffix = '.exe' if os.name == 'nt' else ''
    def exe(name):
        return build / 'bin' / (name + suffix)

    def run(command, expected_exit=0, cwd=build):
        command = [str(x) for x in command]
        start = time.perf_counter()
        result = subprocess.run(command, cwd=cwd, capture_output=True, text=True)
        record = {'command': command, 'exit_code': result.returncode,
                  'seconds': time.perf_counter() - start, 'stderr': result.stderr}
        if Path(command[0]).stem in ('generator', 'leaderboard', 'oracle'):
            record.update(output_lines=result.stdout.count('\n'),
                          output_sha256=hashlib.sha256(result.stdout.encode()).hexdigest())
        else:
            record['stdout'] = result.stdout
        records.append(record)
        if result.returncode != expected_exit:
            raise RuntimeError('Command failed: ' + ' '.join(command) + '\n' + result.stderr)
        return result.stdout

    def check_fixture(path):
        expected = path.with_suffix('.expected').read_text()
        want = run([exe('oracle'), path])
        if want.splitlines() != expected.splitlines():
            raise RuntimeError('Golden output mismatch: ' + str(path))
        for backend in ('avl', 'fenwick'):
            if run([exe('leaderboard'), backend, path]) != want:
                raise RuntimeError('Application output mismatch: ' + backend + ' ' + str(path))
        run([exe('differential'), '--trace', path])

    passed = False
    try:
        if not args.skip_build:
            run(['cmake', '-S', ROOT, '-B', build, '-DCMAKE_BUILD_TYPE=Release'])
            run(['cmake', '--build', build, '--config', 'Release', '--parallel', '4'])
        for name in ('player_table_test', 'replay_test', 'avl_test', 'fenwick_test'):
            run([exe(name)], cwd=ROOT)
        for folder in ('app/examples', 'oracle/tests', 'adversary/tests'):
            for path in sorted((ROOT / folder).glob('*.trace')):
                check_fixture(path)
        traces = build / 'traces'
        traces.mkdir(exist_ok=True)
        for case in ('rotations', 'cascade', 'ascending', 'descending', 'ties', 'churn',
                     'extremes', 'seasons', 'unequal', 'errors', 'mixed'):
            for seed in ([253, 42, 20261115] if case == 'mixed' else [253]):
                players = 88 if case == 'cascade' else 64
                command = [exe('generator'), '--case', case, '--players', players,
                           '--ops', args.ops, '--seed', seed]
                output = run(command)
                if output != run(command):
                    raise RuntimeError('Non-reproducible generator: ' + case)
                lines = output.count('\n')
                if case == 'mixed':
                    if lines != args.ops:
                        raise RuntimeError('Wrong mixed command count')
                    if args.ops == 1000000 and any(output.count('\n' + op + ' ') != 100
                                                   for op in ('SPLIT', 'MERGE')):
                        raise RuntimeError('Expected 100 mixed splits and 100 mixed merges')
                trace = traces / f'{case}-{seed}.trace'
                trace.write_text(output, encoding='utf-8')
                run([exe('differential'), '--trace', trace])
                if case in ('cascade', 'unequal', 'churn', 'seasons', 'errors', 'mixed'):
                    run([exe('avl_test'), case, trace])
                print(f'PASS {case} seed={seed} commands={lines}', flush=True)
        for invalid in (['--players', '0'], ['--case', 'unknown'], ['--seed', '-1']):
            run([exe('generator'), *invalid], expected_exit=2)
        # Remove only our old intentional reproducer, so stale files cannot pass this check.
        minimal = build / 'oracle-failure.min.trace'
        if minimal.exists():
            minimal.unlink()
        run([exe('differential'), '--trace', ROOT / 'adversary/tests/ties.trace',
             '--inject-fault'], expected_exit=1)
        if not minimal.is_file() or len(minimal.read_text().splitlines()) != 2:
            raise RuntimeError('Missing two-command fault reproducer')
        print(run([exe('differential'), '--scale', args.scale]).strip(), flush=True)
        # Run n real revisions at n simultaneous players, without quadratic oracle insertion.
        trace = traces / 'scale.trace'
        trace.write_text(run([exe('generator'), '--case', 'scale', '--players', args.scale,
                              '--ops', args.scale]), encoding='utf-8')
        # The oracle audit runs after every command, so check the same case at a size it can afford.
        small = traces / 'scale-small.trace'
        small.write_text(run([exe('generator'), '--case', 'scale', '--players', 1000,
                              '--ops', 1000]), encoding='utf-8')
        run([exe('differential'), '--trace', small])
        n = args.scale
        for backend in ('avl', 'fenwick'):
            output = run([exe('leaderboard'), backend, trace]).splitlines()
            for line, (actual, expected) in enumerate(zip_longest(output, expected_scale(n)), 1):
                if actual != expected:
                    raise RuntimeError(f'Scale {backend} line {line}: {actual!r}, expected {expected!r}')
        print(f'PASS application scale: {n} players, {n} revisions, both backends', flush=True)
        passed = True
    finally:
        try:
            inputs = [ROOT / 'CMakeLists.txt', *ROOT.glob('*.md')]
            for folder in ('adversary', 'app', 'bench', 'include', 'oracle', 'src', 'tests'):
                inputs.extend(p for p in (ROOT / folder).rglob('*')
                              if p.suffix in ('.cpp', '.hpp', '.py', '.trace', '.expected')
                              and 'CMakeFiles' not in p.parts and p.is_file())
            sources = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                       for p in sorted(inputs)}
        except OSError as error:
            sources = {'_error': str(error)}
        try:
            evidence.write_text(json.dumps({
                'passed': passed, 'platform': platform.platform(), 'python': sys.version,
                'ops': args.ops, 'scale': args.scale, 'sources': sources, 'commands': records
            }, indent=2), encoding='utf-8')
        except OSError as error:
            if passed:
                raise
            print(f'WARNING: evidence not written: {error}', file=sys.stderr)
    print('PASS test suite; evidence: ' + str(evidence))


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, OSError) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
