# Implementierungsplan - PhotoSorter

## Phase 1: Projekt-Setup & Infrastruktur
1. CMake Setup mit vcpkg für alle Bibliotheken.
2. Dear ImGui Grundgerüst mit Docking.
3. ImPlot Integration für Live-Metriken.
4. `ConfigManager` für persistente JSON-Einstellungen.
5. Portable File Dialogs (pfd) für native Ordnerauswahl.

## Phase 2: Engine & Resiliente Analyse
1. Exiv2 (Bilder) und libavformat (Videos) Integration.
2. Multithreaded Scanner für Quelle und Ziel.
3. `MediaAnalyzer` mit Fallback-Kette (EXIF > Date Mod > Filename Date).
4. `StructureAnalyzer` zur Erkennung bestehender Archivstrukturen.

## Phase 3: Business Logik & Validierung
1. Settings-UI mit allen Parametern (Migration, Renaming, Conversion).
2. `MigrationManager` Dialog (Umbau, Merge, Continue).
3. `Verifier` Modul für MD5/XXHash Checksummen.
4. `Converter` & `Renamer` Module (Magick++/FFmpeg).
5. Implementierung der atomaren "Copy-Verify-Delete" Kette.

## Phase 4: Performance & Live-Monitoring
1. Background Worker Thread mit "Safe Cancel" Logik.
2. Atomic Counters und Thread-sicherer Log-Puffer (Lock-free oder Mutex-geschützt).
3. Implementierung des **Live-Log-Windows** mit Filteroptionen (Erfolg/Fehler/Warnung).
4. ImPlot Visualisierung von Durchsatz (MB/s) und Fehlern.
5. ETA-Rechner mit gleitendem Durchschnitt.
6. Bildvorschau (stb_image/FFmpeg).

## Phase 5: Logging & Deployment
1. Summary-Window mit farblich markiertem Log (Success, Warning, Error).
2. CSV-Export aller Aktionen als Transaktionsbeleg.
3. Robustes Exception Handling (z.B. Disk Full, Read Error).
4. Build-Optimierung (Release Mode) und Final Testing.
