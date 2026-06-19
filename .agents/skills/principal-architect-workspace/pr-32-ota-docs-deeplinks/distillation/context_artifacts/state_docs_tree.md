# Final Documentation Tree State

## `docs/` Root

```
docs/
├── README.md                     ← 2-section index (Developer Guides + Design Spec)
├── design_specification.md       ← Architectural blueprint (2823 lines, 38 ADRs, 64 deep links)
├── archive/
│   ├── comparative_analysis_report.md
│   ├── embedded-best-practices-assessment.md
│   └── security-evaluation-report.md
└── developer/
    ├── README.md                 ← 15-guide reading order
    ├── architecture-overview.md
    ├── build-system.md
    ├── coding-patterns.md
    ├── config-system.md
    ├── creating-a-module.md
    ├── embedded-safety.md
    ├── event-bus.md
    ├── extending-the-web-portal.md
    ├── mcp-server.md
    ├── ota-update-system.md      ← (migrated from docs/ root)
    ├── performance-guide.md
    ├── scheduling-data-fetches.md
    ├── security.md
    ├── using-optional-modules.md
    └── versioning-policy.md
```

## Skill OTA System Tree

```
.agents/skills/framework-expert/systems/ota/
├── overview.md          ← State machine, pipeline, release channels
├── config-lifecycle.md  ← Active/Previous/Golden snapshots, migration, rollback
└── deployment.md        ← REST API, deploy_release.py, manifests, error codes
```

## Design Spec Deep Link Coverage

- 17 section-level `📖 **Developer Guide**` callout boxes (§2–§16)
- 47 subsection-level `📖 See` inline links (§2.1–§16.3)
- 64 total navigable links into 15 developer guides
- Skipped: §1 (context), §11A (no guide yet), §17 (decision log)
