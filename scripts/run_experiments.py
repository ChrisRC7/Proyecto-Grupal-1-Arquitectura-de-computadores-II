#!/usr/bin/env python3

import argparse
import csv
import datetime as dt
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple


TRACE_MATRIX = [
    ("propuesta_1_matmul", "traces/trace_matmul.txt"),
    ("propuesta_2_pipeline", "traces/trace_pipeline.txt"),
    ("extra_migratorio", "traces/trace_migratorio.txt"),
]
PROTOCOLS = ["msi", "firefly"]

TIMESTAMP_RE = re.compile(r"^@(\d+) ns")
INTERCONNECT_HEADER_RE = re.compile(r"^=== ESTADISTICAS INTERCONNECT \((.+)\) ===$")
INTERCONNECT_VALUE_RE = re.compile(r"^([^:]+):\s*(\d+)$")
L1_RE = re.compile(
    r"^L1_(\d+) \| RH=(\d+) RM=(\d+) WH=(\d+) WM=(\d+)"
    r"(?: EV=(\d+))?(?: CM=(\d+))?"
    r"(?: \| cfg\(size=(\d+)B, block=(\d+)B, ways=(\d+), repl=([A-Za-z]+)"
    r"(?:, l1_hit=(\d+)ns)?(?:, l1_miss=(\d+)ns)?\))?$"
)


@dataclass
class RunConfig:
    trace_label: str
    trace_path: str
    protocol: str
    l1_size: int
    block_size: int
    ways: int
    replacement: str


@dataclass
class ParsedRun:
    interconnect: Dict[str, int]
    l1_rows: List[Dict[str, object]]
    sim_time_ns: int


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Ejecuta las traces contra MSI/Firefly y exporta bitacoras + CSV."
    )
    parser.add_argument("--sim", default="./sim_proyecto", help="Ruta al ejecutable del simulador")
    parser.add_argument("--trace-gen", default="./trace_generator", help="Ruta al generador de traces")
    parser.add_argument("--output-dir", default="results", help="Directorio de salida")
    parser.add_argument("--l1-size", type=int, default=1024, help="Tamano total L1 por cache en bytes")
    parser.add_argument("--block-size", type=int, default=64, help="Tamano de bloque en bytes")
    parser.add_argument("--ways", type=int, default=2, help="Asociatividad")
    parser.add_argument(
        "--replacement",
        choices=["lru", "fifo"],
        default="lru",
        help="Politica de reemplazo",
    )
    parser.add_argument(
        "--include-extra",
        action="store_true",
        help="Incluye tambien la traza extra migratoria (por defecto solo propuestas oficiales)",
    )
    parser.add_argument(
        "--summary-file",
        default="results/resumen_experimentos.md",
        help="Archivo Markdown con un resumen agregado del lote",
    )
    return parser.parse_args()


def run_trace_generator(trace_gen: str) -> None:
    subprocess.run([trace_gen], check=True)


def build_runs(args: argparse.Namespace) -> List[RunConfig]:
    selected = TRACE_MATRIX if args.include_extra else TRACE_MATRIX[:2]
    runs: List[RunConfig] = []
    for label, trace_path in selected:
        for protocol in PROTOCOLS:
            runs.append(
                RunConfig(
                    trace_label=label,
                    trace_path=trace_path,
                    protocol=protocol,
                    l1_size=args.l1_size,
                    block_size=args.block_size,
                    ways=args.ways,
                    replacement=args.replacement,
                )
            )
    return runs


def parse_output(output: str, run: RunConfig) -> ParsedRun:
    max_time = 0
    interconnect: Dict[str, int] = {}
    l1_rows: List[Dict[str, object]] = []

    in_interconnect = False

    for raw_line in output.splitlines():
        line = raw_line.strip()

        ts_match = TIMESTAMP_RE.match(line)
        if ts_match:
            max_time = max(max_time, int(ts_match.group(1)))

        if INTERCONNECT_HEADER_RE.match(line):
            in_interconnect = True
            continue

        if in_interconnect:
            if not line:
                in_interconnect = False
            else:
                kv_match = INTERCONNECT_VALUE_RE.match(line)
                if kv_match:
                    key = kv_match.group(1).strip().lower().replace(" ", "_").replace("->", "_to_")
                    interconnect[key] = int(kv_match.group(2))

        l1_match = L1_RE.match(line)
        if l1_match:
            (cache_id, rh, rm, wh, wm, ev, cm, size_b, block_b, ways, repl, l1_hit_ns,
             l1_miss_ns) = l1_match.groups()
            total_reads = int(rh) + int(rm)
            total_writes = int(wh) + int(wm)
            l1_rows.append(
                {
                    "cache_id": int(cache_id),
                    "read_hits": int(rh),
                    "read_misses": int(rm),
                    "write_hits": int(wh),
                    "write_misses": int(wm),
                    "evictions": int(ev) if ev is not None else 0,
                    "conflict_misses": int(cm) if cm is not None else 0,
                    "read_hit_rate": (int(rh) / total_reads) if total_reads else 0.0,
                    "write_hit_rate": (int(wh) / total_writes) if total_writes else 0.0,
                    "l1_size_bytes": int(size_b) if size_b is not None else run.l1_size,
                    "block_size_bytes": int(block_b) if block_b is not None else run.block_size,
                    "associativity": int(ways) if ways is not None else run.ways,
                    "replacement": repl if repl is not None else run.replacement.upper(),
                    "l1_hit_latency_ns": int(l1_hit_ns) if l1_hit_ns is not None else 1,
                    "l1_miss_detect_latency_ns": int(l1_miss_ns) if l1_miss_ns is not None else 1,
                }
            )

    return ParsedRun(interconnect=interconnect, l1_rows=l1_rows, sim_time_ns=max_time)


def write_csv(path: Path, rows: List[Dict[str, object]], fieldnames: List[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as fp:
        writer = csv.DictWriter(fp, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def write_summary(path: Path, rows: List[Dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)

    lines = [
        "# Resumen de experimentos reproducibles",
        "",
        "Este resumen se genera a partir de `results/interconnect_stats.csv` y `results/l1_stats.csv`.",
        "",
        "| Propuesta | Protocolo | Tx bus | Invalidaciones | Updates | Downgrades M->S | Tiempo sim (ns) |",
        "|---|---|---:|---:|---:|---:|---:|",
    ]

    for row in rows:
        lines.append(
            f"| {row['trace_label']} | {row['protocol']} | {row['transacciones_totales']} | "
            f"{row['invalidaciones']} | {row['updates']} | {row['downgrades_m_to_s']} | {row['sim_time_ns']} |"
        )

    lines.extend([
        "",
        "## Conjunto formal de transacciones de bus",
        "",
        "- `BusRead`: un PE lee un bloque que no tiene localmente; el bloque puede compartirse en estado `S`.",
        "- `BusReadX`: un PE solicita el bloque para escribir sin copia previa; invalida copias remotas y obtiene exclusividad.",
        "- `BusUpgrade`: un PE ya tiene el bloque en `S` y quiere escribir; solo invalida a los demas para pasar a `M`.",
        "- `BusUpdate`: politica Firefly; una escritura propaga el nuevo valor a todas las copias remotas.",
        "- `BusInvalidate`: transaccion interna para invalidaciones remotas cuando corresponde al protocolo MSI.",
        "",
    ])

    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    args = parse_args()

    output_dir = Path(args.output_dir)
    logs_dir = output_dir / "logs"
    logs_dir.mkdir(parents=True, exist_ok=True)

    workspace = Path.cwd()
    sim_path = (workspace / args.sim).resolve()
    trace_gen = (workspace / args.trace_gen).resolve()

    if not sim_path.exists():
        print(f"Error: no existe simulador en {sim_path}", file=sys.stderr)
        return 1
    if not trace_gen.exists():
        print(f"Error: no existe trace_generator en {trace_gen}", file=sys.stderr)
        return 1

    print("[INFO] Generando traces...")
    run_trace_generator(str(trace_gen))

    runs = build_runs(args)
    executed_at = dt.datetime.now().isoformat(timespec="seconds")

    interconnect_rows: List[Dict[str, object]] = []
    l1_rows: List[Dict[str, object]] = []

    for run in runs:
        cmd = [
            str(sim_path),
            run.trace_path,
            run.protocol,
            str(run.l1_size),
            str(run.block_size),
            str(run.ways),
            run.replacement,
        ]
        print("[INFO] Ejecutando:", " ".join(cmd))
        proc = subprocess.run(cmd, check=True, capture_output=True, text=True)

        run_key = f"{run.trace_label}_{run.protocol}"
        log_path = logs_dir / f"{run_key}.log"
        log_path.write_text(proc.stdout, encoding="utf-8")

        parsed = parse_output(proc.stdout, run)
        total = int(parsed.interconnect.get("transacciones_totales", 0))
        sim_time = max(1, parsed.sim_time_ns)

        interconnect_rows.append(
            {
                "executed_at": executed_at,
                "trace_label": run.trace_label,
                "trace_path": run.trace_path,
                "protocol": run.protocol.upper(),
                "sim_time_ns": parsed.sim_time_ns,
                "transacciones_totales": total,
                "lecturas": int(parsed.interconnect.get("lecturas", 0)),
                "escrituras": int(parsed.interconnect.get("escrituras", 0)),
                "invalidaciones": int(parsed.interconnect.get("invalidaciones", 0)),
                "updates": int(parsed.interconnect.get("updates", 0)),
                "downgrades_m_to_s": int(parsed.interconnect.get("downgrades_m_to_s", 0)),
                "bus_tx_por_ns": f"{(total / sim_time):.6f}",
                "log_file": str(log_path),
            }
        )

        for row in parsed.l1_rows:
            l1_rows.append(
                {
                    "executed_at": executed_at,
                    "trace_label": run.trace_label,
                    "trace_path": run.trace_path,
                    "protocol": run.protocol.upper(),
                    **row,
                }
            )

    write_csv(
        output_dir / "interconnect_stats.csv",
        interconnect_rows,
        [
            "executed_at",
            "trace_label",
            "trace_path",
            "protocol",
            "sim_time_ns",
            "transacciones_totales",
            "lecturas",
            "escrituras",
            "invalidaciones",
            "updates",
            "downgrades_m_to_s",
            "bus_tx_por_ns",
            "log_file",
        ],
    )

    write_csv(
        output_dir / "l1_stats.csv",
        l1_rows,
        [
            "executed_at",
            "trace_label",
            "trace_path",
            "protocol",
            "cache_id",
            "read_hits",
            "read_misses",
            "write_hits",
            "write_misses",
            "evictions",
            "conflict_misses",
            "read_hit_rate",
            "write_hit_rate",
            "l1_size_bytes",
            "block_size_bytes",
            "associativity",
            "replacement",
            "l1_hit_latency_ns",
            "l1_miss_detect_latency_ns",
        ],
    )

    summary_rows = [
        {
            "trace_label": row["trace_label"],
            "protocol": row["protocol"],
            "transacciones_totales": row["transacciones_totales"],
            "invalidaciones": row["invalidaciones"],
            "updates": row["updates"],
            "downgrades_m_to_s": row["downgrades_m_to_s"],
            "sim_time_ns": row["sim_time_ns"],
        }
        for row in interconnect_rows
    ]
    write_summary(Path(args.summary_file), summary_rows)

    print(f"[OK] CSV generado: {output_dir / 'interconnect_stats.csv'}")
    print(f"[OK] CSV generado: {output_dir / 'l1_stats.csv'}")
    print(f"[OK] Bitacoras: {logs_dir}")
    print(f"[OK] Resumen: {Path(args.summary_file)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
