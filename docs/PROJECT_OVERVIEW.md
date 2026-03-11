# Pholio C++ - Projektübersicht

## Zielsetzung
Entwicklung einer robusten Desktop-Anwendung zur automatisierten Sortierung von Fotos und Videos basierend auf deren Metadaten (EXIF). Die Anwendung bietet eine moderne grafische Benutzeroberfläche mit Echtzeit-Statistiken und höchster Datensicherheit.

## Hauptfunktionen
- **Multi-Format Support:** Unterstützung für gängige Bildformate (JPEG, PNG, HEIC, WEBP, TIFF, RAW) und Videoformate (MP4, MOV, AVI, MKV).
- **Automatisierte Sortierung:** Bilder/Videos werden nach Datum in eine Ordnerstruktur (z. B. `Jahr/Monat-Name/Tag/`) verschoben oder kopiert.
- **Intelligente Zielordner-Analyse:** Erkennt bei der Auswahl des Zielordners automatisch die bestehende Struktur.
- **Struktur-Migration:** Bietet bei Erkennung einer abweichenden Struktur drei Optionen:
    1. **Umbau:** Bestehende Dateien im Zielordner werden in das neue Schema verschoben.
    2. **Merge:** Neue Bilder werden nahtlos in die bestehende Struktur eingefügt (auch wenn diese vom Standard abweicht).
    3. **Weiterführen:** Das erkannte alte Schema wird für alle neuen Importe übernommen.
- **Vollständige Konfigurierbarkeit:** ...
- **Formaterkennung & Konvertierung (Optional):** Erkennt Format-Mismatches und bietet Konvertierung an; kann in den Einstellungen global deaktiviert oder auf "Nachfragen" gestellt werden.
- **Dateibenennung (Optional):** Flexible Namensschemata (z.B. Erhalt des Originalnamens vs. Datum-basiert), vollständig deaktivierbar.
- **Intuitive GUI:** Dear ImGui Docking Oberfläche für Setup, Fortschrittsanzeige und Zusammenfassung.
- **Echtzeit-Metriken:** Anzeige von Verarbeitungsgeschwindigkeit (Bilder/s, MB/s) mit grafischer Darstellung (ImPlot).
- **Vorschau-Funktion:** Anzeige des aktuell verarbeiteten Bildes/Videos während des Vorgangs.
- **Echtzeit-Log-Einsicht:** Während des Prozesses kann ein detailliertes Log-Fenster eingeblendet werden, das jede Aktion (Erfolg, Warnung, Fehler) in Echtzeit anzeigt.
- **Fortschrittskontrolle:** Detaillierter Ladebalken mit verstrichener und verbleibender Zeit.
- **Duplikatsmanagement:** Einstellbares Verhalten für doppelte Dateien (Überspringen, Umbenennen, Ersetzen).

## Datensicherheit (No-Loss Policy)
- Verifizierung der Dateigrößen nach dem Transfer.
- Fehlerprotokollierung für Dateien, die nicht gelesen oder verschoben werden konnten.
- Optionaler "Copy instead of Move" Modus für maximale Sicherheit.
