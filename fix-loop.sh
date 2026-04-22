#!/usr/bin/env bash
# fix-loop.sh — Call Claude Code in a loop until all tree-sitter tests are green.
#
# Usage:
#   ./fix-loop.sh              # runs until 0 failures or user interrupt
#   ./fix-loop.sh --dry-run    # show the current failure count and exit
#
# Requirements: claude CLI available on PATH, tree-sitter CLI available on PATH.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

DRY_RUN=false
[[ "${1:-}" == "--dry-run" ]] && DRY_RUN=true

# ── helpers ──────────────────────────────────────────────────────────────────

count_failures() {
  tree-sitter test --overview-only 2>&1 | grep -c $'✗' || true
}

count_passing() {
  tree-sitter test --overview-only 2>&1 | grep -c $'✓' || true
}

print_status() {
  local fail pass total
  fail=$(count_failures)
  pass=$(count_passing)
  total=$((fail + pass))
  echo "  Failures : $fail"
  echo "  Passing  : $pass"
  echo "  Total    : $total"
}

banner() {
  echo ""
  echo "════════════════════════════════════════════════════════"
  echo "  $*"
  echo "════════════════════════════════════════════════════════"
}

# ── main ─────────────────────────────────────────────────────────────────────

banner "Tree-Sitter Marko — fix loop"
print_status

if $DRY_RUN; then
  echo ""
  echo "Dry run — exiting."
  exit 0
fi

ITERATION=0
MAX_ITERATIONS=200   # safety cap — remove if you trust the loop

CLAUDE_PROMPT='We are fixing failing tree-sitter tests for the marko-tmbundle grammar.
Working directory: tree-sitter-marko/ (you are already inside it).

Reference files:
- test/failing_tests_tracking.md — 110 failing tests grouped by root cause, fix in priority order.
- ../marko-prettier/src/index.ts — critical design reference (see below).

Rules (non-negotiable):
- Run `tree-sitter test --overview-only` first to confirm the current state.
- Fix grammar.js (or C scanner) so the parser produces the correct tree.
- NEVER run `tree-sitter test --update` to auto-accept wrong output.
- Only update a test expectation manually when the expected output is wrong or inconsistent with
  the htmljs-parser reference. To verify, check the matching fixture in
  ../htmljs-parser/src/__tests__/fixtures/<fixture-name>/__snapshots__/<fixture-name>.expected.txt
  and translate its event-based output into the correct tree-sitter s-expression. Update the
  corpus file directly, then fix the grammar to match the corrected expectation.
- Update test/failing_tests_tracking.md (flip FAIL → PASS for fixed tests).
- Commit after each fixed test or coherent group: `git add -p && git commit -m "..."`.
- ONLY WORK ON A SINGLE TEST CASE at time, after it your job is done!

Workflow for each fix:
1. `tree-sitter test -i "<test name regex>"` — see expected vs actual diff.
2. Read the corpus entry in test/corpus/<file>.txt.
3. Read grammar.js and understand the rule(s) involved.
4. Consult ../marko-prettier/src/index.ts for how the same construct is handled there.
5. Make correct grammar change (refactor if the current approach is fundamentally broken).
6. `tree-sitter generate && tree-sitter test -i "<test name>"` — verify fix.
7. `tree-sitter test --overview-only` — confirm no regressions.
8. Append your progress to the test/failing_tests_tracking.md file. Use this to leave a note for the next person working in the codebase.
9. Commit, update tracking file and you are DONE for the day!'

while true; do
  ITERATION=$((ITERATION + 1))

  fail=$(count_failures)

  if [[ "$fail" -eq 0 ]]; then
    banner "ALL TESTS GREEN after $ITERATION iteration(s)!"
    echo "  Nothing left to fix."
    exit 0
  fi

  if [[ "$ITERATION" -gt "$MAX_ITERATIONS" ]]; then
    banner "Safety cap reached ($MAX_ITERATIONS iterations). Stopping."
    print_status
    exit 1
  fi

  banner "Iteration $ITERATION — $fail test(s) still failing"
  echo ""

  # Run Claude non-interactively with the fix prompt.
  # --print means one-shot (no interactive REPL), --yes auto-approves safe tool calls.
  claude \
    --dangerously-skip-permissions \
    --permission-mode acceptEdits \
    --print \
    "$CLAUDE_PROMPT"

  echo ""
  banner "Iteration $ITERATION complete — re-counting…"
  print_status
  echo ""

  # Brief pause so git/FS settle before next run.
  sleep 2
done
