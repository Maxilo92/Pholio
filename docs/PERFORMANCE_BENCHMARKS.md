# Performance Benchmarks (Phase 3 RC)

Dieses Dokument definiert den reproduzierbaren Startpunkt fuer Performance-Messungen in Phase 3.

## Benchmark 1: `StructureAnalyzer.generatePath`

- Binary: `PholioBenchmarks`
- Quelle: `tests/benchmarks/StructureAnalyzerBenchmark.cpp`
- Ziel: Pfad-Generierung fuer grosse Dateimengen unter stabilen Bedingungen messen.

## Datensatz-Definition

- Synthetischer Datensatz mit standardisiertem Muster:
  - Eingabepfade: `/input/IMG_<N>.JPG`
  - Zeitstempel: deterministisch ueber Monat/Tag/Uhrzeit verteilt
  - Template: `{yyyy}-{MM}-{dd}_{original_name}.{ext}`
- Standardgroesse: `200000` Iterationen

## Gemessene Metriken

- `elapsed_seconds`
- `ops_per_second`
- `ns_per_op`
- `checksum` (Guard gegen Dead-Code-Eliminierung)

## Ausfuehrung

```bash
cmake --build build -j4 --target PholioBenchmarks
./build/PholioBenchmarks 200000
```

## Baseline und RC-Zielwerte

### Aktuelle Baseline (lokaler Referenzlauf)

- `iterations`: `200000`
- `elapsed_seconds`: `1.540955`
- `ops_per_second`: `129789.64`
- `ns_per_op`: `7704.77`

### RC-Zielwerte (naechste Optimierungsstufe)

- `ops_per_second`: mindestens `150000`
- `ns_per_op`: maximal `7000`

Die Zielwerte werden nach ersten Messlaeufen auf realen SSD/NVMe-Systemen weiter kalibriert.
