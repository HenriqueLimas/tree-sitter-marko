module.exports = grammar({
  name: 'marko',

  extras: $ => [/\s/],

  rules: {
    document: $ => repeat($._node),

    _node: $ => choice(
      $.doctype,
      $.scriptlet,
      $.placeholder,
      $.top_level_statement,
      $.element,
      $.text,
    ),

    doctype: $ => seq('<!', /[Dd][Oo][Cc][Tt][Yy][Pp][Ee]/, /[^>]+/, '>'),

    top_level_statement: $ => choice(
      seq('static', optional($.statement_tail)),
      seq('class', optional($.statement_tail)),
      seq('import', optional($.statement_tail)),
      seq('export', optional($.statement_tail)),
    ),

    statement_tail: _ => /[^\n]*/,

    element: $ => choice(
      $.normal_element,
      $.self_closing_element,
    ),

    normal_element: $ => seq(
      $.start_tag,
      repeat($._node),
      $.end_tag,
    ),

    self_closing_element: $ => seq($.start_tag, '/>'),

    start_tag: $ => seq(
      '<',
      field('name', $.tag_name),
      repeat($.attribute),
      '>',
    ),

    end_tag: $ => seq('</', field('name', $.tag_name), '>'),

    tag_name: _ => /[A-Za-z0-9_@][A-Za-z0-9_:@.-]*/,

    attribute: $ => choice($.spread_attribute, $.regular_attribute, $.shorthand_attribute),

    spread_attribute: $ => seq('...', $.javascript_fragment),

    shorthand_attribute: _ => /[.#][A-Za-z0-9_$-]+/,

    regular_attribute: $ => seq(
      field('name', $.attribute_name),
      optional(seq('=', field('value', choice($.quoted_attribute_value, $.javascript_fragment, $.unquoted_attribute_value)))),
    ),

    attribute_name: _ => /[A-Za-z0-9_$][A-Za-z0-9_$:-]*/,

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
