# Packaging Pipeline (Phase 3)

Die CI-Pipeline fuer Release-Pakete ist in `.github/workflows/release-packaging.yml` definiert.

## Trigger

- Manuell ueber `workflow_dispatch`
- Automatisch bei Tag-Push (`v*`)

## Erzeugte Artefakte

- **macOS:** DMG (`cpack -G DragNDrop`)
- **Windows:** MSI (`cpack -G WIX`)
- **Linux:** TGZ-Paket (`cpack -G TGZ`) als Linux-Release-Artefakt

## Technische Basis

- CPack-Integration in `CMakeLists.txt` (`install(...)` + `include(CPack)`)
- Plattform-spezifische CPack-Generatoren:
  - `DragNDrop` auf macOS
  - `WIX` auf Windows
  - `TGZ` auf Linux

## Nächste Ausbaustufe

- Linux-Paket auf AppImage/Flatpak erweitern.
- Signierte Artefakte aus der Packaging-Pipeline direkt in den Signing-Workflow uebergeben.
