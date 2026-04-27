#include "tree_sitter/parser.h"
#include <stdbool.h>
#include <string.h>

/*
 * External scanner for tree-sitter-marko.
 *
 * Tokens:
 *
 *  IMPLICIT_CLOSE  — zero-width token emitted when the next char is '<' with
 *                    no preceding whitespace.  Ends an open_element rule.
 *
 *  GT_AFTER_IMPLICIT — consumes '>' but ONLY after IMPLICIT_CLOSE was emitted.
 *                      Used by start_tag_doc instead of the regular '>'.
 *
 *  TS_ATTR_EXPRESSION_VALUE — hidden external token.  Fires at the start of an
 *                      attribute value (after '=') when the value is a TypeScript
 *                      type-assertion or satisfaction expression:
 *                        <atom> 'as'|'satisfies' <ts-type>
 *                      e.g.  x as Array<T>
 *                            fn as (x: number) => string
 *                            fn as <T>(x: T) => T
 *                            x as keyof typeof T
 *                            x as A & B<T>
 *                            val as readonly string[]
 *                      Because this token is hidden (starts with '_' in the
 *                      grammar), the parent attribute_expression_value node
 *                      appears as a leaf — matching the corpus expectations.
 *                      If no 'as'/'satisfies' follows the initial atom, the
 *                      scanner returns false and other grammar rules take over.
 *
 *  TAG_VAR_FRAGMENT — consumes a tag-variable fragment (after '/') that may
 *                     include a TS type annotation with balanced angle brackets,
 *                     e.g. `foo:Bar<\n  A,\n  B\n>`.  Whitespace inside
 *                     angle brackets (depth > 0) is consumed transparently.
 */

enum TokenType {
  IMPLICIT_CLOSE,
  GT_AFTER_IMPLICIT,
  TS_ATTR_EXPRESSION_VALUE,
  TAG_VAR_FRAGMENT,
  TAG_TYPE_ARGS_LT,       /* hidden: consumes '<' immediately (no space) */
  TAG_TYPE_ARGS_FRAGMENT,  /* content inside tag_type_args */
  TAG_TYPE_PARAMS_LT,     /* hidden: consumes '<' after optional whitespace (with space) */
  TAG_TYPE_PARAMS_FRAGMENT, /* content inside tag_type_params */
};

typedef struct {
  bool after_implicit;
  bool after_tag_type_args_lt;    /* true immediately after _tag_type_args_lt fired */
  bool after_tag_type_params_lt;  /* true immediately after _tag_type_params_lt fired */
} Scanner;

void *tree_sitter_marko_external_scanner_create(void) {
  Scanner *s = malloc(sizeof(Scanner));
  s->after_implicit = false;
  s->after_tag_type_args_lt = false;
  s->after_tag_type_params_lt = false;
  return s;
}

void tree_sitter_marko_external_scanner_destroy(void *payload) {
  free(payload);
}

void tree_sitter_marko_external_scanner_reset(void *payload) {
  Scanner *s = payload;
  s->after_implicit = false;
  s->after_tag_type_args_lt = false;
  s->after_tag_type_params_lt = false;
}

unsigned tree_sitter_marko_external_scanner_serialize(void *payload, char *buffer) {
  Scanner *s = payload;
  buffer[0] = (char)s->after_implicit;
  buffer[1] = (char)s->after_tag_type_args_lt;
  buffer[2] = (char)s->after_tag_type_params_lt;
  return 3;
}

void tree_sitter_marko_external_scanner_deserialize(
    void *payload, const char *buffer, unsigned length) {
  Scanner *s = payload;
  s->after_implicit = length > 0 && buffer[0];
  s->after_tag_type_args_lt = length > 1 && buffer[1];
  s->after_tag_type_params_lt = length > 2 && buffer[2];
}

/* ── helpers ──────────────────────────────────────────────────────────────── */

static bool is_ident_char(int32_t c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
         (c >= '0' && c <= '9') || c == '_' || c == '$' || c == '.';
}

static bool is_ws(int32_t c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/* Consume a string literal (single or double quoted). */
static void consume_string(TSLexer *lexer, int32_t quote) {
  lexer->advance(lexer, false);
  while (lexer->lookahead != 0 && lexer->lookahead != quote) {
    if (lexer->lookahead == '\\') lexer->advance(lexer, false);
    lexer->advance(lexer, false);
  }
  if (lexer->lookahead == quote) lexer->advance(lexer, false);
}

/* Consume a balanced bracket pair (open/close).  Handles nesting and strings. */
static void consume_balanced(TSLexer *lexer, int32_t open, int32_t close) {
  int depth = 0;
  while (lexer->lookahead != 0) {
    int32_t c = lexer->lookahead;
    if (c == open) {
      depth++;
      lexer->advance(lexer, false);
    } else if (c == close) {
      depth--;
      lexer->advance(lexer, false);
      if (depth == 0) break;
    } else if (c == '\'' || c == '"') {
      consume_string(lexer, c);
    } else {
      lexer->advance(lexer, false);
    }
  }
}

/* ── TS type expression scanner ───────────────────────────────────────────── */

static void scan_ts_type_atom(TSLexer *lexer);

static const char *const TS_UNARY_KEYWORDS[] = {
  "keyof", "typeof", "readonly", "unique", "infer", "asserts", "is", NULL
};

static void scan_ts_type_atom(TSLexer *lexer) {
  /* Try unary TS keywords. */
  for (int i = 0; TS_UNARY_KEYWORDS[i]; i++) {
    const char *kw = TS_UNARY_KEYWORDS[i];
    if (lexer->lookahead != (int32_t)kw[0]) continue;

    int j = 0;
    bool mismatch = false;
    for (; kw[j]; j++) {
      if (lexer->lookahead != (int32_t)kw[j]) { mismatch = true; break; }
      lexer->advance(lexer, false);
    }
    if (mismatch) {
      while (is_ident_char(lexer->lookahead)) lexer->advance(lexer, false);
      return;
    }
    if (is_ident_char(lexer->lookahead)) {
      while (is_ident_char(lexer->lookahead)) lexer->advance(lexer, false);
      return;
    }
    while (is_ws(lexer->lookahead)) lexer->advance(lexer, false);
    scan_ts_type_atom(lexer);
    return;
  }

  /* Generic function type:  <T>(params) => ReturnType */
  if (lexer->lookahead == '<') {
    consume_balanced(lexer, '<', '>');
    while (is_ws(lexer->lookahead)) lexer->advance(lexer, false);
    if (lexer->lookahead == '(') {
      consume_balanced(lexer, '(', ')');
      while (is_ws(lexer->lookahead)) lexer->advance(lexer, false);
      if (lexer->lookahead == '=') {
        lexer->advance(lexer, false);
        if (lexer->lookahead == '>') {
          lexer->advance(lexer, false);
          while (is_ws(lexer->lookahead)) lexer->advance(lexer, false);
          scan_ts_type_atom(lexer);
        }
      }
    }
    return;
  }

  /* Parenthesised type or function parameter list: (params) => ReturnType */
  if (lexer->lookahead == '(') {
    consume_balanced(lexer, '(', ')');
    while (is_ws(lexer->lookahead)) lexer->advance(lexer, false);
    if (lexer->lookahead == '=') {
      lexer->advance(lexer, false);
      if (lexer->lookahead == '>') {
        lexer->advance(lexer, false);
        while (is_ws(lexer->lookahead)) lexer->advance(lexer, false);
        scan_ts_type_atom(lexer);
      }
    }
    return;
  }

  /* Identifier-based type: optionally dotted, with optional generic args. */
  if (is_ident_char(lexer->lookahead) && lexer->lookahead != '.') {
    while (is_ident_char(lexer->lookahead)) lexer->advance(lexer, false);
    if (lexer->lookahead == '<') {
      consume_balanced(lexer, '<', '>');
    }
    while (lexer->lookahead == '[') {
      lexer->advance(lexer, false);
      while (is_ws(lexer->lookahead)) lexer->advance(lexer, false);
      if (lexer->lookahead == ']') lexer->advance(lexer, false);
    }
  }
}

static bool scan_ts_type_expression(TSLexer *lexer) {
  if (lexer->lookahead == 0) return false;

  int32_t before = lexer->lookahead;
  scan_ts_type_atom(lexer);
  if (lexer->lookahead == before && !is_ws(before)) return false;

  lexer->mark_end(lexer);

  /* Check for generic args with space before '<': `Array< T >` */
  {
    bool saw_ws = false;
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
      lexer->advance(lexer, false);
      saw_ws = true;
    }
    if (saw_ws && lexer->lookahead == '<') {
      consume_balanced(lexer, '<', '>');
      while (lexer->lookahead == '[') {
        lexer->advance(lexer, false);
        while (is_ws(lexer->lookahead)) lexer->advance(lexer, false);
        if (lexer->lookahead == ']') lexer->advance(lexer, false);
      }
      lexer->mark_end(lexer);
    }
  }

  /* Intersection (&) and union (|) continuations. */
  for (;;) {
    while (is_ws(lexer->lookahead)) lexer->advance(lexer, false);

    if (lexer->lookahead == '&') {
      lexer->advance(lexer, false);
      if (lexer->lookahead == '&') break;
      while (is_ws(lexer->lookahead)) lexer->advance(lexer, false);
      scan_ts_type_atom(lexer);
      lexer->mark_end(lexer);
      {
        bool saw_ws = false;
        while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
          lexer->advance(lexer, false);
          saw_ws = true;
        }
        if (saw_ws && lexer->lookahead == '<') {
          consume_balanced(lexer, '<', '>');
          lexer->mark_end(lexer);
        }
      }
    } else if (lexer->lookahead == '|') {
      lexer->advance(lexer, false);
      if (lexer->lookahead == '|') break;
      while (is_ws(lexer->lookahead)) lexer->advance(lexer, false);
      scan_ts_type_atom(lexer);
      lexer->mark_end(lexer);
    } else {
      break;
    }
  }

  return true;
}

/*
 * Scan a full TypeScript type-assertion expression: <atom> 'as'|'satisfies' <type>
 */
static bool scan_ts_attr_expression_value(TSLexer *lexer) {
  if (!is_ident_char(lexer->lookahead) || lexer->lookahead == '.') return false;

  while (is_ident_char(lexer->lookahead)) lexer->advance(lexer, false);

  if (lexer->lookahead != ' ' && lexer->lookahead != '\t') return false;
  while (lexer->lookahead == ' ' || lexer->lookahead == '\t')
    lexer->advance(lexer, false);

  bool keyword_found = false;
  if (lexer->lookahead == 'a') {
    lexer->advance(lexer, false);
    if (lexer->lookahead == 's') {
      lexer->advance(lexer, false);
      if (!is_ident_char(lexer->lookahead)) keyword_found = true;
      else return false;
    } else return false;
  }
  else if (lexer->lookahead == 's') {
    static const char *rest = "atisfies";
    lexer->advance(lexer, false);
    for (const char *p = rest; *p; p++) {
      if (lexer->lookahead != (int32_t)*p) return false;
      lexer->advance(lexer, false);
    }
    if (!is_ident_char(lexer->lookahead)) keyword_found = true;
    else return false;
  }

  if (!keyword_found) return false;

  while (is_ws(lexer->lookahead)) lexer->advance(lexer, false);

  if (!scan_ts_type_expression(lexer)) return false;

  return true;
}

/* ── TAG_VAR_FRAGMENT scanner ─────────────────────────────────────────────── */

/*
 * Returns true if `c` is a "hard stop" character when encountered as the next
 * non-space character during lookahead (used by the space-peeking logic).
 */
static bool is_tag_var_peek_stop(int32_t c) {
  return c == '=' || c == '|' || c == '(' || c == ')' ||
         c == '{' || c == '}' || c == '>' || c == '/' ||
         c == ':' || c == '\n' || c == '\r' || c == 0;
}

static bool scan_tag_var_fragment(TSLexer *lexer) {
  /* First char: must not be a stop char (includes / and - as new stops) */
  {
    int32_t c = lexer->lookahead;
    if (c == 0 || c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
        c == '=' || c == '|' || c == ')' ||
        c == '{' || c == '}' || c == '>' ||
        c == '/' || c == '-') return false;
  }

  int depth_paren = 0; /* tracks ( ) nesting in type annotations  */
  bool consumed = false;

  for (;;) {
    int32_t c = lexer->lookahead;

    if (c == 0) break;

    /* '<' is now a hard stop so that tag_type_args/_params can consume it */
    if (c == '<') break;

    /* '>' is a hard stop at depth 0 (closes the tag) */
    if (c == '>') break;

    /* ── paren tracking (type annotations like a:(B | C)) ── */
    if (c == '(') {
      depth_paren++;
      lexer->advance(lexer, false);
      consumed = true;
      continue;
    }
    if (c == ')') {
      if (depth_paren == 0) break;
      depth_paren--;
      lexer->advance(lexer, false);
      consumed = true;
      if (depth_paren == 0) lexer->mark_end(lexer);
      continue;
    }

    /* ── inside parens: consume everything freely ── */
    if (depth_paren > 0) {
      lexer->advance(lexer, false);
      consumed = true;
      continue;
    }

    /* ── at depth 0: hard stops ── */
    if (c == '/' || c == '}' || c == '{' || c == '|' || c == '=') break;
    if (c == '\n' || c == '\r') break;

    /* ── space handling: peek-ahead for arithmetic/expression operators ── */
    if (c == ' ' || c == '\t') {
      /*
       * Consume the whitespace tentatively (without updating mark_end).
       * Only continue scanning (include the space) if the next non-space char
       * is an arithmetic/expression operator like + - * % & ? !.  For all
       * other next chars (identifier, :, =, punctuation, ...) the space acts
       * as a separator and we stop, leaving the remaining text for the grammar.
       *
       * Special case: if the operator char is immediately followed by '=',
       * it is an assignment operator (+=, -=, …) which terminates the var.
       */
      while (lexer->lookahead == ' ' || lexer->lookahead == '\t')
        lexer->advance(lexer, false);

      int32_t c1 = lexer->lookahead;

      /* Only continue for binary arithmetic / expression operators */
      if (c1 == '+' || c1 == '-' || c1 == '*' || c1 == '%' ||
          c1 == '&' || c1 == '?' || c1 == '!') {
        /* Advance c1 tentatively and check c2 */
        lexer->advance(lexer, false);
        int32_t c2 = lexer->lookahead;

        if (c2 == '=') {
          /* operator= (+=, -=, …) terminates the fragment */
          break;
        }
        if (c1 == '-' && c2 == '-') {
          /* Marko concise fence marker '--' terminates the fragment */
          break;
        }

        /* Include space + operator in the fragment */
        lexer->mark_end(lexer);
        consumed = true;
        /* c1 already consumed; main loop sees c2 next */
        continue;
      }

      /* Default: space is a separator – stop */
      break;
    }

    /* ── regular character ── */
    lexer->advance(lexer, false);
    consumed = true;
    lexer->mark_end(lexer);
  }

  return consumed;
}

/* ── TAG_TYPE_ARGS_FRAGMENT scanner ──────────────────────────────────────── */
/*
 * Scans the content between the opening '<' (already consumed by the grammar
 * rule) and the closing '>' of tag type arguments / parameters / attribute
 * type parameters.  Handles:
 *   - Nested angle brackets:  A extends B<C>
 *   - Braces with '>':         { bar: () => void }
 *   - Parentheses:             (arg: A) => B
 *   - Stops at '>' at angle-bracket depth 0.
 *   - Returns false for empty content (so the fragment node is optional).
 */
static bool scan_tag_type_args_fragment(TSLexer *lexer) {
  if (lexer->lookahead == '>' || lexer->lookahead == 0) return false;
  /* Only start scanning if the first char looks like a TypeScript type expression.
   * Reject operators, punctuation, and other chars that can't start a type. */
  {
    int32_t c = lexer->lookahead;
    /* Allow: letters, digits, _, $, @, {, (, [, ? (conditional), | (union start?) */
    bool is_type_start =
      (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
      (c >= '0' && c <= '9') ||
      c == '_' || c == '$' || c == '@' ||
      c == '{' || c == '(' || c == '[' ||
      c == '"' || c == '\'' || c == '`' || /* string literal types */
      c == '?' || c == '!' || /* conditional / not */
      c == ' ' || c == '\t' || c == '\n'; /* whitespace is OK (empty type args) */
    if (!is_type_start) return false;
  }

  int depth_angle = 0, depth_paren = 0, depth_brace = 0;
  bool consumed = false;

  for (;;) {
    int32_t c = lexer->lookahead;
    if (c == 0) break;

    if (c == '<') {
      depth_angle++;
      lexer->advance(lexer, false);
      consumed = true;
      continue;
    }
    if (c == '>') {
      if (depth_angle == 0 && depth_paren == 0 && depth_brace == 0) break;
      if (depth_angle > 0) depth_angle--;
      lexer->advance(lexer, false);
      consumed = true;
      if (depth_angle == 0 && depth_paren == 0 && depth_brace == 0)
        lexer->mark_end(lexer);
      continue;
    }
    if (c == '(') {
      depth_paren++;
      lexer->advance(lexer, false);
      consumed = true;
      continue;
    }
    if (c == ')') {
      if (depth_paren > 0) depth_paren--;
      lexer->advance(lexer, false);
      consumed = true;
      if (depth_angle == 0 && depth_paren == 0 && depth_brace == 0)
        lexer->mark_end(lexer);
      continue;
    }
    if (c == '{') {
      depth_brace++;
      lexer->advance(lexer, false);
      consumed = true;
      continue;
    }
    if (c == '}') {
      if (depth_brace > 0) depth_brace--;
      lexer->advance(lexer, false);
      consumed = true;
      if (depth_angle == 0 && depth_paren == 0 && depth_brace == 0)
        lexer->mark_end(lexer);
      continue;
    }

    /* All other characters (including whitespace, &, |, ,, =, /, etc.) */
    lexer->advance(lexer, false);
    consumed = true;
    if (depth_angle == 0 && depth_paren == 0 && depth_brace == 0)
      lexer->mark_end(lexer);
  }

  return consumed;
}

/* ── main scan entry point ────────────────────────────────────────────────── */

bool tree_sitter_marko_external_scanner_scan(
    void *payload, TSLexer *lexer, const bool *valid_symbols) {
  Scanner *s = payload;

  if (valid_symbols[GT_AFTER_IMPLICIT] && s->after_implicit) {
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t')
      lexer->advance(lexer, true);
    if (lexer->lookahead == '>') {
      lexer->advance(lexer, false);
      lexer->mark_end(lexer);
      s->after_implicit = false;
      lexer->result_symbol = GT_AFTER_IMPLICIT;
      return true;
    }
  }

  if (valid_symbols[TS_ATTR_EXPRESSION_VALUE]) {
    if (scan_ts_attr_expression_value(lexer)) {
      lexer->result_symbol = TS_ATTR_EXPRESSION_VALUE;
      return true;
    }
  }

  if (valid_symbols[TAG_VAR_FRAGMENT]) {
    lexer->mark_end(lexer);
    if (scan_tag_var_fragment(lexer)) {
      lexer->result_symbol = TAG_VAR_FRAGMENT;
      return true;
    }
  }

  /*
   * TAG_TYPE_ARGS_LT: consumes '<' immediately (no preceding whitespace).
   * Placed BEFORE IMPLICIT_CLOSE so it takes priority when both are valid —
   * this prevents _implicit_close from ending open_element before tag_type_args
   * can consume its opening '<'.
   * Only fires if there is no whitespace between the last token and '<'
   * (enforced by checking that the lookahead is '<' with no preceding extras).
   */
  if (valid_symbols[TAG_TYPE_ARGS_LT]) {
    if (lexer->lookahead == '<') {
      lexer->advance(lexer, false);
      lexer->mark_end(lexer);
      s->after_tag_type_args_lt = true;
      lexer->result_symbol = TAG_TYPE_ARGS_LT;
      return true;
    }
  }

  if (valid_symbols[TAG_TYPE_ARGS_FRAGMENT]) {
    /* Only fire if the immediately preceding external token was _tag_type_args_lt. */
    if (s->after_tag_type_args_lt) {
      s->after_tag_type_args_lt = false;
      lexer->mark_end(lexer);
      if (scan_tag_type_args_fragment(lexer)) {
        lexer->result_symbol = TAG_TYPE_ARGS_FRAGMENT;
        return true;
      }
    }
  }

  /*
   * TAG_TYPE_PARAMS_LT: consumes '<' that MAY be preceded by whitespace.
   * Fires when '<' appears after optional spaces/tabs — this distinguishes
   * tag_type_params (space allowed) from tag_type_args (no space).
   * Also placed BEFORE IMPLICIT_CLOSE to prevent open_element from firing.
   * We only fire if the lookahead is currently whitespace or '<'.
   */
  if (valid_symbols[TAG_TYPE_PARAMS_LT]) {
    /* Skip any leading whitespace (advance with skip=true so they become extras) */
    bool had_space = false;
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
      lexer->advance(lexer, true); /* mark as extra (skip) */
      had_space = true;
    }
    if (lexer->lookahead == '<') {
      if (had_space) {
        /* Space was present before '<': this is a tag_type_params context */
        lexer->advance(lexer, false);
        lexer->mark_end(lexer);
        s->after_tag_type_params_lt = true;
        lexer->result_symbol = TAG_TYPE_PARAMS_LT;
        return true;
      }
      /* No space: this might be tag_type_args, but TAG_TYPE_ARGS_LT would have
       * fired earlier. If we reach here, TAG_TYPE_ARGS_LT was not valid in this
       * state, so fire TAG_TYPE_PARAMS_LT for the direct '<' case too. */
      lexer->advance(lexer, false);
      lexer->mark_end(lexer);
      s->after_tag_type_params_lt = true;
      lexer->result_symbol = TAG_TYPE_PARAMS_LT;
      return true;
    }
  }

  if (valid_symbols[TAG_TYPE_PARAMS_FRAGMENT]) {
    /* Only fire if the immediately preceding external token was _tag_type_params_lt. */
    if (s->after_tag_type_params_lt) {
      s->after_tag_type_params_lt = false;
      lexer->mark_end(lexer);
      if (scan_tag_type_args_fragment(lexer)) { /* reuse same fragment scanner */
        lexer->result_symbol = TAG_TYPE_PARAMS_FRAGMENT;
        return true;
      }
    }
  }

  /* Any other token resets the type-lt flags */
  s->after_tag_type_args_lt = false;
  s->after_tag_type_params_lt = false;

  if (valid_symbols[IMPLICIT_CLOSE]) {
    lexer->mark_end(lexer);
    if (lexer->lookahead == '<') {
      s->after_implicit = true;
      lexer->result_symbol = IMPLICIT_CLOSE;
      return true;
    }
  }

  return false;
}
