(scriptlet) @injection.content
(scriptlet) @injection.language

(placeholder
  (javascript_fragment) @injection.content)

((placeholder
  (javascript_fragment) @injection.content)
 (#set! injection.language "javascript"))

((scriptlet) @injection.content
 (#set! injection.language "javascript"))
