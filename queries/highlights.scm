(doctype) @keyword

(scriptlet) @keyword

(placeholder
  ["${" "$!{" "}"] @punctuation.special)

(tag_name) @tag
(attribute_name) @tag.attribute
(shorthand_attribute) @tag.attribute

[
  "<"
  "</"
  ">"
  "/>"
] @tag.delimiter

(quoted_attribute_value) @string
(unquoted_attribute_value) @string

(top_level_statement) @none
(text) @none
