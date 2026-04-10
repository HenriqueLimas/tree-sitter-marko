#include "tree_sitter/parser.h"

/*
 * External scanner for tree-sitter-marko.
 *
 * Two external tokens work together so that open_element (a start_tag that
 * closes without an explicit '>') and the subsequent "bare" start_tags at
 * document level interact correctly without causing ERROR nodes:
 *
 *  IMPLICIT_CLOSE  — zero-width token emitted when the next significant char
 *                    is '<' (or EOF).  Ends an open_element rule.
 *
 *  GT_AFTER_IMPLICIT — emitted for the '>' character but ONLY when a
 *                      preceding IMPLICIT_CLOSE was produced.  This token is
 *                      used by start_tag_doc (a document-level start_tag
 *                      alias) instead of the regular '>'.  Because
 *                      normal_element's start_tag uses the regular '>' and
 *                      not this token, normal_element cannot match <T>; it
 *                      fails cleanly, leaving start_tag_doc as the winner.
 */

enum TokenType {
  IMPLICIT_CLOSE,
  GT_AFTER_IMPLICIT,
};

typedef struct {
  bool after_implicit; /* true after IMPLICIT_CLOSE was emitted */
} Scanner;

void *tree_sitter_marko_external_scanner_create() {
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

bool tree_sitter_marko_external_scanner_scan(
    void *payload, TSLexer *lexer, const bool *valid_symbols) {
  Scanner *s = payload;

  /* GT_AFTER_IMPLICIT: consume '>' only when we are in after_implicit mode.
   * This is what start_tag_doc uses instead of the regular '>'. */
  if (valid_symbols[GT_AFTER_IMPLICIT] && s->after_implicit) {
    /* Skip spaces/tabs that precede the '>'. */
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
      lexer->advance(lexer, true);
    }
    if (lexer->lookahead == '>') {
      lexer->advance(lexer, false);
      lexer->mark_end(lexer);
      s->after_implicit = false;
      lexer->result_symbol = GT_AFTER_IMPLICIT;
      return true;
    }
  }

  /* IMPLICIT_CLOSE: zero-width, emitted ONLY when '<' is the VERY NEXT
   * character (no whitespace skipping).  This ensures we fire only when
   * another HTML tag begins immediately (e.g. Array<T>), not when there is
   * a space before '<' (e.g. `<foo <A>` is invalid and must stay ERROR).
   * We intentionally do NOT check for EOF here — a tag at EOF should error
   * out via normal recovery, not produce a silent implicit close. */
  if (valid_symbols[IMPLICIT_CLOSE]) {
    if (lexer->lookahead == '<') {
      lexer->mark_end(lexer); /* zero-width: do not advance past '<' */
      s->after_implicit = true;
      lexer->result_symbol = IMPLICIT_CLOSE;
      return true;
    }
  }

  return false;
}
