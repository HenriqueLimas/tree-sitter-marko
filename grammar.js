module.exports = grammar({
  name: 'marko',

  extras: $ => [/\s/],

  rules: {
    document: $ => repeat($._document_node),

    _document_node: $ => choice(
      $.top_level_statement,
      $.function_tag_statement,
      $.concise_tag,
      $.concise_fence_block,
      $.concise_fence_line,
      $._node,
    ),

    _node: $ => choice(
      $.doctype,
      $.cdata,
      $.html_comment,
      $.style_block_css,
      $.style_block_less,
      $.style_block_scss,
      $.style_block_js,
      $.style_block_ts,
      $.script_block,
      $.scriptlet_block,
      $.scriptlet,
      $.placeholder,
      $.element,
      $.text,
    ),

    doctype: $ => seq('<!', /[Dd][Oo][Cc][Tt][Yy][Pp][Ee]/, /[^>]+/, optional('>')),

    cdata: _ => token(prec(2, /<!\[CDATA\[[\s\S]*?\]\]>/)),

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

    statement_block_tail: _ => /[^\n]*(\n[ \t][^\n]*)*(\n\})?/,

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
      optional($.tag_default_value),
      optional($.concise_attribute_group),
      repeat($.attribute),
      optional($.concise_terminator),
    ),

    concise_attribute_group: $ => seq('[', repeat($.attribute), ']'),

    concise_terminator: _ => ';',

    concise_fence_block: $ => seq('---', repeat(choice(
      $.element,
      $.placeholder,
      $.scriptlet_block,
      $.scriptlet,
      $.text,
    )), '---'),

    concise_fence_line: $ => seq('--', /[^\n]+/),

    function_tag_statement: $ => prec(2, seq(
      '<',
      field('name', $.function_tag_name),
      repeat($.shorthand_attribute),
      optional($.tag_variable),
      optional($.tag_default_value),
      optional($.tag_parameters),
      optional($.tag_arguments),
      optional($.tag_method),
      optional($.tag_default_value),
      repeat($.attribute),
      '>',
    )),

    element: $ => choice(
      $.self_closing_element,
      $.normal_element,
    ),

    normal_element: $ => seq(
      $.start_tag,
      repeat($._node),
      $.end_tag,
    ),

    self_closing_element: $ => choice(
      seq(
        '<',
        field('name', $._tag_name),
        repeat($.shorthand_attribute),
        optional($.tag_variable),
        optional($.tag_default_value),
        optional($.tag_parameters),
        optional($.tag_arguments),
        optional($.tag_method),
        optional($.tag_default_value),
        repeat($.attribute),
        '/>',
      ),
      seq(
        '<',
        repeat1($.shorthand_attribute),
        optional($.tag_variable),
        optional($.tag_default_value),
        optional($.tag_parameters),
        optional($.tag_arguments),
        optional($.tag_method),
        optional($.tag_default_value),
        repeat($.attribute),
        '/>',
      ),
    ),

    start_tag: $ => choice(
      seq(
        '<',
        field('name', $._tag_name),
        repeat($.shorthand_attribute),
        optional($.tag_variable),
        optional($.tag_default_value),
        optional($.tag_parameters),
        optional($.tag_arguments),
        optional($.tag_method),
        optional($.tag_default_value),
        repeat($.attribute),
        '>',
      ),
      seq(
        '<',
        repeat1($.shorthand_attribute),
        optional($.tag_variable),
        optional($.tag_default_value),
        optional($.tag_parameters),
        optional($.tag_arguments),
        optional($.tag_method),
        optional($.tag_default_value),
        repeat($.attribute),
        '>',
      ),
    ),

    end_tag: $ => seq('</', optional(field('name', $._tag_name)), '>'),

    _tag_name: $ => choice(
      $.builtin_tag_name,
      $.flow_tag_name,
      $.function_tag_name,
      $.dynamic_tag_name,
      $.tag_name,
    ),

    _concise_tag_name: $ => choice(
      $.flow_tag_name,
      $.function_tag_name,
      $.tag_name,
    ),

    builtin_tag_name: _ => choice('script', 'style', 'html-script', 'html-style', 'html-comment'),

    flow_tag_name: _ => choice('for', 'if', 'while', 'else-if', 'else', 'try', 'await', 'return'),

    function_tag_name: _ => choice('const', 'context', 'debug', 'define', 'id', 'let', 'log', 'lifecycle'),

    dynamic_tag_name: $ => seq('${', optional($.javascript_fragment), '}'),

    tag_name: _ => /[A-Za-z0-9_@][A-Za-z0-9_:@-]*/,

    tag_variable: $ => seq('/', optional($.tag_variable_fragment)),

    tag_default_value: $ => seq(choice(':=', '='), choice(
      $.quoted_attribute_value,
      $.attribute_bracket_value,
      $.attribute_paren_value,
      $.attribute_brace_value,
      $.backtick_attribute_value,
      $.attribute_expression_value,
      $.tag_default_fragment,
      $.unquoted_attribute_value,
    )),

    tag_parameters: $ => seq('|', optional($.tag_parameters_fragment), '|'),

    tag_arguments: $ => seq('(', repeat(choice(
      $.quoted_attribute_value,
      $.placeholder,
      $.tag_arguments_fragment,
      $.tag_arguments_nested,
    )), ')'),

    tag_method: $ => seq('{', repeat(choice(
      $.quoted_attribute_value,
      $.placeholder,
      $.tag_method_block_fragment,
    )), '}'),

    tag_variable_fragment: _ => /[^\s=|(){}>]+/,

    tag_default_fragment: _ => /[^\s>\/()\[\]{}"'`]+/,

    tag_parameters_fragment: _ => /[^|\n]*/,

    tag_arguments_fragment: _ => /[^()"'$]+/,

    tag_arguments_nested: $ => seq('(', repeat(choice(
      $.quoted_attribute_value,
      $.placeholder,
      $.tag_arguments_fragment,
      $.tag_arguments_nested,
    )), ')'),

    tag_method_block_fragment: _ => /[^"'$}]+/,

    attribute: $ => choice($.spread_attribute, $.regular_attribute),

    spread_attribute: $ => seq('...', alias($.spread_attribute_fragment, $.javascript_fragment)),

    spread_attribute_fragment: _ => /[^\s/>]+/,

    shorthand_attribute: _ => /[.#][A-Za-z0-9_$-]+/,

    regular_attribute: $ => seq(
      field('name', choice($.special_attribute_name, $.attribute_name)),
      optional($.attribute_bound_value),
      optional($.attribute_arguments),
      optional($.attribute_method),
      optional(seq('=', field('value', choice(
        $.attribute_expression_value,
        $.attribute_brace_value,
        $.backtick_attribute_value,
        $.quoted_attribute_value,
        $.attribute_bracket_value,
        $.attribute_paren_value,
        $.attribute_value_fragment,
        $.unquoted_attribute_value,
      )))),
    ),

    special_attribute_name: _ => /(?:key|on[A-Za-z0-9_$-]+|[A-Za-z0-9_$]+Change|no-update(?:-body)?(?:-if)?)/,

    attribute_name: _ => /[A-Za-z0-9_$][A-Za-z0-9_$-]*/,

    attribute_bound_value: $ => seq(':=', field('value', choice(
      $.attribute_expression_value,
      $.attribute_brace_value,
      $.backtick_attribute_value,
      $.quoted_attribute_value,
      $.attribute_bracket_value,
      $.attribute_paren_value,
      $.attribute_value_fragment,
      $.unquoted_attribute_value,
    ))),

    attribute_arguments: $ => seq('(', optional($.attribute_arguments_fragment), ')'),

    attribute_method: $ => seq('{', repeat(choice(
      $.quoted_attribute_value,
      $.placeholder,
      $.attribute_method_fragment,
      $.attribute_method_nested_block,
    )), '}'),

    attribute_paren_value: $ => seq('(', repeat(choice(
      $.quoted_attribute_value,
      $.placeholder,
      $.attribute_paren_fragment,
      $.attribute_paren_value,
    )), ')'),

    attribute_paren_fragment: _ => /[^()"'$]+/,

    attribute_brace_value: $ => seq('{', repeat(choice(
      $.quoted_attribute_value,
      $.placeholder,
      $.attribute_brace_fragment,
      $.attribute_brace_value,
    )), '}'),

    attribute_brace_fragment: _ => /[^{}"'$]+/,

    attribute_bracket_value: $ => seq('[', repeat(choice(
      $.quoted_attribute_value,
      $.placeholder,
      $.attribute_bracket_fragment,
    )), ']'),

    attribute_bracket_fragment: _ => /[^"'$\]]+/,

    attribute_arguments_fragment: _ => /[^)\n]*/,

    attribute_method_fragment: _ => /[^{}"'$]+/,

    attribute_method_nested_block: $ => seq('{', repeat(choice(
      $.quoted_attribute_value,
      $.placeholder,
      $.attribute_method_fragment,
      $.attribute_method_nested_block,
    )), '}'),

    attribute_value_fragment: _ => /[^\s,>"'`()\[\]{}\/]+/,

    attribute_expression_value: $ => seq(
      $.attribute_expression_atom,
      repeat1(seq($.attribute_expression_operator, $.attribute_expression_atom)),
    ),

    attribute_expression_atom: $ => choice(
      $.quoted_attribute_value,
      $.backtick_attribute_value,
      $.placeholder,
      $.attribute_paren_value,
      $.attribute_bracket_value,
      $.attribute_brace_value,
      $.attribute_expression_fragment,
    ),

    attribute_expression_operator: _ => /\+|\-|\*|\/|%|&&|\|\||\?\?|===|==|!==|!=|<=|>=|<|>/,

    attribute_expression_fragment: _ => /[A-Za-z0-9_.:]+/,

    backtick_attribute_value: $ => seq('`', repeat(choice(/[^`$]+/, $.placeholder)), '`'),

    quoted_attribute_value: $ => choice(
      seq('"', repeat(choice(/[^"$]+/, $.placeholder, $.quoted_dollar_fragment, $.quoted_dollar_brace_fragment)), '"'),
      seq("'", repeat(choice(/[^'$]+/, $.placeholder, $.quoted_dollar_fragment, $.quoted_dollar_brace_fragment)), "'"),
    ),

    quoted_dollar_fragment: _ => /\$[^\{][^"']*/,

    quoted_dollar_brace_fragment: _ => /\$\{[^}"']*;[^}"']*/,

    unquoted_attribute_value: _ => /[^\s"'=<>`()\[\]{}\/]+/,

    scriptlet: _ => /\$\s+[^\s{\n][^\n]*/,

    scriptlet_block: $ => seq('$', '{', optional($.scriptlet_block_content), '}'),

    scriptlet_block_content: _ => /[^}]*/,

    placeholder: $ => choice(
      seq('${', $.javascript_fragment, '}'),
      seq('$!{', $.javascript_fragment, '}'),
      seq('${`', alias($.template_placeholder_fragment, $.javascript_fragment), '`}'),
    ),

    template_placeholder_fragment: _ => /[^`]*\$\{[^}]+\}[^`]*/,

    javascript_fragment: _ => /[^{}<>\n=][^{}<>\n=]*/,

    text: _ => token(prec(-1, /[^<$\n][^<$\n]*/)),
  },
})
