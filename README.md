# tree-sitter-marko

Fresh tree-sitter grammar for Marko, rebuilt from current `Syntaxes/marko.tmLanguage` behavior.

## Development

- Generate parser: `npm run generate`
- Run tests: `npm test`

## Scope

This parser targets:

- HTML mode tags
- Placeholders and scriptlets
- Top-level `import` / `export` / `class` / `static`
- Concise mode (iteratively added)
- Embedded script/style regions (via injections)
