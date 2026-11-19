# Prompt for the submission session

Paste everything below the line into a new session started in this repository
(`/Users/louis/Documents/research/Programming/highway`).

---

You are picking up the R package **highway** (repository
`louisaslett/highway`, package root = repository root), which bundles Google
Highway 1.4.0 C++ headers for use through `LinkingTo`. Development and
pre-submission checking are finished; I have now reviewed it. Your job is to
apply my review changes (if I give any), re-verify, and take it through CRAN
submission with me.

**Read first** (they are the source of truth; do not rely on memory):
`dev/DESIGN.md` (design, decisions, section 15 "Findings"),
`dev/check-results/SUMMARY.md` (check matrix, target coverage, sizes),
`cran-comments.md`, `inst/COPYRIGHTS`, `NEWS.md`, `DESCRIPTION`. The bundled
`inst/include/hwy/` is generated: never edit it by hand. Change it only by
adding or editing a patch in `tools/patches/` and running
`tools/update_highway.sh` (then `tools/update_highway.sh --check`); list every
patch in `inst/COPYRIGHTS` section 4.2 and `NEWS.md`.

**Ground rules**
- Commit only under the git identity already configured; never change
  `user.name`/`user.email`, never use `--author`. No co-author, "Generated
  with" or other AI/tool attribution in commits, tags, release notes or files.
- Do not modify my global configuration (`~/.R/Makevars`, `~/.Rprofile`,
  `~/.gitconfig`, shell profiles). For strict flags use
  `R_MAKEVARS_USER=dev/Makevars.strict` (git-ignored; recreate it if missing:
  `CXXFLAGS = -g -O2 -Wall -Wextra -pedantic -Werror`).
- If a `gh`/`git push` operation fails for authentication or token-scope
  reasons, stop and tell me the fix; do not work around it.
- Ask me before anything irreversible. **Never submit to CRAN without my
  explicit "yes, submit" in this session.**

**Steps**
1. Apply my review changes, if any, keeping `dev/DESIGN.md`, `NEWS.md`,
   `inst/COPYRIGHTS` and `cran-comments.md` consistent with them. Bump
   nothing unless I ask (the version stays `1.4.0-1` until first acceptance).
2. Local checks: `roxygen2::roxygenise()` if R docs changed; `R CMD build .`;
   `_R_CHECK_CRAN_INCOMING_REMOTE_=true R CMD check --as-cran` on the tarball
   (expect exactly one NOTE, "New submission"); the strict-flags build;
   `spelling::spell_check_package()` and `urlchecker::url_check()`. Replace
   the files in `dev/check-results/` with the new results, with machine paths
   replaced by `<repo>`, `<home>`, `<tmp>`.
3. Push to `main` and wait for the GitHub Actions workflows to finish
   (`R-CMD-check`, `consumers`, `sanitizers`, `other-arches`, `intel-sde`,
   `webr`; `header-warnings` also runs if `inst/include/` or `tools/`
   changed; it takes about an hour). All must be green. Do not poll in short
   loops: one background wait per batch, and stop it when done.
4. Re-run R-hub: `gh workflow run rhub.yaml -f config="nosuggests,gcc-asan,clang-asan,clang-ubsan,valgrind,gcc16,clang23,m1-san,lto,donttest" -f name="final <sha>"`.
   Every job's log must show `Status: OK`.
5. win-builder: `devtools::check_win_devel()` (and `check_win_release()`,
   `check_win_oldrelease()` if the code changed). Results are emailed to me;
   ask me to paste the result links, then fetch each `00check.log` and save it
   to `dev/check-results/win-builder-*.txt`. Known: the previous run's NOTE
   also flagged "R's" in DESCRIPTION; that wording has since been removed, so
   the only NOTE should now be "New submission".
6. mac-builder: `devtools::check_mac_release()`. It returned HTTP 502 on
   2026-11-18 and 2026-11-19; retry, and if it is still down, record that in
   `SUMMARY.md` and `cran-comments.md` rather than waiting indefinitely.
7. Finalise `cran-comments.md` (test environments and results, exactly as
   observed) and `dev/check-results/SUMMARY.md`; commit and push.
8. Show me a short readiness report (check matrix, anything that changed,
   remaining risks; in particular the Linux installed size is 4.6–4.7 MB with
   `-g` against CRAN's 5 MB NOTE threshold) and ask whether to submit.
9. Only after my explicit yes: run `devtools::submit_cran()` from the package
   root (it uploads the tarball and CRAN emails me a confirmation link that I
   must click). Do not commit the `CRAN-SUBMISSION` file it creates (it is
   build-ignored; leave it untracked unless I say otherwise).
10. After CRAN accepts (I will tell you): in `README.md`, link the CRAN badge
    to `https://CRAN.R-project.org/package=highway` (see the comment there),
    tag the accepted commit `v1.4.0-1`, and push the tag; ask me before
    creating a GitHub release.
