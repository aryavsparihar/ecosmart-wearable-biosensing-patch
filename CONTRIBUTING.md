# Contributing

Thanks for your interest! This is an educational research prototype, so
contributions that improve correctness, clarity, testing or documentation are
especially welcome.

## Ground rules

- **No medical claims.** Don't add wording, features or outputs that suggest
  diagnosis, treatment or clinical validity. Anomaly flags are relative to a
  personal baseline, nothing more.
- **Label synthetic data as synthetic**, everywhere it appears.
- **Never commit personal or health data.** If you collect real recordings for
  your own experiments, keep them out of the repository.

## Development setup

```bash
python3 -m venv .venv && source .venv/bin/activate
pip install platformio==6.2.0 -r ml/requirements.txt
pio run                 # build all AVR environments
pio test -e native      # firmware unit tests on the host
pytest                  # Python tests
```

No Arduino is required. Use `nanoatmega328_sim`, the Wokwi circuit or simavr
(see the README).

## Rules that keep the project honest

1. **Firmware ↔ Python parity.** `src/` and `ml/ecosmart_ml/` implement the
   same integer maths. If you change features, the baseline, rounding, the
   guard or the alert logic, change both sides, then run:
   ```bash
   python ml/generate_data.py && python ml/train.py
   python ml/export_parity_vectors.py
   pio test -e native && pytest
   ```
   Commit the regenerated `include/model_weights.h`, `ml/model/model.json`,
   `test/test_parity/parity_vectors.h` and `docs/results/metrics.json`. CI
   rejects stale artefacts.
2. **Never edit generated files by hand** (`model_weights.h`,
   `parity_vectors.h`).
3. **Memory discipline on AVR.** Static allocation only: no `String`, `new` or
   `malloc`. Wrap literals in `F()` or `PROGMEM`. Remember that `int` is 16-bit
   and widen explicitly. Report RAM and flash changes in your PR (CI posts
   them in the job summary).
4. **Portable core.** Hardware access belongs in `src/hal/` only. Everything
   else must build in the `native` environment.
5. **Constants live in `include/config.h`**, mirrored in
   `ml/ecosmart_ml/config.py`. `test_config.py` checks that they agree.

## Style

- C++11 (avr-gcc 7.3), Google-ish style: `kConstants`, `camelCase` functions,
  trailing-underscore members, 2-space indent, 100-column lines.
- Python 3.11+, PEP 8, type hints where they help.
- Comments explain *why*. Keep them to the density of the surrounding code.

## Commits and pull requests

- Use [Conventional Commits](https://www.conventionalcommits.org/): `feat:`,
  `fix:`, `test:`, `docs:`, `ci:`, `build:`, `refactor:`, `perf:`, `chore:`,
  optionally scoped, e.g. `feat(firmware): ...` or `fix(ml): ...`.
- Keep commits small, and make each one build and pass tests on its own.
- Open a PR against `main`. All CI jobs must be green.
- Add an entry under *Unreleased* in [CHANGELOG.md](CHANGELOG.md) for
  user-visible changes.
