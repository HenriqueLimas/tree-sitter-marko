# tree-sitter-marko

Fresh tree-sitter grammar for Marko, rebuilt from current `Syntaxes/marko.tmLanguage` behavior.

## Neovim Setup (nvim-treesitter)

Add this to your Neovim config before `nvim-treesitter` is set up:

```lua
vim.api.nvim_create_autocmd('User', {
  pattern = 'TSUpdate',
  callback = function()
    require('nvim-treesitter.parsers').get_parser_configs().marko = {
      install_info = {
        url = 'https://github.com/HenriqueLimas/tree-sitter-marko',
        branch = 'main',
        files = { 'src/parser.c', 'src/scanner.c' },
      },
      filetype = 'marko',
    }
  end,
})
```

Then run `:TSInstall marko` to install the parser.

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
