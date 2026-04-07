(doctype) @keyword

(scriptlet) @keyword

(placeholder
  ["${" "$!{" "}"] @punctuation.special)

(builtin_tag_name) @tag
(flow_tag_name) @conditional
(function_tag_name) @function
(dynamic_tag_name
  ["${" "}"] @punctuation.special)

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
