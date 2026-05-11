#!/usr/bin/env python3

import argparse
import csv
from collections import defaultdict
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Genera graficas PNG desde los CSV de resultados")
    parser.add_argument("--input", default="results/interconnect_stats.csv", help="CSV de estadisticas interconnect")
    parser.add_argument("--out-dir", default="results/plots", help="Directorio de salida de graficas")
    return parser.parse_args()


def load_rows(path: Path):
    with path.open("r", newline="", encoding="utf-8") as fp:
        return list(csv.DictReader(fp))


def require_matplotlib():
    try:
        import matplotlib.pyplot as plt
    except Exception as exc:  # pragma: no cover
        raise RuntimeError(
            "No se pudo importar matplotlib. Instala con: sudo apt install -y python3-matplotlib"
        ) from exc
    return plt


def grouped_by_trace(rows):
    grouped = defaultdict(dict)
    for row in rows:
        grouped[row["trace_label"]][row["protocol"]] = row
    return grouped


def metric(row, key):
    return float(row[key])


def make_bar_chart(plt, out_path: Path, title: str, labels, msi_vals, firefly_vals, y_label: str):
    x = list(range(len(labels)))
    width = 0.36

    fig, ax = plt.subplots(figsize=(10, 5))
    msi_bars = ax.bar([i - width / 2 for i in x], msi_vals, width=width, label="MSI")
    firefly_bars = ax.bar([i + width / 2 for i in x], firefly_vals, width=width, label="Firefly")
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=20)
    ax.set_title(title)
    ax.set_ylabel(y_label)
    ax.legend()
    ax.grid(axis="y", alpha=0.25)

    max_value = max([0.0, *msi_vals, *firefly_vals])
    ax.set_ylim(bottom=0, top=(max_value * 1.15 if max_value > 0 else 1.0))

    def annotate_bars(bars, values):
        for bar, value in zip(bars, values):
            label = f"{value:g}"
            y_pos = bar.get_height() if value > 0 else 0
            offset = 3 if value > 0 else 1
            ax.annotate(
                label,
                (bar.get_x() + bar.get_width() / 2, y_pos),
                xytext=(0, offset),
                textcoords="offset points",
                ha="center",
                va="bottom",
                fontsize=8,
            )

    annotate_bars(msi_bars, msi_vals)
    annotate_bars(firefly_bars, firefly_vals)

    if all(value == 0 for value in firefly_vals):
        ax.text(
            0.99,
            0.96,
            "Firefly = 0 en esta metrica",
            transform=ax.transAxes,
            ha="right",
            va="top",
            fontsize=8,
            color="dimgray",
        )
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)


def main() -> int:
    args = parse_args()
    csv_path = Path(args.input)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    if not csv_path.exists():
        print(f"Error: no existe CSV en {csv_path}")
        return 1

    rows = load_rows(csv_path)
    if not rows:
        print("Error: el CSV esta vacio")
        return 1

    plt = require_matplotlib()

    grouped = grouped_by_trace(rows)
    labels = sorted(grouped.keys())

    msi_total = [metric(grouped[label]["MSI"], "transacciones_totales") for label in labels]
    ff_total = [metric(grouped[label]["FIREFLY"], "transacciones_totales") for label in labels]

    msi_inv = [metric(grouped[label]["MSI"], "invalidaciones") for label in labels]
    ff_inv = [metric(grouped[label]["FIREFLY"], "invalidaciones") for label in labels]

    msi_upd = [metric(grouped[label]["MSI"], "updates") for label in labels]
    ff_upd = [metric(grouped[label]["FIREFLY"], "updates") for label in labels]

    msi_bw = [metric(grouped[label]["MSI"], "bus_tx_por_ns") for label in labels]
    ff_bw = [metric(grouped[label]["FIREFLY"], "bus_tx_por_ns") for label in labels]

    make_bar_chart(
        plt,
        out_dir / "transacciones_totales.png",
        "Transacciones de bus por trace",
        labels,
        msi_total,
        ff_total,
        "transacciones",
    )
    make_bar_chart(
        plt,
        out_dir / "invalidaciones.png",
        "Invalidaciones MSI vs Firefly",
        labels,
        msi_inv,
        ff_inv,
        "invalidaciones",
    )
    make_bar_chart(
        plt,
        out_dir / "updates.png",
        "Updates MSI vs Firefly",
        labels,
        msi_upd,
        ff_upd,
        "updates",
    )
    make_bar_chart(
        plt,
        out_dir / "bus_tx_por_ns.png",
        "Ancho de banda aproximado del bus",
        labels,
        msi_bw,
        ff_bw,
        "tx/ns",
    )

    print(f"[OK] Graficas generadas en {out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
