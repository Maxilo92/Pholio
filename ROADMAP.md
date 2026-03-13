# 🗺️ Pholio Roadmap

Dieses Dokument beschreibt die geplanten Entwicklungsphasen und Meilensteine für **Pholio**. Da sich das Projekt in aktiver Entwicklung befindet, können sich Prioritäten und Zeitpläne ändern.

---

## 🏗️ Phase 1: Alpha (Aktueller Status) - Fundament & Stabilität
*Fokus: Kern-Engine, grundlegende Benutzeroberfläche und robuste Dateioperationen.*

- [x] **Core Engine:** Implementierung von Scanner, Sorter und MediaAnalyzer.
- [x] **Metadata-Extraction:** Unterstützung für EXIF (Bilder) und libavformat (Videos).
- [x] **ImGui UI:** Docking-Framework, Progress-Window, Settings und Live-Logs.
- [x] **Update-System:** Integrierter Updater für macOS (App-Bundle Support) und GitHub-API Integration.
- [x] **Metriken:** Echtzeit-Statistiken (MB/s, FPS) mit ImPlot Visualisierung.
- [x] **Sicherheit:** Checksummen-Verifizierung (MD5/XXHash) und "No-Loss Policy".
- [x] **Plattformen:** Grundlegender Support für macOS und Linux.
- [x] **Struktur-Migration:** Finale Implementierung der "Umbau/Merge/Weiterführen" Logik.
- [x] **Dry Run:** Simulation des Sortiervorgangs ohne tatsächliche Schreibvorgänge.

---

## 🧪 Phase 2: Beta - Feature-Vervollständigung & Polishing
*Fokus: Erweiterung des Funktionsumfangs und Verbesserung der Benutzererfahrung.*

- [x] **Format-Konvertierung:** Vollständige Integration von Magick++ und FFmpeg für On-the-fly Konvertierung.
- [x] **Erweitertes Duplikat-Management:** Visueller Vergleich von Duplikaten vor dem Überschreiben.
- [x] **Custom Templates:** Flexiblere Namensschemata durch Benutzer-definierte Variablen.
- [ ] **Internationalisierung (I18n):** Unterstützung für mehrere Sprachen (Deutsch/Englisch initial).
- [ ] **UI/UX Refinement:** Themes (Dark/Light Mode), verbesserte Icons und Layout-Presets.
- [ ] **Fehler-Reporting:** Automatisierter Export von Absturzberichten und Diagnose-Logs.
- [ ] **Windows Support:** Portierung und Validierung des Build-Systems für Windows (MSVC).

---

## 🚀 Phase 3: Release Candidate (1.0) - Produktionsreife
*Fokus: Performance-Optimierung, Dokumentation und Deployment.*

- [ ] **Optimierung:** Multithreading-Feinschliff für maximale Performance bei SSD/NVMe-Laufwerken.
- [ ] **Qualitätssicherung:** Umfassende Unit- und Integrationstests für alle Kernmodule.
- [ ] **Dokumentation:** Vollständiges Benutzerhandbuch und technische API-Dokumentation.
- [ ] **Code Signing:** Signierung und Notarisierung für macOS; Code-Signing für Windows.
- [ ] **Packaging:** Erstellung von stabilen Installern (DMG, MSI, AppImage/Flatpak).
- [ ] **V1.0 Launch:** Offizieller Release der ersten stabilen Version.

---

## 🌟 Phase 4: Post 1.0 - Zukünftige Erweiterungen
*Fokus: Intelligente Funktionen und Ökosystem-Erweiterung.*

- [ ] **Gesichtserkennung:** Datenschutz-konforme Erkennung von Personen (lokale Verarbeitung).
- [x] **Plugin-System:** API für Community-basierte Sortierlogiken und Filter.
- [ ] **Archiv-Validierung:** Regelmäßiger Hintergrund-Check der Archiv-Integrität (Bit-Rot Protection).

---

*Zuletzt aktualisiert: 13. März 2026*
