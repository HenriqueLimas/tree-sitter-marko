(scriptlet) @injection.content
(scriptlet) @injection.language

(placeholder
  (javascript_fragment) @injection.content)

((placeholder
  (javascript_fragment) @injection.content)
 (#set! injection.language "javascript"))

((scriptlet) @injection.content
 (#set! injection.language "javascript"))

((style_block_css
  (style_block_content) @injection.content)
 (#set! injection.language "css"))

((style_block_less
  (style_block_content) @injection.content)
 (#set! injection.language "less"))

((style_block_scss
  (style_block_content) @injection.content)
 (#set! injection.language "scss"))

((style_block_js
  (script_block_content) @injection.content)
 (#set! injection.language "javascript"))

((style_block_ts
  (script_block_content) @injection.content)
 (#set! injection.language "typescript"))

((script_block
  (script_block_content) @injection.content)
 (#set! injection.language "javascript"))
