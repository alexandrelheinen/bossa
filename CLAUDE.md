# Claude instructions

@.guidelines/workflow/sdd.md
@.guidelines/workflow/integration.md
@.guidelines/workflow/tdd.md
@.guidelines/agents/writing.md
@.guidelines/style/naming.md
@.guidelines/languages/cpp.md
@.guidelines/languages/cmake.md
@.guidelines/languages/sh.md

For BOSSA's own project context, embedded/hardware policy, and merge
policy, read [CONTRIBUTING.md](CONTRIBUTING.md). For BOSSA-specific coding
notes, read [docs/guidelines.md](docs/guidelines.md).

Before every push on a PR branch, run at minimum:

```bash
bash scripts/check/formatting.sh
./scripts/build.sh
```
