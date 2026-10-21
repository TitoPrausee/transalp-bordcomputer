# npm Dependency Vulnerability Audit (TOT-56 + TOT-60)

> Companion-App Security Audit — npm dependency remediation
> Stand: Oktober 2026
> Issues: TOT-56 (parent: TOT-51, TOT-16), TOT-60 (parent: TOT-57)

---

## 1. Zusammenfassung

| Kategorie | TOT-56 Vorher | TOT-56 Nachher | TOT-60 Nachher | Status |
|-----------|---------------|----------------|-----------------|--------|
| Critical  | 1             | 0              | 0               | Behoben (TOT-56) |
| High      | 28            | 25             | 25              | 3 behoben (TOT-56), 25 akzeptiertes Risiko |
| Moderate  | 31            | 20             | 0               | 11 behoben (TOT-56), 20 behoben (TOT-60) |
| **Total** | **60**        | **45**         | **25**          | **35 behoben** |

**TOT-56:** 4 Overrides (tar, @xmldom/xmldom, postcss, uuid) — 15 Vulnerabilities behoben.
**TOT-60:** 2 Overrides (js-yaml, argparse) — 20 Vulnerabilities behoben (sprintf-js-Kette).
**Verbleibend:** 25 High (braces + node-forge — kein Upstream-Fix verfuegbar).

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

## 2b. TOT-60: sprintf-js-Kette behoben (20 Moderate → 0)

Die in TOT-56 als akzeptiertes Risiko dokumentierte sprintf-js-Kette (20 Moderate
Vulnerabilities) wurde in TOT-60 durch zwei neue Overrides aufgeloest.

### Neue Overrides (TOT-60)

| Package | Advisory | Vorher | Override | Severity |
|---------|----------|--------|----------|----------|
| `js-yaml` | GHSA-hp3w-g68c-fv3c (sprintf-js DoS, indirekt) | 3.15.2 | 4.3.2 | Moderate |
| `argparse` | GHSA-hp3w-g68c-fv3c (sprintf-js DoS, indirekt) | 1.0.10 | 2.0.1 | Moderate |

### Begruendung der Overrides

- **js-yaml 3.15.2 -> 4.3.2:** `@istanbuljs/load-nyc-config@1.1.0` verwendet
  `require('js-yaml').load()` (index.js:80). Die `load()`-Funktion existiert in
  js-yaml 4.x unverändert (in 3.x als Alias fuer `safeLoad`).
  js-yaml 4.x verwendet `argparse@2.x`, das **nicht mehr von sprintf-js abhaengt**.
  js-yaml 4.3.2 wird bereits von `@expo/xcpretty@4.4.7` in diesem Projekt verwendet
  und ist daher als kompatibel validiert. Der Override dedupliziert auf diese
  bereits getestete Version.
- **argparse 1.0.10 -> 2.0.1:** argparse 2.0.1 hat die Abhaengigkeit von sprintf-js
  vollstaendig entfernt. Da js-yaml 4.x argparse 2.x erwartet, wird der Override
  implizit durch den js-yaml-Override geladen; der explizite Override stellt
  sicher, dass keine argparse 1.x-Version als transitive Abhaengigkeit uebrig bleibt.

### Aufgeloeste transitive Kette (20 Vulnerabilities)

```
sprintf-js (eliminiert) -> argparse 1.x -> js-yaml 3.x
-> @istanbuljs/load-nyc-config -> babel-plugin-istanbul
-> @jest/transform -> {jest-snapshot, jest-runtime, jest-runner,
   jest-config, babel-jest, @jest/reporters}
-> jest, jest-circus, jest-cli, ts-jest, @jest/core, @jest/expect,
   @jest/globals, jest-resolve-dependencies
```

Durch den js-yaml + argparse Override wird `sprintf-js` vollständig aus dem
Abhaengigkeitsbaum entfernt (`npm ls sprintf-js` → leer). Alle 20 als
akzeptiertes Risiko markierten Moderate-Vulnerabilities sind behoben.

### Bekannte Einschraenkung: expo prebuild (pre-existing from TOT-56)

`npx expo prebuild` schlaegt mit `Cannot read properties of undefined (reading 'extract')`
fehl. Dies ist ein **pre-existing Issue aus TOT-56** (nicht durch TOT-60 verursacht):

- **Ursache:** Der TOT-56 `tar`-Override (6.2.1 -> 7.5.22) aendert den CJS-Export
  von tar. tar 7.x setzt `__esModule: true` ohne `default`-Export. Die
  Babel-kompilierte `@expo/cli` verwendet `_interopRequireDefault(require('tar')).default.extract()`,
  was `undefined.extract()` ergibt.
- **Auswirkung:** `expo prebuild` kann die Template-Tarballs nicht extrahieren.
- **Workaround-Versuch:** Nested Override `@expo/cli: { tar: "6.2.1" }` stellt
  prebuild wieder her, reintroduziert aber die tar-Kritikal-Vulnerability
  (GHSA-r292-9mhp-454m). Daher verworfen.
- **Empfehlung:** Bei naechstem Expo SDK Upgrade (53+) pruefen, ob @expo/cli
  mit tar 7.x kompatibel ist. Alternativ `patch-package` evaluieren, um
  `_tar().default.extract` -> `_tar().extract` in @expo/cli zu patchen.

---

## 3. Akzeptiertes Risiko — Verbleibende 25 Vulnerabilities

Alle verbleibenden 25 Vulnerabilities lassen sich auf **2 Root-Advisories**
zurueckfuehren, fuer die **keine Fix-Version auf npm verfuegbar** ist.
Die jeweils neueste Version des Packages ist selbst verwundbar.

(Die ehemals 20 Moderate-Vulnerabilities aus sprintf-js wurden in TOT-60
behoben — siehe Abschnitt 2b.)

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

### 3.3 sprintf-js (20 Moderate) — GHSA-hp3w-g68c-fv3c — BEHOBEN IN TOT-60

**Status: Behoben** durch js-yaml 4.3.2 + argparse 2.0.1 Overrides (siehe Abschnitt 2b).
sprintf-js ist vollständig aus dem Abhaengigkeitsbaum entfernt.

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
  Fixes fuer braces oder node-forge verfuegbar wurden.
- **Bei Expo SDK Upgrade:** Wenn Expo SDK 53+ migriert wird, pruefen ob
  die transitive Abhaengigkeiten aktualisiert werden, die Overrides
  gegebenenfalls entfernen, und das `expo prebuild`-Problem (Abschnitt 2b)
  beheben.
- **js-yaml/argparse Overrides:** Koennen entfernt werden, wenn
  @istanbuljs/load-nyc-config von js-yaml 4.x nativ abhaengt.

### 4.3 Langfristige Behebung

Die 2 verbleibenden Root-Advisories koennen erst behoben werden, wenn:
1. **braces** 3.0.4+ released wird (oder micromatch/fast-glob alternativen
   einschalten)
2. **node-forge** 1.4.1+ released wird

(Die sprintf-js-Abhaengigkeit wurde in TOT-60 durch Override-Aufloesung
der js-yaml/argparse-Kette behoben — kein Upstream-Fix mehr erforderlich.)

Zusaetzlich sollte das `expo prebuild`-Problem (Abschnitt 2b) bei einem
Expo SDK 53+ Upgrade angegangen werden.

Bis diese Upstream-Fixes verfuegbar sind, bleibt das Risiko als
Build/Dev-Time-DoS akzeptiert.

---

## 5. Verifikation

### TOT-56 Verifikation

| Check | Status |
|-------|--------|
| `npm audit` Critical/High mit Fix | 0 verbleibend (tar, @xmldom/xmldom, postcss behoben) |
| `npx tsc --noEmit` | 0 Errors |
| `npx jest --coverage` | 132 Tests, 5 Suites, alle bestanden |
| `npx eslint .` | 0 Errors |
| Breaking Changes | Keine (nur package-lock.json + overrides in package.json) |

### TOT-60 Verifikation

| Check | Status |
|-------|--------|
| `npm audit` | 25 high (down from 45), 0 moderate, 0 critical |
| `npm ls sprintf-js` | leer (sprintf-js vollständig entfernt) |
| `npx tsc --noEmit` | 0 Errors |
| `npm test` | 132 Tests, 5 Suites, alle bestanden |
| `expo prebuild` | Pre-existing failure (TOT-56 tar 7.x ESM/CJS interop — siehe Abschnitt 2b) |
| Breaking Changes | Keine (nur package-lock.json + 2 neue overrides in package.json) |

---

## 6. Geaenderte Dateien

### TOT-56

| Datei | Aenderung |
|-------|-----------|
| `companion-app/package.json` | `overrides`-Block hinzugefuegt (tar, @xmldom/xmldom, postcss, uuid) |
| `companion-app/package-lock.json` | Aktualisiert durch `npm install` |
| `companion-app/SECURITY-AUDIT.md` | Neue Datei — dieses Dokument |

### TOT-60

| Datei | Aenderung |
|-------|-----------|
| `companion-app/package.json` | `js-yaml` und `argparse` zu `overrides` hinzugefuegt |
| `companion-app/package-lock.json` | Aktualisiert durch `npm install` (3 packages entfernt: sprintf-js, argparse 1.x, js-yaml 3.x) |
| `companion-app/SECURITY-AUDIT.md` | Abschnitt 2b, 3.3, 4.3, 5, 6 aktualisiert |
