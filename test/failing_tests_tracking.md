# Tree-Sitter Marko — Failing Tests Tracker

**Goal:** Get all 345 tests green by fixing the grammar/parser — never by downgrading expected output.

**Rules:**
1. Fix the grammar/parser; only update a test expectation when a node name is renamed or a new node is introduced upstream.
2. Commit after every test (or coherent group of tests) fixed.
3. Update this file — flip `FAIL` → `PASS` — after each commit.
4. Pick the next test that is the most architecturally important (fixes root causes first, not leaf symptoms).
5. Never use `--update` to auto-accept wrong output.

**Status as of 2026-04-09:** 103 failing / 242 passing / 345 total

---

## Key reference: `../marko-prettier/src/index.ts`

Before writing any grammar fix, check how **marko-prettier** handles the same construct.
It uses a well-proven strategy: wrap a Marko sub-expression in JavaScript/TypeScript
scaffolding, send it to the Babel/TS parser, then strip the scaffolding back out.
The same insight applies here — instead of re-implementing JS/TS expression parsing
inside the Marko grammar, **delegate those ranges to the JavaScript/TypeScript
tree-sitter parser** via `injections.scm` (language injection) or by shaping the
grammar rule so the external scanner can consume them as opaque JS blobs.

### Pattern catalogue (from marko-prettier → tree-sitter equivalent)

| Construct | marko-prettier wraps as | Tree-sitter approach |
|-----------|------------------------|----------------------|
| Attribute method value `onClick() {}` | `function${value}` → TS expression parser, then strips `function ` | Grammar rule should accept a method body as an opaque JS expression fragment; the external scanner or an injection handles the body |
| Tag variable `/{ name, type }` | `var ${code}=_` → babel-ts statement parser, strips `=_` | `tag_variable` rule should accept a full destructuring pattern; inject JS to parse it |
| Tag type arguments `<T, U>` | `` _<${code}> `` → TS expression parser, strips `_` | Grammar should capture the angle-bracket-balanced range as `tag_type_args`; use external scanner for balanced `<>` |
| Tag params `|x, y|` | `function _(${code}){}` → babel-ts, extracts params group | Grammar captures the pipe-delimited range; injection maps it to a JS params context |
| Tag type params `<T extends X>` | `function _<${code}>(){}` → babel-ts, extracts type-params group | Same as type args, use balanced-`<>` scanner |
| Attribute args `(expr)` | `_(${code})` → TS expression parser, strips `_` | Grammar already has `attribute_expression`; may need external scanner for balanced parens containing generics |
| Attribute value `=expr` | `read(value)` → TS expression parser directly | `attribute_value_fragment` must consume enough JS to handle `typeof x`, `void 0`, `x instanceof Y`, unary `!` etc. |
| Tag type args in `<Tag<T>>` | `` _<${code}> `` | Critical: `<` after a tag name must not be re-interpreted as a new HTML tag open when it starts a type argument list |

### Concrete examples from the source

```typescript
// AttrMethod — lines 364-376 of index.ts
// Prepends "function" so the TS parser sees a named function, then removes it.
const attrMethodDoc = await toDoc(`function${read(node.value, opts)}`, exprParse);
attrMethodDoc[0] = attrMethodDoc[0].replace(/^function\s*/, "");

// TagVar — lines 428-467
// Wraps in a var declaration so the TS parser handles the destructuring pattern.
let doc = await toDoc(`var ${code}=_`, stmtParse);

// TagTypeArgs — lines 470-487
// Wraps in a generic call expression so TS sees it as a type argument list.
const doc = await toDoc(`_<${code}>`, exprParse);
doc[0] = doc[0].replace(/^_/, "");

// TagParams — lines 489-521
// Wraps in a function signature so TS handles union/intersection param types.
const doc = await toDoc(`function _(${code}){}`, stmtParse);

// TagTypeParams — lines 524-538
// Wraps in a generic function signature.
const doc = await toDoc(`function _<${code}>(){}`, stmtParse);

// TagArgs / AttrArgs — lines 921-937
// Wraps in a call expression.
const doc = await toDoc(`_(${code})`, exprParse);
doc[0] = doc[0].replace(/^_/, "");
```

### When this matters for grammar fixes

- **TypeScript generics group (Group 1):** The root issue is `<Tag<T>>` — the second
  `<` looks like a new tag. The fix should make the grammar or external scanner consume
  the full `<T>` type-args span before the HTML parser sees it, mirroring how
  marko-prettier wraps it as `` _<T> ``.

- **Attribute expression group (Group 2–4):** `typeof`, `void`, unary `!`, regex
  literals — these are all valid JS expressions that marko-prettier sends to the TS
  expression parser as-is. The grammar's `attribute_value_fragment` rule (or external
  scanner) needs to handle whatever JS sub-expression can appear after `=` in an
  attribute.

- **Tag variable group (Group 6):** marko-prettier wraps as `var X=_`. The grammar rule
  for `tag_variable` should accept any valid JS destructuring LHS (including typed
  patterns like `{ x }: Type`).

- **Attribute method group (Group 5):** marko-prettier prepends `function`. The grammar
  should already have a rule for method-shorthand attributes; check if the external
  scanner correctly handles the opening `(` of the params.

---

## Fix priority order (suggested — re-evaluate as you go)

Tests are grouped by likely root cause. Fix in this order so early wins unblock later ones.

### Group 1 — TypeScript generics confuse HTML tag parser (`<Tag<T>>` ambiguity)

These all fail because `<` inside a type argument position is interpreted as another HTML tag open.

| # | Test | Status |
|---|------|--------|
| 1 | `Fixture ts-generic-simple (htmljs target)` | FAIL |
| 2 | `Fixture ts-generic-complex (htmljs target)` | FAIL |
| 3 | `Fixture ts-generic-function-type (htmljs target)` | FAIL |
| 4 | `Fixture ts-function-type (htmljs target)` | FAIL |
| 5 | `Fixture ts-intersection-type (htmljs target)` | FAIL |
| 6 | `Fixture ts-nested-generics (htmljs target)` | FAIL |
| 7 | `Fixture ts-type-statement (htmljs target)` | FAIL |
| 8 | `Fixture ts-unary-exression (htmljs target)` | FAIL |
| 9 | `Fixture tag-with-type-arguments (htmljs target)` | FAIL |
| 10 | `Fixture tag-type-argument-arrow-function (htmljs target)` | FAIL |
| 11 | `Fixture tag-params-with-type-parameters (htmljs target)` | FAIL |
| 12 | `Fixture attr-method-with-type-parameters (htmljs target)` | FAIL |
| 13 | `Fixture attr-method-shorthand-with-type-parameters (htmljs target)` | FAIL |
| 14 | `Fixture invalid-multiple-tag-type-params (htmljs target)` | FAIL |
| 15 | `Fixture invalid-type-params-after-args (htmljs target)` | FAIL |
| 16 | `Fixture invalid-type-params-after-method (htmljs target)` | FAIL |
| 17 | `Fixture invalid-type-params-after-params (htmljs target)` | FAIL |
| 18 | `Fixture invalid-type-params-attr-arg (htmljs target)` | FAIL |

### Group 2 — Attribute expressions / operators / whitespace

Attribute value parsing produces `ERROR` nodes around operators, unenclosed whitespace, typeof/void/unary forms.

| # | Test | Status |
|---|------|--------|
| 19 | `Fixture attr-simple-expression (htmljs target)` | PASS |
| 20 | `Fixture attr-complex (htmljs target)` | FAIL |
| 21 | `Fixture attr-complex-functions (htmljs target)` | FAIL |
| 22 | `Fixture attr-complex-instanceof (htmljs target)` | FAIL |
| 23 | `Fixture attr-complex-unary (htmljs target)` | FAIL |
| 24 | `Fixture attr-non-literal (htmljs target)` | FAIL |
| 25 | `Fixture attr-operators-space-after (htmljs target)` | FAIL |
| 26 | `Fixture attr-operators-space-before (htmljs target)` | FAIL |
| 27 | `Fixture attr-operators-space-between (htmljs target)` | FAIL |
| 28 | `Fixture attr-operators-newline-after (htmljs target)` | FAIL |
| 29 | `Fixture attr-operators-newline-before (htmljs target)` | FAIL |
| 30 | `Fixture attr-operator-whitespace-eof (htmljs target)` | FAIL |
| 31 | `Fixture attr-value-typeof (htmljs target)` | FAIL |
| 32 | `Fixture attr-value-typeof-comma (htmljs target)` | FAIL |
| 33 | `Fixture attr-value-typeof-no-space-prefix (htmljs target)` | FAIL |
| 34 | `Fixture attr-value-void (htmljs target)` | FAIL |
| 35 | `Fixture attr-value-single-quote-escaped (htmljs target)` | FAIL |
| 36 | `Fixture attr-inc-and-dec (htmljs target)` | FAIL |
| 37 | `Attribute operator spacing in default values (htmljs target)` | FAIL |

### Group 3 — Unenclosed attribute whitespace

| # | Test | Status |
|---|------|--------|
| 38 | `Fixture attr-unenclosed-whitespace (htmljs target)` | FAIL |
| 39 | `Fixture attr-unenclosed-whitespace-minus (htmljs target)` | FAIL |
| 40 | `Fixture attr-unenclosed-whitespace-multiple-attrs (htmljs target)` | FAIL |
| 41 | `Fixture attr-unenclosed-whitespace-odd (htmljs target)` | PASS |
| 42 | `Fixture attr-without-delimiters (htmljs target)` | PASS |

### Group 4 — Attribute regex / regex character classes

| # | Test | Status |
|---|------|--------|
| 43 | `Fixture attr-regexp (htmljs target)` | FAIL |
| 44 | `Fixture attr-regex-character-classes (htmljs target)` | FAIL |
| 45 | `Attribute regex literal forms (htmljs target)` | FAIL |

### Group 5 — Attribute method shorthand / concise hyphens / name-with-html-chars

| # | Test | Status |
|---|------|--------|
| 46 | `Fixture attr-method-shorthand (htmljs target)` | FAIL |
| 47 | `Fixture attr-concise-hyphens (htmljs target)` | FAIL |
| 48 | `Fixture attr-name-with-html-chars (htmljs target)` | FAIL |
| 49 | `Fixture attr-multi-line-comment (htmljs target)` | FAIL |

### Group 6 — Tag variable / var declaration

| # | Test | Status |
|---|------|--------|
| 50 | `Fixture var (htmljs target)` | FAIL |
| 51 | `Fixture var-new-Date (htmljs target)` | FAIL |
| 52 | `Fixture tag-var-declaration (htmljs target)` | FAIL |
| 53 | `Fixture tag-var-before-concise-text (htmljs target)` | FAIL |
| 54 | `Fixture tag-var-type-with-parens (htmljs target)` | FAIL |
| 55 | `Fixture invalid-missing-tag-var (htmljs target)` | FAIL |

### Group 7 — Tag name expression (literal prefix/suffix)

| # | Test | Status |
|---|------|--------|
| 56 | `Fixture tag-name-expression-literal-prefix-suffix (htmljs target)` | FAIL |
| 57 | `Fixture tag-name-expression-literal-suffix (htmljs target)` | FAIL |

### Group 8 — Shorthand (id/class) edge cases

| # | Test | Status |
|---|------|--------|
| 58 | `Fixture shorthand-mixed (htmljs target)` | FAIL |
| 59 | `Fixture shorthand-id-dup (htmljs target)` | FAIL |

### Group 9 — Mixed (html+concise) mode errors

| # | Test | Status |
|---|------|--------|
| 60 | `Fixture mixed (htmljs target)` | FAIL |
| 61 | `Fixture mixed-bad-indentation (htmljs target)` | FAIL |
| 62 | `Fixture mixed-invalid (htmljs target)` | FAIL |
| 63 | `Fixture mixed-open-tag-only (htmljs target)` | FAIL |

### Group 10 — Concise-mode / statement / scriptlet / semicolon

| # | Test | Status |
|---|------|--------|
| 64 | `Fixture statement-concise-only (htmljs target)` | FAIL |
| 65 | `Fixture statement-root-only (htmljs target)` | FAIL |
| 66 | `Fixture semicolon-concise (htmljs target)` | FAIL |
| 67 | `Scriptlet after text inside fenced block` | PASS |
| 68 | `Fixture scriptlet-block-nested (htmljs target)` | FAIL |
| 69 | `Fixture scriptlet-block-invalid-comment (htmljs target)` | FAIL |
| 70 | `Fixture scriptlet-line-continue (htmljs target)` | FAIL |
| 71 | `Fixture concise-hyphen-after-tag-name (htmljs target)` | FAIL |
| 72 | `Script concise forms and script src attr (htmljs target)` | FAIL |
| 73 | `Fixture script (htmljs target)` | FAIL |
| 74 | `Fixture script-concise-invalid-nested-tag (htmljs target)` | FAIL |
| 75 | `Fixture bad-tag-indent (htmljs target)` | FAIL |

### Group 11 — Attribute group / comma / bound

| # | Test | Status |
|---|------|--------|
| 76 | `Multiple grouped concise attributes split by tag default` | FAIL |
| 77 | `Bound and tag-default ambiguity edge cases (htmljs target)` | FAIL |
| 78 | `Comma-separated multiline concise attributes (htmljs target)` | FAIL |
| 79 | `Fixture comma-after-tag-variable (htmljs target)` | PASS |

### Group 12 — Placeholder edge cases

| # | Test | Status |
|---|------|--------|
| 80 | `Fixture placeholder-backslash (htmljs target)` | FAIL |
| 81 | `Fixture placeholder-unnamed-attr-escaped (htmljs target)` | FAIL |
| 82 | `Fixture placeholder-unnamed-attr-escaped-escaped (htmljs target)` | FAIL |
| 83 | `Empty placeholder expression in dynamic tag` | FAIL |
| 84 | `Fixture eof-placeholder-body-escaped (htmljs target)` | FAIL |
| 85 | `Fixture eof-placeholder-body-non-escaped (htmljs target)` | FAIL |
| 86 | `Fixture eof-placeholder-concise (htmljs target)` | FAIL |
| 87 | `Fixture eof-placeholder-string (htmljs target)` | FAIL |

### Group 13 — EOF / unclosed / invalid structure

| # | Test | Status |
|---|------|--------|
| 88 | `Fixture unclosed-tag-eof (htmljs target)` | FAIL |
| 89 | `EOF doctype without closing bracket` | FAIL |
| 90 | `Fixture eof-xml-declaration (htmljs target)` | FAIL |
| 91 | `Fixture multiline-html-block-missing-end (htmljs target)` | FAIL |
| 92 | `Fixture multiline-html-block-same-line (htmljs target)` | FAIL |
| 93 | `Fixture multiline-text-block-bad (htmljs target)` | FAIL |
| 94 | `Fixture backtick-string-eof (htmljs target)` | FAIL |
| 95 | `Fixture invalid-closing-tag (htmljs target)` | FAIL |

### Group 14 — Open-tag-only / parsed-text / root

| # | Test | Status |
|---|------|--------|
| 96 | `Fixture open-tag-only-with-body (htmljs target)` | FAIL |
| 97 | `Fixture open-tag-only-with-body-concise (htmljs target)` | FAIL |
| 98 | `Fixture parsed-text-style-tag (htmljs target)` | FAIL |
| 99 | `Fixture root-el (htmljs target)` | FAIL |
| 100 | `Fixture text-after-semicolon (htmljs target)` | FAIL |

### Group 15 — CSS / misc complex

| # | Test | Status |
|---|------|--------|
| 101 | `Fixture complex (htmljs target)` | FAIL |
| 102 | `Fixture complex-attr-name (htmljs target)` | FAIL |
| 103 | `Fixture css-calc (htmljs target)` | PASS |
| 104 | `Fixture css-grid (htmljs target)` | PASS |
| 105 | `Fixture default-attr (htmljs target)` | FAIL |
| 106 | `Fixture html-comment-tag (htmljs target)` | FAIL |
| 107 | `Fixture stray-forward-slash-within-open-tag (htmljs target)` | FAIL |
| 108 | `Fixture stray-special-chars (htmljs target)` | FAIL |
| 109 | `Fixture strip-bom (htmljs target)` | PASS |
| 110 | `Fixture unary-as-member-expression (htmljs target)` | FAIL |

---

## Initial Claude prompt (paste this at the start of a new session)

```
We are fixing failing tree-sitter tests for the marko-tmbundle grammar.
Working directory: tree-sitter-marko/

Reference files:
- test/failing_tests_tracking.md — 110 failing tests grouped by root cause, fix in priority order.
- ../marko-prettier/src/index.ts — critical design reference for how Marko constructs
  map to JavaScript/TypeScript expressions.

Rules (non-negotiable):
- Run `tree-sitter test --overview-only` to confirm current failure count before starting.
- Fix grammar.js (or C scanner if needed) so the parser produces the correct tree.
- NEVER run `tree-sitter test --update` to auto-accept wrong output.
- Only update a test expectation when a node name changed upstream or a new node was introduced.
- Commit after each fixed test or coherent group: `git add -p && git commit`.
- After each commit, update test/failing_tests_tracking.md (flip FAIL → PASS).
- Then immediately pick and start the next highest-priority failing test without waiting.

Design reference — marko-prettier strategy (../marko-prettier/src/index.ts):
marko-prettier resolves the same parsing ambiguities by wrapping Marko sub-expressions
in JS/TS scaffolding before handing them to the Babel/TS parser. Use this as a guide
for what each Marko construct must look like to a JS/TS parser:
  - Attribute method `onClick() {}`  →  `function${value}` (strip "function" after formatting)
  - Tag variable `/{ x }: T`        →  `var ${code}=_` (strip "=_" after)
  - Tag type args `<T, U>`          →  `_<${code}>` (strip "_" after)
  - Tag params `|x, y|`             →  `function _(${code}){}`
  - Tag type params `<T extends X>` →  `function _<${code}>(){}`
  - Attribute / tag args            →  `_(${code})`
  - Attribute value `=expr`         →  sent as a bare TS expression

Apply the same mental model in the grammar: shape grammar rules so the external scanner
consumes full JS spans as opaque tokens, or use injections.scm to delegate those ranges
to the JavaScript/TypeScript tree-sitter grammar. This is especially important for:
  - Group 1 (TS generics): `<Tag<T>>` — second `<` must NOT be seen as a new HTML open
    tag; consume the whole `<…>` type-arg span (balanced) before the HTML layer sees it,
    just as marko-prettier wraps it as `_<T>`.
  - Group 2–4 (attr expressions): `typeof x`, `void 0`, `!x`, `/regex/` — all valid JS
    that marko-prettier sends straight to the TS expression parser without any wrapping.
    The grammar's attribute value rule must accept these without producing ERROR nodes.
  - Group 6 (tag variable): `/{ x }: Type` — marko-prettier wraps as `var X=_`. The
    tag_variable grammar rule must accept any JS destructuring LHS including typed patterns.
  - Group 5 (attr method): `onClick() {}` — marko-prettier prepends "function". The
    grammar external scanner must correctly handle the opening `(` of the params.

When starting a fix:
1. `tree-sitter test -i "<test name regex>"` to see the diff (expected vs actual).
2. Read the relevant corpus file in test/corpus/.
3. Read grammar.js and understand the rule(s) involved.
4. Check ../marko-prettier/src/index.ts for how the same construct is handled there.
5. Make the minimal, correct grammar change (refactor if fundamentally broken — no quick hacks).
6. `tree-sitter generate && tree-sitter test -i "<test name>"` to verify.
7. Run full `tree-sitter test --overview-only` to confirm no regressions.
8. Commit, update tracking file, move to next test.

Pick the next FAIL test from the tracking file (Group 1 first) and start fixing now.
```

---

## Progress log

| Date | Tests fixed | Commit |
|------|-------------|--------|
| 2026-04-09 | 1 (function_tag_statement GLR fix: allows self_closing_element to win when /> present) | 9603618 |
| 2026-04-09 | 1 (concise_fence_text: allow $<char> in text so 'hello $ var' is one text, not text+scriptlet) | e7b1cc2 |
| 2026-04-09 | 3 (BOM fix: add U+FEFF as grammar extra — fixes strip-bom, css-calc, css-grid) | 8f6f364 |
| 2026-04-09 | 2 (tag_variable after attribute list — fixes attr-simple-expression, comma-after-tag-variable, attr-unenclosed-whitespace-odd) | da187dd |
