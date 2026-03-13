# Signing Workflow (Phase 3 Placeholder)

Dieses Dokument beschreibt die vorbereiteten CI-Placeholder fuer Code-Signing.

## Workflow-Datei

- `.github/workflows/release-signing-placeholder.yml`

Der Workflow wird manuell (`workflow_dispatch`) oder bei Tag-Push (`v*`) gestartet.

## Erforderliche Secrets

### macOS

- `APPLE_CERT_BASE64`: Base64-kodiertes Signing-Zertifikat (z. B. `.p12`)
- `APPLE_CERT_PASSWORD`: Passwort fuer das Zertifikat
- `APPLE_TEAM_ID`: Apple Developer Team ID
- `APPLE_ID`: Apple Account fuer Notarisierung
- `APPLE_APP_PASSWORD`: App-spezifisches Passwort fuer `notarytool`

### Windows

- `WINDOWS_PFX_BASE64`: Base64-kodiertes Code-Signing-Zertifikat (`.pfx`)
- `WINDOWS_PFX_PASSWORD`: Passwort fuer das Zertifikat

## Aktueller Status

- CI validiert bereits das Vorhandensein aller benoetigten Secrets.
- Die eigentlichen Signier-Schritte sind bewusst als `TODO` hinterlegt, damit die Pipeline sicher vorbereitet ist, ohne falsche/unsichere Defaults zu verwenden.

## Naechster Umsetzungsschritt

- Platzhalter durch echte Build-, Signierungs- und Verifikationsschritte ersetzen (`codesign`/`notarytool` fuer macOS, `signtool` fuer Windows).
