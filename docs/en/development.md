# Development guide

[简体中文](../zh-CN/development.md) · [Documentation](index.md)

Use the [getting-started environment](getting-started.md) first. Firmware uses C++17 and the
unchanged PlatformIO/Arduino package pins. Windows host tests need Visual Studio C++ Build Tools;
POSIX host tests use g++ or the executable named by CXX. Quality checks additionally need Node.js 22.

<!-- section: setup -->
## Install check tools

```powershell
.\.venv\Scripts\python -m pip install -r requirements-dev.txt
npm ci --ignore-scripts
```

These are development dependencies, not a frontend runtime. The shipped page uses no npm package
loader or bundler; the single vendored MD5 implementation retains its own MIT notice.
Only artwork regeneration needs Pillow; ordinary build/check steps do not regenerate PNGs.

<!-- section: build -->
## Generate and build

```powershell
.\.venv\Scripts\python scripts/generate_replay_frames.py
.\.venv\Scripts\python scripts/embed_web.py
.\.venv\Scripts\python -m platformio run -e esp32dev -e phase2 -e phase3 -e phase4
```

Wall/Wextra remain enabled; project source warnings are errors. Installed framework packages are
not edited. The PowerShell helper `scripts/pio.ps1` selects the local environment/cache and can be
called from another directory. It does not require rtk. On POSIX use `.venv/bin/python` instead
of the Windows interpreter path.

Generated files are `include/remote_frames.h` and `include/web_assets.h`.
Their generators validate source CSV indices/values and calculate exact CSP hashes.
Don't format these generated headers manually.

<!-- section: checks -->
## Run local software checks

```powershell
.\.venv\Scripts\python scripts/test_host.py
.\.venv\Scripts\python tests/test_analysis.py
node --check web/app.js
node --check web/auth.js
node tests/test_web_auth.js
.\.venv\Scripts\python scripts/check_repo.py
npm run lint:markdown
git diff --check
```

The C++ suite tests identity, RF math, replay bounds and web/auth policies without I/O.
Python analysis uses retained real samples; the JavaScript vectors use the actual client code.
None of these commands accesses serial, a gateway or RF. `check_repo.py` checks internal links,
language filename/section/command coverage, source formatting/syntax, pins, JSON/YAML, API route
agreement and generated resources. It is not a browser renderer, translator or full security audit.

For first-party C++ changes, format with the pinned clang-format installed beside the virtual
environment's Python. Use `-i` for only the edited C++ files. Python uses four spaces; C++/web
uses two. Existing compact web code is retained instead of introducing a frontend formatter migration.

To validate workflow syntax with Go installed:

```sh
go run github.com/rhysd/actionlint/cmd/actionlint@v1.7.7 .github/workflows/ci.yml
```

<!-- section: ci -->
## CI coverage

[ci.yml](../../.github/workflows/ci.yml) has two Ubuntu 24.04 jobs: all four firmware builds, and
offline quality/tests. Actions are pinned by commit; permissions are contents:read, checkout
credentials are not persisted, jobs have timeouts, and there is no publish/deploy step.
Python 3.11 and Node 22 select compatible maintenance patches; firmware package versions and npm
dependencies remain pinned. No hardware, credentials or RF response is simulated.

GitHub-hosted CI has not run in this local checkout. Local Windows build/test success and
actionlint/YAML checks do not establish success on an Ubuntu runner. Do not add a green badge
until the real remote workflow has passed. No firmware binary artifact is distributed by CI.

<!-- section: changes -->
## Keep changes reviewable

Use [CONTRIBUTING](../../CONTRIBUTING.md). Preserve RF parameters until new data justifies a
separate change. Keep meaningful first-party comments bilingual and public API contracts accurate.
Update both language files and matching section markers; manually check meaning after structural
checks pass. Preserve machine JSON/serial keys and original operator quotations.

The cleanup's authentication lifetime fix is separate from formatting/documentation:
64-bit RAM expiry and Digest epoch binding prevent old credentials reviving after clock wrap.
It adds ten pure boundary checks and does not change RF behavior; it still needs current-firmware
real-device login/reboot validation before a firmware release.
