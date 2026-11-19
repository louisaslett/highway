# Check results

Results gathered before submission (paths to the local machine replaced by
`<repo>`, `<home>` or `<tmp>`). CI runs are linked from `SUMMARY.md`.

| File | What |
|------|------|
| `SUMMARY.md` | Final check matrix, target coverage, sizes, install times |
| `local-macos-arm64-R4.6.2.txt` | `R CMD check --as-cran` with `_R_CHECK_CRAN_INCOMING_REMOTE_=true` |
| `local-strict-flags-macos-arm64.txt` | Build with `-Wall -Wextra -pedantic -Werror` via `R_MAKEVARS_USER` |
| `local-spelling-urls.txt` | `spelling::spell_check_package()`, `urlchecker::url_check()` |
| `local-sanitizers-macos-arm64.txt` | `dev/sanitizers/run.sh` (TSan, ASan+UBSan) |
| `local-consumers-macos-arm64.txt` | `dev/consumer-tests/run-all.sh` |
| `win-builder-*.txt` | win-builder `00check.log` for R-devel, R-release and R-oldrelease |
