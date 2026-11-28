#!/usr/bin/env python3
"""Keep whole-game completion status synchronized with the repository.

Read progress/full_game_progress.json. There is deliberately no invented
percentage per area: 44% is the retained manual whole-game estimate.
Use --sync after a documented milestone and --check in continuous integration.
"""
import argparse
from datetime import date
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "progress/full_game_progress.json"
README = ROOT / "README.md"
REPORT = ROOT / "docs/full-game-progress.md"
START = "<!-- FULL_GAME_PROGRESS_START -->"
END = "<!-- FULL_GAME_PROGRESS_END -->"
VALID_STATES = {"in_progress", "unverified", "planned", "verified"}


def bar(percent):
    filled = (percent * 25 + 50) // 100
    return "█" * filled + "░" * (25 - filled)


def validate(d):
    if d.get("schema_version") != 1:
        raise ValueError("Incorrect tracker schema")
    p = d["whole_game_progress"]
    v = p["completed_percent"]
    rem = p["remaining_percent"]
    if type(v) is not int or not 0 <= v <= 100:
        raise ValueError("Completion must be integer from 0 to 100")
    if type(rem) is not int or rem != 100 - v:
        raise ValueError("Incorrect remaining percentage")
    if p["source_scenes_implemented"] != len(p["source_scenes"]):
        raise ValueError("Scene count does not match implemented scene list")
    total = p["original_total_scenes"]
    if total is not None and (type(total) is not int or
                              total < p["source_scenes_implemented"]):
        raise ValueError("Incorrect original scene denominator")
    date.fromisoformat(d["snapshot_date"])
    areas = d["areas"]
    if not areas or len({a["id"] for a in areas}) != len(areas):
        raise ValueError("Areas require unique IDs")
    if any(a["status"] not in VALID_STATES or
           not all(a.get(k) for k in ("title", "evidence", "missing"))
           for a in areas):
        raise ValueError("Bad area data")
    gates = d["required_for_100"]
    if not gates or len({g["id"] for g in gates}) != len(gates):
        raise ValueError("Release gates require unique IDs")
    if any(type(g["verified"]) is not bool for g in gates):
        raise ValueError("Release gate must be boolean")
    if v == 100 and (total is None or not all(g["verified"] for g in gates)):
        raise ValueError("Cannot claim 100% without a verified scene count and release gates")
    history = d["history"]
    if not history or history[-1]["completed_percent"] != v or \
       history[-1]["date"] != d["snapshot_date"]:
        raise ValueError("Progress history must match current estimate")


def readme_block(d):
    p = d["whole_game_progress"]
    v = p["completed_percent"]
    rem = p["remaining_percent"]
    return "\n".join([
        START,
        "### Completamento del gioco intero (stima)",
        "",
        "**[" + bar(v) + "] " + str(v) + "% completato · " +
        str(rem) + "% rimanente**",
        "",
        "Riferito al **porting completo**, non alla ROM dimostrativa.",
        "La percentuale è una stima provvisoria, non una verifica di fedeltà.",
        "",
        "[Dettagli, aree mancanti e storico](docs/full-game-progress.md)",
        END,
    ])


def report(d):
    p = d["whole_game_progress"]
    v = p["completed_percent"]
    rem = p["remaining_percent"]
    original_total = p["original_total_scenes"]
    original_label = "non ancora accertato" if original_total is None else str(original_total)
    lines = [
        "# Stato di completamento — Volvy's Adventure ReSlimed GBA",
        "",
        "Aggiornato: " + d["snapshot_date"] + " | Versione: " +
        d["current_milestone"],
        "",
        "**[" + bar(v) + "] " + str(v) + "% completato · " +
        str(rem) + "% rimanente**",
        "",
        "**Stima provvisoria dell'intero gioco.** " +
        p["note"],
        "",
        "**Scene presenti nei sorgenti:** " +
        str(p["source_scenes_implemented"]) +
        " | **Totale scene originali:** " + original_label + ".",
        "",
    ]
    lines += ["- " + s for s in p["source_scenes"]]
    lines += ["", "## Aree del progetto", "",
              "| Area | Stato | Evidenza | Ancora da completare |",
              "|---|---|---|---|"]
    for a in d["areas"]:
        lines.append("| " + " | ".join([
            a["title"], a["status"], a["evidence"], a["missing"]
        ]) + " |")
    lines += ["", "## Requisiti obbligatori per il 100%", ""]
    lines += ["- [" + ("x" if g["verified"] else " ") + "] " +
              g["label"] for g in d["required_for_100"]]
    lines += ["", "## Storico", "",
              "| Data | Versione | Progresso | Motivo |",
              "|---|---|---:|---|"]
    lines += ["| " + h["date"] + " | " + h["milestone"] +
              " | " + str(h["completed_percent"]) + "% | " +
              h["note"] + " |" for h in d["history"]]
    lines += [
        "",
        "Il valore complessivo non cresce automaticamente con nuovi "
        "test, documenti o ROM tecniche. Cambiarlo soltanto quando "
        "un milestone del gioco completo è stato implementato e verificato.",
        "",
        "Fonte strutturata: progress/full_game_progress.json",
        "",
    ]
    return "\n".join(lines)


def apply_block(original, new_block):
    if START not in original and END not in original:
        heading = "# Volvy's Adventure: Reslimed — GBA homebrew adaptation"
        if heading not in original:
            raise ValueError("Project heading not found")
        return original.replace(heading, heading + "\n\n" + new_block, 1)
    if original.count(START) != 1 or original.count(END) != 1:
        raise ValueError("Invalid marker count")
    left = original.index(START)
    right = original.index(END) + len(END)
    if right <= left:
        raise ValueError("Invalid progress marker order")
    return original[:left] + new_block + original[right:]


def main():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--sync", action="store_true")
    group.add_argument("--check", action="store_true")
    args = parser.parse_args()

    d = json.loads(SOURCE.read_text(encoding="utf-8"))
    validate(d)
    old_readme = README.read_text(encoding="utf-8")
    new_readme = apply_block(old_readme, readme_block(d))
    new_report = report(d)
    if args.sync:
        README.write_text(new_readme, encoding="utf-8")
        REPORT.write_text(new_report, encoding="utf-8")
        print("Progresso gioco completo: documenti sincronizzati")
    elif args.check:
        if new_readme != old_readme or not REPORT.exists() or \
           REPORT.read_text(encoding="utf-8") != new_report:
            raise SystemExit("Tracker non aggiornato: eseguire --sync")
        print("Progresso gioco completo: PASS (JSON, README, storico, 100% gates)")
    else:
        print(readme_block(d))


if __name__ == "__main__":
    main()
