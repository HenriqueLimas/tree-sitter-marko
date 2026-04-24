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
};

typedef struct {
  bool after_implicit;
} Scanner;

void *tree_sitter_marko_external_scanner_create(void) {
  Scanner *s = malloc(sizeof(Scanner));
  s->after_implicit = false;
  return s;
}

void tree_sitter_marko_external_scanner_destroy(void *payload) {
  free(payload);
}

void tree_sitter_marko_external_scanner_reset(void *payload) {
  ((Scanner *)payload)->after_implicit = false;
}

unsigned tree_sitter_marko_external_scanner_serialize(void *payload, char *buffer) {
  Scanner *s = payload;
  buffer[0] = (char)s->after_implicit;
  return 1;
}

void tree_sitter_marko_external_scanner_deserialize(
    void *payload, const char *buffer, unsigned length) {
  Scanner *s = payload;
  s->after_implicit = length > 0 && buffer[0];
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

static bool scan_tag_var_fragment(TSLexer *lexer) {
  {
    int32_t c = lexer->lookahead;
    if (c == 0 || c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
        c == '=' || c == '|' || c == '(' || c == ')' ||
        c == '{' || c == '}' || c == '>') return false;
  }

  int depth = 0;
  bool consumed = false;

  for (;;) {
    int32_t c = lexer->lookahead;

    if (c == 0) break;

    if (c == '<') {
      depth++;
      lexer->advance(lexer, false);
      consumed = true;
      continue;
    }

    if (c == '>') {
      if (depth == 0) break;
      depth--;
      lexer->advance(lexer, false);
      consumed = true;
      if (depth == 0) {
        lexer->mark_end(lexer);
        break;
      }
      continue;
    }

    if (depth > 0 && is_ws(c)) {
      lexer->advance(lexer, false);
      consumed = true;
      continue;
    }

    if (depth == 0 &&
        (c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
         c == '=' || c == '|' || c == '(' || c == ')' ||
         c == '{' || c == '}' || c == '>')) break;

    lexer->advance(lexer, false);
    consumed = true;
    if (depth == 0) lexer->mark_end(lexer);
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
