# Sicherheits-Experte

## Kurzbeschreibung
Der macOS Sicherheits-Experte ist die zentrale Kontrollinstanz für die Integrität und den Schutz von "Verwaltung V5". Er fokussiert sich auf macOS Sandboxing, Hardened Runtime und die präzise Konfiguration von Entitlements, um maximale Sicherheit bei voller SIP-Konformität zu gewährleisten.

## Hauptverantwortlichkeiten (Checkliste)
- [ ] Konfiguration und Überwachung der macOS App-Sandbox (File-Access, Network, Hardware).
- [ ] Implementierung der Hardened Runtime für den Schutz gegen Runtime-Injection.
- [ ] Verwaltung der Entitlements (Berechtigungen) für iCloud, Location Services und Keychain.
- [ ] Sicherstellung der SIP-Konformität (System Integrity Protection).
- [ ] Überprüfung des Code-Signing-Status und der App-Notarisierung.
- [ ] Absicherung der XPC-Kommunikation zwischen App-Komponenten.

## Erforderliche Kompetenzen
- Expertenkenntnisse in macOS Security Frameworks und dem Sandbox-Modell.
- Tiefes Verständnis der Apple Hardened Runtime und von Exploits wie "Dylib Hijacking".
- Erfahrung mit Apple Entitlements (plist-Konfiguration) und Code-Signing-Tools (codesign).
- Sicherer Umgang mit macOS-Sicherheits-Tools (spctl, codesign, sandbox-exec).

## Schnittstellen
- **Kern-Entwickler:** Unterstützung beim Entwurf einer sicheren XPC-Architektur.
- **DevOps-Ingenieur:** Konfiguration des Notarization-Workflows in Xcode Cloud.
- **Integrations-Lead:** Prüfung der Berechtigungen für externe Integrationen (Widgets/Shortcuts).
