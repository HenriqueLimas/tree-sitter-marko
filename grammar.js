module.exports = grammar({
  name: 'marko',

  extras: $ => [/\s/],

  rules: {
    document: $ => repeat($._document_node),

    _document_node: $ => choice(
      $.top_level_statement,
      $.concise_tag,
      $.concise_fence_block,
      $.concise_fence_line,
      $._node,
    ),

    _node: $ => choice(
      $.doctype,
      $.html_comment,
      $.style_block_css,
      $.style_block_less,
      $.style_block_scss,
      $.style_block_js,
      $.style_block_ts,
      $.script_block,
      $.scriptlet,
      $.placeholder,
      $.element,
      $.text,
    ),

    doctype: $ => seq('<!', /[Dd][Oo][Cc][Tt][Yy][Pp][Ee]/, /[^>]+/, '>'),

    html_comment: _ => token(prec(2, /<!--([^-]|-[^-])*-->/)),

    top_level_statement: $ => choice(
      seq('import', optional($.statement_line_tail)),
      seq('export', optional($.statement_block_tail)),
      seq('class', optional($.statement_block_tail)),
      seq('server', optional($.statement_line_tail)),
      seq('client', optional($.statement_line_tail)),
      seq('static', optional($.statement_block_tail)),
    ),

    statement_line_tail: _ => /[^\n]*/,

    statement_block_tail: _ => /[^{}\n]*(\{[^}]*\})?[^\n]*/,

    style_block_css: $ => prec(2, seq('style', '{', optional($.style_block_content), '}')),

    style_block_less: $ => prec(2, seq('style.less', '{', optional($.style_block_content), '}')),

    style_block_scss: $ => prec(2, seq('style.scss', '{', optional($.style_block_content), '}')),

    style_block_js: $ => prec(2, seq(choice('style.js', 'style.mjs', 'style.cjs'), '{', optional($.script_block_content), '}')),

    style_block_ts: $ => prec(2, seq(choice('style.ts', 'style.mts', 'style.cts'), '{', optional($.script_block_content), '}')),

    script_block: $ => prec(2, seq('script', '{', optional($.script_block_content), '}')),

    style_block_content: _ => /[^}]*/,

    script_block_content: _ => /[^}]*/,

    concise_tag: $ => seq(
      field('name', $._concise_tag_name),
      repeat($.shorthand_attribute),
      optional($.tag_variable),
      optional($.tag_default_value),
      optional($.tag_parameters),
      optional($.tag_arguments),
      optional($.tag_method),
      optional($.concise_attribute_group),
      repeat($.attribute),
      optional($.concise_terminator),
    ),

    concise_attribute_group: $ => seq('[', repeat($.attribute), ']'),

    concise_terminator: _ => ';',

    concise_fence_block: $ => seq('---', repeat(choice($.element, $.placeholder, $.text)), '---'),

    concise_fence_line: $ => seq('--', /[^\n]+/),

    element: $ => choice(
      $.normal_element,
      $.self_closing_element,
    ),

    normal_element: $ => seq(
      $.start_tag,
      repeat($._node),
      $.end_tag,
    ),

    self_closing_element: $ => seq(
      '<',
      field('name', $._tag_name),
      repeat($.shorthand_attribute),
      optional($.tag_variable),
      optional($.tag_default_value),
      optional($.tag_parameters),
      optional($.tag_arguments),
      optional($.tag_method),
      repeat($.attribute),
      '/>',
    ),

    start_tag: $ => seq(
      '<',
      field('name', $._tag_name),
      repeat($.shorthand_attribute),
      optional($.tag_variable),
      optional($.tag_default_value),
      optional($.tag_parameters),
      optional($.tag_arguments),
      optional($.tag_method),
      repeat($.attribute),
      '>',
    ),

    end_tag: $ => seq('</', field('name', $._tag_name), '>'),

    _tag_name: $ => choice(
      $.builtin_tag_name,
      $.flow_tag_name,
      $.function_tag_name,
      $.dynamic_tag_name,
      $.tag_name,
    ),

    _concise_tag_name: $ => choice(
      $.builtin_tag_name,
      $.flow_tag_name,
      $.function_tag_name,
      $.tag_name,
    ),

    builtin_tag_name: _ => choice('script', 'style', 'html-script', 'html-style', 'html-comment'),

    flow_tag_name: _ => choice('for', 'if', 'while', 'else-if', 'else', 'try', 'await', 'return'),

    function_tag_name: _ => choice('const', 'context', 'debug', 'define', 'id', 'let', 'log', 'lifecycle'),

    dynamic_tag_name: $ => seq('${', $.javascript_fragment, '}'),

    tag_name: _ => /[A-Za-z0-9_@][A-Za-z0-9_:@-]*/,

    tag_variable: $ => seq('/', $.tag_variable_fragment),

    tag_default_value: $ => seq('=', $.tag_default_fragment),

    tag_parameters: $ => seq('|', optional($.tag_parameters_fragment), '|'),

    tag_arguments: $ => seq('(', optional($.tag_arguments_fragment), ')'),

    tag_method: $ => seq('{', optional($.tag_method_fragment), '}'),

    tag_variable_fragment: _ => /[^\s=|(){}>]+/,

    tag_default_fragment: _ => /[^\s>]+/,

    tag_parameters_fragment: _ => /[^|\n]*/,

    tag_arguments_fragment: _ => /[^)\n]*/,

    tag_method_fragment: _ => /[^}\n]*/,

    attribute: $ => choice($.spread_attribute, $.regular_attribute),

    spread_attribute: $ => seq('...', $.javascript_fragment),

    shorthand_attribute: _ => /[.#][A-Za-z0-9_$-]+/,

    regular_attribute: $ => seq(
      field('name', choice($.special_attribute_name, $.attribute_name)),
      optional($.attribute_arguments),
      optional($.attribute_method),
      optional(seq('=', field('value', choice(
        $.quoted_attribute_value,
        $.attribute_bracket_value,
        $.attribute_value_fragment,
        $.unquoted_attribute_value,
      )))),
    ),

    special_attribute_name: _ => /(?:key|on[A-Za-z0-9_$-]+|[A-Za-z0-9_$]+Change|no-update(?:-body)?(?:-if)?)/,

    attribute_name: _ => /[A-Za-z0-9_$][A-Za-z0-9_$:-]*/,

    attribute_arguments: $ => seq('(', optional($.attribute_arguments_fragment), ')'),

    attribute_method: $ => seq('{', optional($.attribute_method_fragment), '}'),

    attribute_bracket_value: $ => seq('[', repeat(choice(
      $.quoted_attribute_value,
      $.placeholder,
      $.attribute_bracket_fragment,
    )), ']'),

    attribute_bracket_fragment: _ => /[^"'$\]]+/,

    attribute_arguments_fragment: _ => /[^)\n]*/,

    attribute_method_fragment: _ => /[^}\n]*/,

    attribute_value_fragment: _ => /[^\s,>"']+/,

    quoted_attribute_value: $ => choice(
      seq('"', repeat(choice(/[^"$]+/, $.placeholder)), '"'),
      seq("'", repeat(choice(/[^'$]+/, $.placeholder)), "'"),
    ),

    unquoted_attribute_value: _ => /[^\s"'=<>`]+/,

    scriptlet: _ => /\$\s+[^\n]+/,

    placeholder: $ => choice(
      seq('${', $.javascript_fragment, '}'),
      seq('$!{', $.javascript_fragment, '}'),
    ),

    javascript_fragment: _ => /[^{}<>\n"'`=][^{}<>\n"'`=]*/,

    text: _ => token(prec(-1, /[^<$\n][^<$\n]*/)),
  },
})
