# htmljs fixture tracking

Source: `../htmljs-parser/src/__tests__/fixtures`

Tracking method: exact `input.marko` snippet present in `test/corpus/*.txt`.
Last updated: 2026-04-08

## Working instructions

1. Commit on every phase.
2. Update this tracking file on every phase.
3. Test expectations must follow htmljs-parser output.
4. NEVER change expected test output just to make tests pass; fix parser code/grammar so tests pass.
5. Any newly added failing htmljs-target case triggers a fix phase immediately after the add phase.
6. Do not wait for user approval to start the next phase after finishing a phase.

## Deferred blocker

- `attr-comma-multiline` remains intentionally in `test/corpus/pending_regressions.txt`.
- Reason: parser still splits multiline comma attributes into separate concise tags instead of one concise tag with attributes.
- Action: continue coverage for other fixtures and revisit this blocker in dedicated fix phases.

## Snapshot

- Total fixtures scanned: 318
- Exact-input covered in corpus: 65
- Exact-input missing in corpus: 253

## Recently covered in this phase

- `attr-grouped-4` -> `test/corpus/attribute_group_fixtures.txt`
- `attr-grouped-invalid` -> `test/corpus/attribute_group_fixtures.txt`
- `attr-grouped-invalid-eof` -> `test/corpus/attribute_group_fixtures.txt`
- `attr-bound` -> `test/corpus/attribute_error_fixtures.txt`
- `attr-literal-false` -> `test/corpus/attribute_value_fixtures.txt`
- `attr-literal-null` -> `test/corpus/attribute_value_fixtures.txt`
- `attr-literal-numbers` -> `test/corpus/attribute_value_fixtures.txt`
- `attr-literal-string-single-quotes` -> `test/corpus/attribute_value_fixtures.txt`
- `attr-literal-true` -> `test/corpus/attribute_value_fixtures.txt`
- `attr-literal-undefined` -> `test/corpus/attribute_value_fixtures.txt`
- `attr-value-self-closed` -> `test/corpus/attribute_value_fixtures.txt`
- `attr-comma-multiline` -> `test/corpus/pending_regressions.txt` (pending)
- `attr-inconsistent-commas` -> `test/corpus/comma_attribute_fixtures.txt`
- `comma-ends-attr-operators` -> `test/corpus/comma_attribute_fixtures.txt`
- `attr-literal-true` -> `test/corpus/attribute_value_fixtures.txt`
- `attr-literal-false` -> `test/corpus/attribute_value_fixtures.txt`
- `attr-literal-null` -> `test/corpus/attribute_value_fixtures.txt`
- `attr-literal-undefined` -> `test/corpus/attribute_value_fixtures.txt`
- `attr-literal-numbers` -> `test/corpus/attribute_value_fixtures.txt`
- `attr-literal-string-single-quotes` -> `test/corpus/attribute_value_fixtures.txt`

## Known exact matches currently tracked

- `argument-and-params` -> `test/corpus/core_syntax_regressions.txt`
- `argument-attr` -> `test/corpus/attribute_methods.txt`
- `argument-attr-extra-whitespace` -> `test/corpus/attribute_methods.txt`
- `argument-attr-multiple` -> `test/corpus/attribute_methods.txt`
- `argument-tag` -> `test/corpus/argument_tag_fixtures.txt`
- `argument-tag-complex` -> `test/corpus/marko_tags.txt`
- `argument-tag-multiple` -> `test/corpus/argument_tag_fixtures.txt`
- `argument-tag-extra-whitespace` -> `test/corpus/argument_tag_fixtures.txt`
- `argument-tag-nested-parens` -> `test/corpus/argument_tag_fixtures.txt`
- `attr-value-string-concat` -> `test/corpus/attribute_value_fixtures.txt`
- `attr-grouped` -> `test/corpus/attribute_group_fixtures.txt`
- `attr-grouped-2` -> `test/corpus/attribute_group_fixtures.txt`
- `attr-grouped-3` -> `test/corpus/attribute_group_fixtures.txt`
- `attr-grouped-4` -> `test/corpus/attribute_group_fixtures.txt`
- `attr-grouped-invalid` -> `test/corpus/attribute_group_fixtures.txt`
- `attr-grouped-invalid-eof` -> `test/corpus/attribute_group_fixtures.txt`
- `attr-grouped-multiple` -> `test/corpus/attribute_group_fixtures.txt`
- `attr-bound` -> `test/corpus/attribute_error_fixtures.txt`
- `attr-with-parens-and-double-quoted-strings` -> `test/corpus/attribute_value_fixtures.txt`
- `backtick-string` -> `test/corpus/attribute_value_fixtures.txt`
- `cdata` -> `test/corpus/cdata_and_doctype_fixtures.txt`
- `comment-concise-js-block` -> `test/corpus/concise_comment_fixtures.txt`
- `comment-concise-js-block-trailing` -> `test/corpus/concise_comment_fixtures.txt`
- `comment-concise-js-line` -> `test/corpus/concise_comment_fixtures.txt`
- `comments-within-open-tag` -> `test/corpus/open_tag_comment_fixtures.txt`
- `declaration` -> `test/corpus/xml_declaration_fixtures.txt`
- `double-hyphen-block` -> `test/corpus/double_hyphen_fixtures.txt`
- `double-hyphen-line` -> `test/corpus/double_hyphen_fixtures.txt`
- `double-hyphen-line-start` -> `test/corpus/double_hyphen_fixtures.txt`
- `eof-attr-name` -> `test/corpus/eof_open_tag_fixtures.txt`
- `eof-attr-value` -> `test/corpus/eof_open_tag_fixtures.txt`
- `eof-doctype` -> `test/corpus/cdata_and_doctype_fixtures.txt`
- `eof-tag-start` -> `test/corpus/eof_open_tag_fixtures.txt`
- `empty-closing-tag2` -> `test/corpus/comments_and_edges.txt`
- `html-comments` -> `test/corpus/comments_and_edges.txt`
- `invalid-multiple-tag-params` -> `test/corpus/core_syntax_regressions.txt`
- `open-tag-only` -> `test/corpus/void_tag_fixtures.txt`
- `open-tag-only-with-body` -> `test/corpus/void_tag_fixtures.txt`
- `placeholder-attr-invalid` -> `test/corpus/placeholder_edge_fixtures.txt`
- `placeholder-empty` -> `test/corpus/placeholder_edge_fixtures.txt`
- `placeholder-no-escape` -> `test/corpus/placeholder_edge_fixtures.txt`
- `script-concise` -> `test/corpus/concise_script_fixtures.txt`
- `script-empty-concise` -> `test/corpus/concise_script_fixtures.txt`
- `scriptlet-block-html` -> `test/corpus/scriptlet_fixtures.txt`
- `scriptlet-line-html` -> `test/corpus/scriptlet_fixtures.txt`
- `scriptlet-line-no-middle-html` -> `test/corpus/scriptlet_fixtures.txt`
- `scriptlet-terminated-by-semi-colon` -> `test/corpus/scriptlet_fixtures.txt`
- `shorthand-closing-html2` -> `test/corpus/shorthand_fixtures.txt`
- `shorthand-div-id` -> `test/corpus/shorthand_id_class_fixtures.txt`
- `shorthand-id-class-html` -> `test/corpus/shorthand_id_class_fixtures.txt`
- `tag-name-expression-literal-prefix-attrs` -> `test/corpus/marko_tags.txt`
- `text-after-tag` -> `test/corpus/core_syntax_regressions.txt`
- `text-parse-mode-delimited-block` -> `test/corpus/concise_mode.txt`
- `attr-grouped` -> `test/corpus/attribute_group_fixtures.txt`
- `argument-attr-multiple` -> `test/corpus/attribute_methods.txt`
- `unclosed-tag-eof` -> `test/corpus/eof_open_tag_fixtures.txt`
- `void-tag-self-closed` -> `test/corpus/void_tag_fixtures.txt`
- `whitespace-around-equals` -> `test/corpus/core_syntax_regressions.txt`
- `xml-declaration` -> `test/corpus/xml_declaration_fixtures.txt`
- `attr-value-missing` -> `test/corpus/attribute_error_fixtures.txt`

## Backlog (remaining exact-input fixtures)

```text
attr-complex
attr-complex-functions
attr-complex-instanceof
attr-complex-unary
attr-concise-hyphens
attr-escape-characters
attr-escaped-dollar
attr-inc-and-dec
attr-int-and-id
attr-literal-regex
attr-literal-string-double-quotes
attr-literal-string-single-quotes
attr-method-shorthand
attr-method-shorthand-with-type-parameters
attr-method-with-type-parameters
attr-multi-line-comment
attr-multi-line-string
attr-name-with-html-chars
attr-nested-literal-regex
attr-no-value
attr-non-literal
attr-operator-whitespace-eof
attr-operators-newline-after
attr-operators-newline-before
attr-operators-space-after
attr-operators-space-before
attr-operators-space-between
attr-regex-character-classes
attr-regexp
attr-simple-expression
attr-unenclosed-whitespace
attr-unenclosed-whitespace-minus
attr-unenclosed-whitespace-multiple-attrs
attr-unenclosed-whitespace-odd
attr-value-missing
attr-value-single-quote-escaped
attr-value-typeof
attr-value-typeof-comma
attr-value-typeof-no-space-prefix
attr-value-void
attr-with-right-angle-bracket
attr-with-right-angle-bracket-parens
attr-without-delimiters
backtick-string-eof
backtick-string-nested-with-space
backtick-string-placeholder-newline
bad-tag-indent
coma-after-open-tag
comma-after-tag-variable
commas-relax
commas-require-all-attrs-first
commas-require-all-attrs-last
comments-within-tag-body
complex
complex-attr-name
concise-block-whitespace-leading
concise-contentplaceholder-html-start
concise-contentplaceholder-start
concise-hyphen-after-tag-name
concise-line-whitespace-ending
concise-multiline-content
css-calc
css-grid
default-attr
default-attr-comma
default-attr-no-space
dtd
empty-closing-tag
eof-attr-argument
eof-attr-value-js-comment-comment
eof-attr-value-js-comment-comment-concise
eof-attr-value-string
eof-placeholder-body-escaped
eof-placeholder-body-non-escaped
eof-placeholder-concise
eof-placeholder-script-body-escaped
eof-placeholder-script-body-non-escaped
eof-placeholder-string
eof-script-body
eof-style
eof-xml-declaration
html-block-after-tag
html-comment-tag
html-comments-ignore-placeholders
ignoreNonstandardStringPlaceholders
ignorePlaceholders-attr-double-escaped-dollar-placeholder
ignorePlaceholders-attr-escaped-dollar-placeholder
ignorePlaceholders-attr-placeholder
invalid-closing-tag
invalid-missing-tag-var
invalid-multiple-tag-type-params
invalid-type-args-after-space
invalid-type-params-after-args
invalid-type-params-after-method
invalid-type-params-after-params
invalid-type-params-attr-arg
mixed
mixed-bad-indentation
mixed-block-root
mixed-cdata
mixed-comment
mixed-invalid
mixed-open-tag-only
multi-line-attrs
multiline-html-block
multiline-html-block-empty
multiline-html-block-missing-end
multiline-html-block-nested
multiline-html-block-nested-empty
multiline-html-block-same-line
multiline-text-block
multiline-text-block-bad
multiline-text-block-with-hyphens
multiple-tags-on-line-close-next
nested-pyramid
nested-siblings
nested-tags
nested-text
open-tag-only-with-body-concise
param-tag
parsed-text-placeholder-in-template-literal-string
parsed-text-style-tag
placeholder-attr-string-value
placeholder-attr-string-value-parens
placeholder-attr-within-string-complex
placeholder-attr-within-string-simple
placeholder-attr-wrapped
placeholder-backslash
placeholder-body-multi-line-js-comment
placeholder-body-single-line-js-comment
placeholder-body-text
placeholder-escape
placeholder-tag-name-concise
placeholder-tag-name-html
placeholder-unnamed-attr
placeholder-unnamed-attr-escaped
placeholder-unnamed-attr-escaped-escaped
placeholder-unnamed-attr-last
placeholder-unnamed-attr-last-concise
placeholder-unnamed-attr-last-self-closed
placeholder-within-placeholder
placeholder-within-placeholder-escaping
placeholder-within-script-tag
placeholder-within-script-within-string
placeholder-within-string-newlines
root-el
root-els
script
script-concise-invalid-nested-tag
script-escaped-placeholder
script-self-closed
script-single-line-comment
script-tag-entities
script-with-strings
scriptlet-block
scriptlet-block-division
scriptlet-block-invalid-comment
scriptlet-block-multiline-comment
scriptlet-block-nested
scriptlet-block-regex
scriptlet-block-single-line-comment
scriptlet-block-template-literal-href
scriptlet-block-with-semi-colon
scriptlet-comment
scriptlet-eof
scriptlet-line
scriptlet-line-continue
scriptlet-line-continue-chain
scriptlet-line-multiline-comments
scriptlet-line-no-middle
scriptlet-line-regex-open-bracket
scriptlet-line-regex-open-curly
scriptlet-line-template-literal
scriptlet-line-trailing-line-comment
self-closed-tag
self-closed-tag-attrs
self-closing-open-tag-only
semicolon-concise
shorthand-class-dynamic
shorthand-class-dynamic-literal-prefix
shorthand-class-dynamic-literal-suffix
shorthand-class-self-closed
shorthand-closing-html
shorthand-div-id-class
shorthand-div-id-class-multiple
shorthand-id
shorthand-id-class
shorthand-id-dup
shorthand-id-dynamic
shorthand-id-dynamic-literal-prefix
shorthand-id-dynamic-literal-suffix
shorthand-id-self-closed
shorthand-mixed
shorthand-mixed-concise
simple-text-delimited
single-line-text-block
single-line-text-block-nested
single-line-text-block-sibling
statement-concise-only
statement-root-only
stray-forward-slash-within-open-tag
stray-special-chars
strip-bom
tag-name-expression-literal-prefix
tag-name-expression-literal-prefix-suffix
tag-name-expression-literal-suffix
tag-name-expression-shorthand-id
tag-name-expression-simple
tag-name-expression-simple-empty-close
tag-params-with-type-parameters
tag-type-argument-arrow-function
tag-var-before-concise-text
tag-var-declaration
tag-var-type-with-parens
tag-var-with-params
tag-with-type-arguments
text-after-semicolon
text-parse-mode-concise
text-parse-mode-html
textarea
ts-function-type
ts-generic-complex
ts-generic-function-type
ts-generic-simple
ts-intersection-type
ts-keyof-typeof
ts-nested-generics
ts-static-const-arrow-function-return-type
ts-static-type
ts-tag-var-type-generic
ts-type-statement
ts-unary-exression
unary-as-member-expression
var
var-new-Date
whitespace-empty-lines
whitespace-empty-lines-indent
whitespace-newline
whitespace-newline-indent
whitespace-only-reset-indent
whitespace-preservation
xml-declaration-ill-formed
```
- `attr-spread` -> `test/corpus/attribute_spread_fixtures.txt`
- `attr-value-self-closed` -> `test/corpus/attribute_value_fixtures.txt`
- `attr-inconsistent-commas` -> `test/corpus/comma_attribute_fixtures.txt`
- `comma-ends-attr-operators` -> `test/corpus/comma_attribute_fixtures.txt`
