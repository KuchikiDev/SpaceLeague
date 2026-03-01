# Phase 0 Test Plan (UE5 Pragmatique)

Reference principale:

- `docs/qa/assessments/0.0-test-design-20260219.md`

Execution minimale recommandee:

1. Stabiliser Data/Blueprint critiques (P0 INT 001-003)
2. Valider build et lancement smoke (P0 INT 004/006 + E2E 001)
3. Valider gameplay de base (P0 E2E 003/004/005/006)
4. Clore avec QA gate et rapport

Commandes associees:

- `/ue5-health-check --report`
- `/ue5-compile` (ou build local `-NoXGE`)
- `/ue5-test` (si scénarios automatisés ajoutés)
- `/qa-gate`
