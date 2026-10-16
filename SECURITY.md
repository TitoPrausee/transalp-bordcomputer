# Sicherheitsrichtlinie

> Transalp Bordcomputer — Open-Source Bordcomputer fuer die Honda Transalp 600 (PD06)

---

## Sicherheitsluecken melden

Sicherheit hat fuer dieses Projekt hoechste Prioritaet. Wenn du eine
Sicherheitsluecke findest, melde sie bitte **verantwortungsvoll** und
**nicht** als oeffentliches GitHub Issue.

### Meldeweg

1. Oeffne einen privaten Security Advisory auf GitHub:
   **Security**-Tab → **Advisories** → **Report a vulnerability**
2. Alternativ sende eine E-Mail an den Maintainer ueber das GitHub-Profil
   von [@TitoPrausee](https://github.com/TitoPrausee).

Bitte stelle dem Report folgende Informationen bei:

- Beschreibung der Schwachstelle und ihrer potenziellen Auswirkung
- Betroffene Komponente (Firmware, Companion-App, BLE-Protokoll, OTA)
- Schritte zur Reproduktion oder ein Proof-of-Concept
- Vorschlag fuer eine Behebung (falls verfuegbar)
- Firmware- bzw. App-Version, auf der der Fehler auftrat

### Was du NICHT tun solltest

- Oeffne **kein** oeffentliches GitHub Issue fuer Sicherheitsprobleme
- Veroeffentliche **keine** Details der Schwachstelle, bevor ein Fix
  veroeffentlicht wurde
- Versuche **nicht**, Schwachstellen an produktiven Geraeten auszunutzen

---

## Responsible-Disclosure-Zeitleiste

| Phase | Zeitrahmen | Beschreibung |
|---|---|---|
| 1. Eingang | 0–3 Tage | Bestaetigung des Eingangs; erste Bewertung der Schwere |
| 2. Validierung | 3–14 Tage | Ueberpruefung der Schwachstelle; Reproduktion; Schweregrad-Bewertung (CVSS) |
| 3. Behebung | 14–60 Tage | Entwicklung und Testing eines Fixes; Vorbereitung des Release |
| 4. veroeffentlichung | 60–90 Tage | Oeffentliche veroeffentlichung des Advisories nach Release des Fixes |

Anmerkungen:

- Kritische Schwachstellen (RCE, Auth-Bypass, OTA-Kompromittierung) werden
  mit hoechster Prioritaet behandelt und nach Moeglichkeit innerhalb von
  30 Tagen behoben.
- Der Reporter wird in jedem Fall genannt (sofern gewuenscht), es sei
  denn, er bittet um Anonymitaet.
- Die Bewertung des Schweregrads erfolgt nach CVSS v3.1.
- Abweichungen von der Zeitleiste werden dem Reporter mitgeteilt und
  begruendet.

---

## Unterstuetzte Versionen

Da dieses Projekt sich in aktiver Entwicklung befindet, werden
Sicherheitsupdates ausschliesslich fuer die neueste Version auf dem
`main`-Branch bereitgestellt. Es gibt keine LTS-Versionen.

| Version | Supported |
|---|---|
| `main` (latest) | Ja |
| Aeltere Releases | Nein |

---

## Sicherheitsarchitektur

Eine umfassende Sicherheitsanalyse mit Threat Model (STRIDE),
Angriffsflaechen-Bewertung und Mitigations-Plan befindet sich in:

→ [`docs/SECURITY-ANALYSIS.md`](docs/SECURITY-ANALYSIS.md)

Diese Analyse deckt folgende Bereiche ab:

- BLE-Angriffsflaeche (GATT-Server, 8 Services, 33 Characteristics)
- OTA-Update-Sicherheit (SHA-256-Verifikation, RSA-3072-Signatur,
  Dual-Partition-Rollback)
- Secure Coding-Richtlinien fuer die Firmware
- Physische Angriffsflaeche (SD-Karte, Serial Console, NVS)
- Secure-Code-Review-Checkliste

---

## Bekannte Sicherheitsmassnahmen

- **BLE-Pairing**: LE Secure Connections (ECDH P-256) mit MITM-Schutz
  und Bonding
- **OTA-Verifikation**: SHA-256-Checksumme und RSA-3072-Signatur
  (Secure Boot V2)
- **Dual-Partition-Layout**: Automatischer Rollback beim Boot-Loop
- **Konfigurations-Schutz**: NVS-BLOB mit Verschluesselung (optional)
- **Keine hardcodierten Secrets**: Sensordaten und Konfiguration in NVS,
  nicht im Source-Code

---

## Automatisierte Security-Scans

Die CI/CD-Pipeline enthaelt drei automatisierte Security-Scan-Workflows,
die bei jedem Push auf `main` und bei jedem Pull Request ausgefuehrt
werden. Die Ergebnisse werden als SARIF-Report in den GitHub Security
Tab hochgeladen.

### CodeQL Analysis

Datei: `.github/workflows/codeql.yml`

- Sprachen: C/C++ (Firmware) und JavaScript/TypeScript (Companion-App)
- Query-Suite: `security-extended` (ueber die Standard-Queries hinaus)
- Trigger: Push auf main, Pull Requests, woechentlicher Schedule-Scan
- Ergebnisse: SARIF-Upload zum GitHub Security Tab

### Semgrep CI

Datei: `.github/workflows/semgrep.yml`

- Rulesets: C, TypeScript, OWASP Top 10
- Trigger: Push auf main, Pull Requests
- Ergebnisse: SARIF-Upload zum GitHub Security Tab

### Trivy Filesystem Scan

Datei: `.github/workflows/trivy.yml`

- Scan-Ziele: Repository-Filesystem und Firmware-Binary (falls gebaut)
- Trigger: Push auf main, Pull Requests
- Ergebnisse: SARIF-Upload zum GitHub Security Tab

### Zusammenfassung der Security-Workflows

| Workflow | Scope | Trigger | Report |
|---|---|---|---|
| CodeQL | C/C++ + JS/TS | push, PR, weekly | SARIF -> Security Tab |
| Semgrep | C + TypeScript + OWASP | push, PR | SARIF -> Security Tab |
| Trivy | Filesystem + Binary | push, PR | SARIF -> Security Tab |

Zusaetzlich fuehrt die `firmware-ci`-Pipeline `cppcheck` mit
`--error-exitcode=1` als blockingen Check aus. Die `docs-ci`-Pipeline
prueft alle Dokumentationsdateien gegen die Brand-Blocklist, um
Markennamen-Verstoesse fruehzeitig zu erkennen.

---

*Transalp Bordcomputer — Open-Source Bordcomputer fuer die Honda Transalp 600 (PD06)*
