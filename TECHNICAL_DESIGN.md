# Technisches Design & Architektur

## Technologiestack
- **Programmiersprache:** C++20.
- **GUI:** Dear ImGui Docking (Glfw + OpenGL3).
- **Grafiken:** ImPlot für Verlaufsstatistiken.
- **Metadaten:** Exiv2 (Bilder) + FFmpeg/libavformat (Video-Metadaten).
- **Konvertierung:** Magick++ (ImageMagick) + FFmpeg (Transkodierung).
- **Abhängigkeiten:** vcpkg (Paketmanager für konsistente Builds).
- **Konfiguration:** nlohmann/json für persistente Einstellungen.
- **Bildvorschau:** stb_image + FFmpeg (Frame Extraction).
- **Dateisystem:** std::filesystem.

## Komponenten-Architektur
### 1. Engine (Background Thread)
- **Scanner:** Rekursives Einlesen mit Multithreading-Support für schnelles Scanning.
- **StructureAnalyzer:** Regex-basierte Mustererkennung für Zielordner.
- **MediaAnalyzer:** Resiliente Extraktion (EXIF -> File Date -> Path Date Fallback).
- **Converter & Renamer:** Modulare Transformatoren gesteuert durch den ConfigManager.
- **Verifier:** Generiert Checksummen (XXHash oder MD5) für Quell- und Zieldateien.
- **Sorter:** Führt Dateioperationen atomar aus (soweit möglich) und loggt jede Transaktion.

### 2. Frontend (Main Thread)
- **Settings-Window:** Pfade, Schemata, Dubletten, Konvertierung, Checksum-Level.
- **Progress-Window:** Echtzeit-Graphen, Vorschau, ETA-Berechnung (Moving Average).
- **Live-Log-Window:** Optional einblendbares Fenster mit Thread-sicherem Puffer für alle Aktionen während des Prozesses.
- **Summary-Window:** Filterbares Log am Ende des Prozesses mit Export-Funktion.

## Sicherheitskonzept (No-Loss Policy)
- **Checksum Verification:** Vergleich von Hashwerten nach dem Transfer.
- **Atomic-ish Operations:** Kopieren + Verifizieren + (optional) Löschen der Quelle erst nach Erfolg.
- **Safe Cancel:** Signal-System zum sauberen Stoppen des Hintergrundthreads ohne Datenverlust.
- **Dry Run:** Komplette Simulation des Pfadbaus ohne Schreibzugriff.
