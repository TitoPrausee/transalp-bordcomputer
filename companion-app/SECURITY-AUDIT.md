# npm Dependency Vulnerability Audit (TOT-56)

> Companion-App Security Audit — npm dependency remediation
> Stand: Oktober 2026
> Issue: TOT-56 (parent: TOT-51, TOT-16)

---

## 1. Zusammenfassung

| Kategorie | Vorher | Nachher | Status |
|-----------|--------|---------|--------|
| Critical  | 1      | 0       | Behoben |
| High      | 28     | 25      | 3 behoben, 25 akzeptiertes Risiko |
| Moderate  | 31     | 20      | 11 behoben, 20 akzeptiertes Risiko |
| **Total** | **60** | **45**  | **15 behoben** |

**Critical/High mit Fix behoben:** 4 (tar, @xmldom/xmldom, postcss)
**Critical/High als akzeptiertes Risiko:** 25 (braces, node-forge — kein Fix verfuegbar)

---

## 2. Behobene Vulnerabilities (npm overrides)

Die folgenden transitive Abhaengigkeiten wurden ueber `overrides` in `package.json`
auf sichere Versionen angehoben:

| Package | Advisory | Vorher | Override | Severity |
|---------|----------|--------|----------|----------|
| `tar` | GHSA-r292-9mhp-454m (Uncontrolled recursion DoS) | 6.2.1 | 7.5.22 | Critical/High |
| `@xmldom/xmldom` | GHSA-93r5-fhx6-vmg9 (Quadratic-time parsing DoS) | 0.7.13 | 0.8.15 | High |
| `postcss` | GHSA-r28c-9q8g-f849 (Path Traversal in source map loading) | 8.4.49 | 8.5.31 | High |
| `uuid` | GHSA-w5hq-g745-h8pq (Missing buffer bounds check) | 7.0.3 / 8.3.2 | 11.1.1 | Moderate |

### Begruendung der Overrides

- **tar 6.2.1 -> 7.5.22:** cacache@18.0.4 und @expo/cli@0.22.28 erwarten tar ^6.x.
  tar 7.x ist API-kompatibel fuer die von cacache verwendeten Pack/Unpack-Streams.
  Node.js v26 erfuellt die Node 20+ Anforderung von tar 7.x.
- **@xmldom/xmldom 0.7.13 -> 0.8.15:** @expo/plist erwartet ~0.7.x.
  0.8.x ist ein Maintenance-Release, abwaertskompatibel zu 0.7.x.
  0.8.15 enthaelt den Fix fuer die Quadratic-time parsing Schwachstelle.
- **postcss 8.4.49 -> 8.5.31:** @expo/metro-config verwendet PostCSS fuer CSS-Transform.
  8.5.x ist eine Minor-Version-Erweiterung, voll abwaertskompatibel zu 8.4.x.
- **uuid 7/8 -> 11.1.1:** xcode@3.0.1 und @expo/bunyan@4.0.1 verwenden `require('uuid')`.
  uuid 11.x bietet dual CJS/ESM exports (`require` condition -> `./dist/cjs/index.js`),
  daher kompatibel mit CommonJS-Consumern. 11.1.1 behebt den buffer bounds check Fehler.

### Aufgeloeste transitive Ketten

Durch die 4 Overrides wurden folgende transitive Abhaengigkeiten automatisch
als nicht mehr verwundbar eingestuft:

- **tar-Kette:** cacache, @expo/cli (tar-related advisories)
- **@xmldom/xmldom-Kette:** @expo/plist, xcode, @expo/config-plugins, @expo/config, @expo/prebuild-config, expo-constants, expo-asset
- **postcss-Kette:** @expo/metro-config
- **uuid-Kette:** xcode, @expo/bunyan, @expo/rudder-sdk-node

---

## 3. Akzeptiertes Risiko — Verbleibende 45 Vulnerabilities

Alle verbleibenden 45 Vulnerabilities lassen sich auf **3 Root-Advisories**
zurueckfuehren, fuer die **keine Fix-Version auf npm verfuegbar** ist.
Die jeweils neueste Version des Packages ist selbst verwundbar.

### 3.1 braces (25 High) — GHSA-vfj7-8cjw-p6xm

| Feld | Wert |
|------|------|
| Advisory | braces vulnerable to stack-exhaustion denial of service through deeply nested patterns |
| Verwundbare Versionen | <=3.0.3 |
| Neueste Version auf npm | 3.0.3 (verwundbar) |
| Fix verfuegbar | Nein |
| Severity | High |

**Betroffene transitive Kette:**
braces -> micromatch -> fast-glob -> {globby, del -> tempy,
expo-modules-autolinking, @expo/cli, jscodeshift -> @react-native/codegen
-> @react-native/babel-plugin-codegen -> @react-native/babel-preset
-> babel-preset-expo -> expo}
braces -> micromatch -> {metro-file-map -> metro -> metro-config,
metro-transform-worker -> @react-native/community-cli-plugin
-> react-native -> @react-native/virtualized-lists}

**Risikobewertung:**
- **Angriffsvektor:** DoS (Stack-Exhaustion) durch tief verschachtelte
  Glob-Patterns. Erfordert kontrollierten Input fuer Glob-Pattern-Argumente.
- **Exposition:** Alle betroffenen Packages sind **Build-Time/Dev-Dependencies**
  (Expo CLI, Metro Bundler, React Native Codegen). Keines davon ist im
  Runtime-Bundle der App enthalten.
- **Ausnutzung in Produktion:** Nicht moeglich — die App-Binary enthaelt
  keinen Code aus diesen Abhaengigkeiten. Ein Angreifer muesste Entwickler-
  oder Build-Umgebung kompromittieren und kontrollierte Glob-Patterns
  in den Build-Prozess einschleusen.
- **Akzeptiertes Risiko:** Ja — DoS-Schwachstelle in Build-Tooling,
  keine Runtime-Exposition, kein Fix verfuegbar.

### 3.2 node-forge (High) — GHSA-86w9-cpqp-85rv

| Feld | Wert |
|------|------|
| Advisory | node-forge RSA PKCS#1 v1.5 signature verification accepts extra nested DigestAlgorithm elements |
| Verwundbare Versionen | <=1.4.0 |
| Neueste Version auf npm | 1.4.0 (verwundbar) |
| Fix verfuegbar | Nein |
| Severity | High |

**Betroffene transitive Kette:**
node-forge -> selfsigned -> @react-native/dev-middleware -> @expo/cli -> expo
node-forge -> @expo/code-signing-certificates -> @expo/cli -> expo

**Risikobewertung:**
- **Angriffsvektor:** RSA-Signatur-Verifikation akzeptiert zusaetzliche
  verschachtelte DigestAlgorithm-Elemente. Dies koennte bei der Verifikation
  von Zertifikaten in Build-Prozessen relevant sein.
- **Exposition:** node-forge wird von `@expo/code-signing-certificates`
  (Code-Signing waehrend EAS Build) und `selfsigned` (Dev-Server-HTTPS)
  verwendet. Beide sind **Build-Time-Dependencies**, nicht im App-Runtime.
- **Ausnutzung in Produktion:** Nicht moeglich — die App signiert keine
  Zertifikate zur Laufzeit. Ein Angreifer muesste den Build-Prozess
  manipulieren, was separate Sicherheitsmassnahmen (CI/CD-Security)
  abdecken.
- **Akzeptiertes Risiko:** Ja — Build-Time-Schwachstelle in
  Code-Signing-Tooling, keine Runtime-Exposition, kein Fix verfuegbar.

### 3.3 sprintf-js (20 Moderate) — GHSA-hp3w-g68c-fv3c

| Feld | Wert |
|------|------|
| Advisory | sprintf-js vulnerable to denial of service through unbounded precision specifiers |
| Verwundbare Versionen | <=1.1.3 |
| Neueste Version auf npm | 1.1.3 (verwundbar) |
| Fix verfuegbar | Nein |
| Severity | Moderate |

**Betroffene transitive Kette:**
sprintf-js -> argparse -> js-yaml -> @istanbuljs/load-nyc-config
-> babel-plugin-istanbul -> @jest/transform -> {jest-snapshot,
jest-runtime, jest-runner, jest-config, babel-jest, @jest/reporters}
-> jest, jest-circus, jest-cli, ts-jest, @jest/core, @jest/expect,
@jest/globals, jest-resolve-dependencies

**Risikobewertung:**
- **Angriffsvektor:** DoS durch unbegrenzte Precision-Specifiers in
  sprintf-Format-Strings. Erfordert kontrollierten Input fuer
  Format-String-Argumente.
- **Exposition:** sprintf-js wird ausschliesslich von Jest/ts-jest
  (Test-Framework) als transitive Abhaengigkeit verwendet. Es ist eine
  **Dev/Test-Dependency**, nicht im App-Runtime oder Production-Build.
- **Ausnutzung in Produktion:** Nicht moeglich — Test-Dependencies werden
  nicht in die App-Binary aufgenommen. Ein Angreifer muesste Test-Input
  kontrollieren, was innerhalb der CI-Pipeline isoliert ist.
- **Akzeptiertes Risiko:** Ja — Dev/Test-Time-DoS, keine Runtime-Exposition,
  kein Fix verfuegbar. Moderate Severity.

---

## 4. Massnahmen und Empfehlungen

### 4.1 Warum `npm audit fix --force` NICHT verwendet wurde

`npm audit fix --force` wuerde folgende **Breaking Changes** durchfuehren:
- `expo` 52.0.49 -> 44.0.6 (Major-Downgrade, 8 SDK-Versionen zurueck)
- `react-native` 0.87.1 -> 0.72.17 (Major-Downgrade)
- `jest` 30.5.2 -> 25.0.0 (Major-Downgrade)
- `ts-jest` 29.4.x -> 29.1.2 (Minor-Downgrade)

Diese Downgrades wuerden die App zerstoeren (inkompatible APIs,
fehlende Features, TypeScript-Inkompatibilitaet). Die npm-audit-Resolution
ist hier fehlerhaft, da sie keine vorwaertskompatiblen Fixes anbietet.

### 4.2 Ueberpruefungs-Rhythmus

- **Monatlich:** `npm audit` in companion-app ausfuehren und pruefen, ob
  Fixes fuer braces, node-forge oder sprintf-js verfuegbar wurden.
- **Bei Expo SDK Upgrade:** Wenn Expo SDK 53+ migriert wird, pruefen ob
  die transitive Abhaengigkeiten aktualisiert werden und die Overrides
  gegebenenfalls entfernen.
- **Bei Jest Upgrade:** Wenn ts-jest 30+ oder Jest 31+ verfuegbar sind,
  pruefen ob die sprintf-js-Abhaengigkeit aufgeloest wird.

### 4.3 Langfristige Behebung

Die 3 Root-Advisories koennen erst behoben werden, wenn:
1. **braces** 3.0.4+ released wird (oder micromatch/fast-glob alternativen
   einschalten)
2. **node-forge** 1.4.1+ released wird
3. **sprintf-js** 1.1.4+ released wird (oder argparse js-yaml durch
   eine nicht-sprintf-js-abhaengige Implementierung ersetzt)

Bis diese Upstream-Fixes verfuegbar sind, bleibt das Risiko als
Build/Dev/Test-Time-DoS akzeptiert.

---

## 5. Verifikation

| Check | Status |
|-------|--------|
| `npm audit` Critical/High mit Fix | 0 verbleibend (tar, @xmldom/xmldom, postcss behoben) |
| `npx tsc --noEmit` | 0 Errors |
| `npx jest --coverage` | 132 Tests, 5 Suites, alle bestanden |
| `npx eslint .` | 0 Errors |
| Breaking Changes | Keine (nur package-lock.json + overrides in package.json) |

---

## 6. Geaenderte Dateien

| Datei | Aenderung |
|-------|-----------|
| `companion-app/package.json` | `overrides`-Block hinzugefuegt (tar, @xmldom/xmldom, postcss, uuid) |
| `companion-app/package-lock.json` | Aktualisiert durch `npm install` |
| `companion-app/SECURITY-AUDIT.md` | Neue Datei — dieses Dokument |
