# Cursive Language Specification

## Clause 1 — Introduction and Conformance

**Part**: I — Introduction and Conformance
**File**: 01-6_Versioning-Evolution.md  
**Section**: §1.7 Versioning and Evolution  
**Stable label**: [intro.versioning]  
**Forward references**: §1.7.4 [intro.versioning.deprecation], §1.7.5 [intro.versioning.patch], Annex H [changes], Clause 10 [generic], Clause 12 [contract]

---

### §1.7 Versioning and Evolution [intro.versioning]

[1] Cursive follows semantic versioning (`MAJOR.MINOR.PATCH`). This subclause describes the meaning of each component and the obligations imposed on implementations and programs as the language evolves.

#### §1.7.1 Version Identifiers [intro.versioning.identifiers]

[2] The specification defined herein corresponds to version **1.0.0**. Minor revisions (`1.x.0`) add backwards-compatible features; patch revisions (`1.0.x`) provide errata and clarifications without changing behavior. Major revisions (`2.0.0`, `3.0.0`, …) may introduce breaking changes but shall follow the migration process in §1.7.4 [intro.versioning.deprecation].

[3] Projects shall declare a minimum required language version in their manifest or build configuration. If no version is declared, implementations may default to the latest supported minor release within the highest major version they provide (for example, 1.0 defaults to 1.latest within the 1.x series).

#### §1.7.2 Implementation Support [intro.versioning.impl]

[4] Conforming implementations shall document the set of language versions they support, including the default compilation version. When multiple versions are supported simultaneously, the implementation shall expose a mechanism (flag, configuration value, or project manifest entry) that selects the desired version.

[5] Preview or experimental versions may be offered, but they shall require explicit opt-in (e.g., `--language-version 2.0-preview`). Preview behavior shall never silently override stable semantics.

#### §1.7.3 Feature Stability Classes [intro.versioning.features]

[6] Features fall into three stability classes:

- **Stable**: Available by default in all minor releases of the current major version; breaking changes require a major release.
- **Preview**: Opt-in features that are intended for the next major release. Preview features shall be tagged `[Preview: target_version]` in the specification.
- **Experimental**: Highly unstable features guarded by feature-control directives. Experimental features may change or disappear without notice and shall never be enabled implicitly.

[7] Implementations shall clearly report when preview or experimental features are enabled. Tooling should emit warnings reminding users of the stability level.

#### §1.7.4 Deprecation and Removal [intro.versioning.deprecation]

[8] A feature slated for removal shall first be marked **deprecated** in a minor release. Deprecation notes include the planned removal version, rationale, and migration guidance. Deprecation lasts for at least one minor release before removal.

[9] Removal occurs only in a major release. Implementations shall issue diagnostics in the new major version when code relies on removed features and should provide migration hints. Automated migration tooling is a recommended, but not mandatory, quality-of-implementation feature.

[10] Deprecated features remain fully functional until removal but may trigger warnings. Programs may suppress such warnings only through documented means that do not mask other diagnostics.

#### §1.7.5 Errata and Patch Releases [intro.versioning.patch]

[11] Patch releases address specification defects, typographical errors, or clarifications that do not change behavior. When errata alter normative text, the change log in Annex H [changes] shall record the revision, rationale, and impacted sections.

[12] Implementations are encouraged to adopt patch releases promptly. When behavior diverges from a published patch release, implementations shall document the discrepancy and, if possible, provide compatibility modes.

#### §1.7.6 Project Manifest Schema [intro.versioning.manifest]

[13] Every project shall provide a UTF-8 encoded manifest file named `Cursive.toml` at the workspace root. The manifest shall conform to TOML 1.0 (ISO/IEC - ISO/IEC DIS 30170:2022) so that tooling can parse it deterministically.

[14] The manifest shall contain a `[cursive.language]` table with a `version = "MAJOR.MINOR.PATCH"` entry that names the language version targeted by the project. The version string shall follow the semantic-versioning grammar defined in §1.7.1 and shall be present even when the implementation also accepts command-line overrides.

[15] The manifest shall declare at least one source root using the `[cursive.source]` table with a `roots = ["relative/path", ...]` array. Each entry shall be a POSIX-style relative path rooted at the manifest directory, shall resolve to an existing directory, and shall be unique after path normalisation. Clause 4 derives module paths exclusively from these roots.

[16] Implementations shall issue diagnostics when the manifest is missing, malformed, omits the `language.version`, or provides an empty `roots` array. Implementations may extend the manifest with additional tables provided that the `cursive.language` and `cursive.source` tables remain intact and their semantics are not altered.

---

**Previous**: §1.6 Document Conventions (§1.6 [intro.document]) | **Next**: §1.8 Design Decisions and Absent Features (§1.8 [intro.absent])
