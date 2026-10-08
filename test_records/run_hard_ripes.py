"""Host runner only. Tests renderer-free assembly on RV32_ISS, sequentially."""
from pathlib import Path
import argparse, csv, hashlib, json, math, re, subprocess, time

ROOT = Path(__file__).resolve().parent
RIPES = Path(r'C:\Users\USER\Downloads\Ripes-v2.2.6-106-g5b8a616-win-x86_64\Ripes.exe')
NAMES = ['R', 'R2', "R'", 'B', 'B2', "B'", 'D', 'D2', "D'"]
SRC = [[1,4,2,0,3,5,6], [0,1,2,4,5,6,3], [0,2,5,3,1,4,6]]
TWIST = [[1,2,0,2,1,0,0], [0,0,0,1,2,1,2], [0]*7]

def decode(rank):
    pr, ori = divmod(rank, 729)
    available = list(range(7))
    p = []
    for i in range(7):
        q, pr = divmod(pr, math.factorial(6-i))
        p.append(available.pop(q))
    o = [0]*7
    for i in range(5,-1,-1):
        ori, o[i] = divmod(ori,3)
    o[6] = -sum(o[:6]) % 3
    return ''.join(str(x+1) for x in p+o)

def check_replay(state, tokens):
    p, o = [int(x)-1 for x in state[:7]], [int(x)-1 for x in state[7:]]
    for token in tokens:
        m = NAMES.index(token)
        f = m//3
        for _ in range(m%3+1):
            p, o = ([p[j] for j in SRC[f]],
                    [(o[SRC[f][i]]+TWIST[f][i])%3 for i in range(7)])
    return p == list(range(7)) and o == [0]*7

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--smoke', action='store_true')
    args = parser.parse_args()
    oracle_path = ROOT/'c/host-validation/host-exact-distances.bin'
    oracle = oracle_path.read_bytes()
    if len(oracle) != 3674160 or max(oracle) != 11:
        raise RuntimeError('Unexpected BFS oracle')
    states = [decode(r) for r,d in enumerate(oracle) if d == 11]
    if len(states) != 2644:
        raise RuntimeError('Expected exactly 2644 distance-11 states')
    if args.smoke:
        states = ['21345671111111', '54721631111111', states[0]]
    folder = ROOT/'evidence'/('hard-ripes-smoke' if args.smoke else 'hard-ripes-full')
    folder.mkdir(parents=True, exist_ok=True)
    template_path = folder/'solver_frozen.s'
    if not template_path.exists():
        source = (ROOT/'asm/solver_tested.s').read_text(encoding='utf-8-sig')
        source, count = re.subn(r'(?m)^\.equ RUN_TESTS,\s*\d+\s*$', '.equ RUN_TESTS, 0', source)
        if count != 1:
            raise RuntimeError('Cannot configure single-input mode')
        if 'LED_MATRIX_0_BASE' in source:
            raise RuntimeError('Renderer must be excluded')
        template_path.write_text(source, encoding='utf-8')
    source = template_path.read_text(encoding='utf-8')
    meta = {'mode': 'SMOKE ONLY' if args.smoke else 'FULL', 'processor': 'RV32_ISS',
            'ISA_extensions': [], 'RUN_TESTS': 0, 'renderer': 'excluded',
            'limit_iret_per_state': 50000000, 'states_requested': len(states),
            'template_sha256': hashlib.sha256(template_path.read_bytes()).hexdigest(),
            'oracle_sha256': hashlib.sha256(oracle).hexdigest(),
            'ripes_sha256': hashlib.sha256(RIPES.read_bytes()).hexdigest(),
            'ripes_executable': str(RIPES),
            'source_generation': 'Replace only input_state string in solver_frozen.s. active_case.s is overwritten for each case.'}
    manifest = folder/'configuration.json'
    if manifest.exists() and json.loads(manifest.read_text(encoding='utf-8')) != meta:
        raise RuntimeError('Configuration changed; use a new results folder')
    manifest.write_text(json.dumps(meta, indent=2), encoding='utf-8')
    csv_path = folder/'results.csv'
    rows = []
    if csv_path.exists():
        with csv_path.open(newline='', encoding='utf-8') as f:
            rows = list(csv.DictReader(f))
    done = {r['input'] for r in rows}
    if len(done) != len(rows) or not done.issubset(set(states)):
        raise RuntimeError('Unexpected or duplicate saved results')
    fields = ['input', 'length', 'iret', 'model_ms', 'path', 'replay', 'under_limit']
    if not csv_path.exists():
        with csv_path.open('w', newline='', encoding='utf-8') as f:
            csv.DictWriter(f, fieldnames=fields).writeheader()
    print(f'Mode: {meta["mode"]}; cases: {len(states)}; already saved: {len(done)}', flush=True)
    start = time.perf_counter()
    for state in states:
        if state in done:
            continue
        active = folder/'active_case.s'
        generated, count = re.subn(r'(?m)^input_state:\s*\.string\s*"\d{14}"',
                                  f'input_state: .string "{state}"', source)
        if count != 1:
            raise RuntimeError('Cannot replace input_state')
        active.write_text(generated, encoding='utf-8')
        report = folder/f'{state}-report.txt'
        command = [str(RIPES), '--mode', 'cli', '--src', str(active), '-t', 'asm',
                   '--proc', 'RV32_ISS', '--iret', '--exectime', '--runinfo',
                   '--timeout', '120000', '--output', str(report)]
        result = subprocess.run(command, capture_output=True, timeout=150,
                                creationflags=subprocess.CREATE_NO_WINDOW)
        console = result.stdout.decode('utf-8', errors='replace')
        errors = result.stderr.decode('utf-8', errors='replace')
        (folder/f'{state}-console.txt').write_text(console, encoding='utf-8')
        (folder/f'{state}-stderr.txt').write_text(errors, encoding='utf-8')
        if result.returncode or 'PASS: replay reaches solved state.' not in console or 'Program exited with code: 0' not in console:
            raise RuntimeError(f'{state}: Ripes run failed; inspect raw files')
        lengths = re.findall(r'Solution length:\s*(\d+)', console)
        if lengths != ['11'] or re.findall(r'Input:\s*(\d+)', console) != [state]:
            raise RuntimeError(f'{state}: incorrect input or solution length')
        path = re.search(r'Moves:\s*([^\r\n]+)', console).group(1).strip()
        tokens = path.split()
        if len(tokens) != 11 or not check_replay(state, tokens):
            raise RuntimeError(f'{state}: independent replay failed')
        raw = report.read_text(encoding='utf-8-sig')
        if 'processor: RV32_ISS' not in raw or re.search(r'ISA extensions:[ \t]*([^\r\n]*)', raw).group(1).strip():
            raise RuntimeError('Unexpected processor or ISA extensions')
        iret = int(re.search(r'instructions retired\s+(\d+)', raw).group(1))
        ms = int(re.search(r'wall-clock model execution time \(ms\)\s+(\d+)', raw).group(1))
        row = dict(zip(fields, [state, 11, iret, ms, path, 'PASS', 'PASS' if iret <= 50000000 else 'FAIL']))
        with csv_path.open('a', newline='', encoding='utf-8') as f:
            csv.DictWriter(f, fieldnames=fields).writerow(row)
        rows.append(row)
        done.add(state)
        print(f'{len(done)}/{len(states)} Input={state} iret={iret:,} limit={row["under_limit"]}', flush=True)
    worst = max(rows, key=lambda r:int(r['iret']))
    failures = sum(int(r['iret']) > 50000000 for r in rows)
    summary = (f'Mode: {meta["mode"]}\nCompleted cases: {len(rows)}/{len(states)}\n'
               f'Optimal length and independent replay: PASS for all completed cases\n'
               f'Cases exceeding 50,000,000 retired instructions: {failures}\n'
               f'Maximum retired instructions: {int(worst["iret"]):,}\n'
               f'Worst input: {worst["input"]}\n'
               f'Sum of recorded model execution time (ms): {sum(int(r["model_ms"]) for r in rows)}\n'
               f'This runner invocation wall-clock seconds: {time.perf_counter()-start:.3f}\n'
               f'Full 2644-case requirement: {"PASS" if not args.smoke and not failures and len(rows)==2644 else "NOT PASSED"}\n')
    (folder/'summary.txt').write_text(summary, encoding='utf-8')
    print(summary, flush=True)
    return int(failures != 0)

if __name__ == '__main__':
    raise SystemExit(main())
