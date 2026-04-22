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

  /* IMPLICIT_CLOSE: zero-width token that ends an open_element.
   *
   * Case 1 — Next char is '<' (no whitespace skip):
   *   Fires for constructs like Array<T> where the type-arg '<' immediately
   *   follows the attribute text.  Whitespace before '<' is intentionally NOT
   *   skipped so that `<foo <A>` (a space before '<') stays an ERROR.
   *   Sets after_implicit=true so the following start_tag_doc can use
   *   GT_AFTER_IMPLICIT instead of the plain '>'.
   *
   * Case 2 — Next non-whitespace chars are '=>' (TS function return type):
   *   Fires for constructs like `as (x: T) => ReturnType` where the `=>`
   *   signals a TypeScript arrow-function type.  The zero-width token lands
   *   BEFORE any leading whitespace, so `=> ReturnType />` is left as
   *   document-level text rather than producing an ERROR.
   *   Sets after_implicit=false (no GT_AFTER_IMPLICIT needed here — the
   *   content after this point is plain text, not a start_tag_doc).
   *
   * Case 3 — TS intersection-type `& identifier` sequences followed by '<':
   *   Fires for constructs like `as A & B<T>` where `& B` is a TypeScript
   *   intersection-type continuation.  The token is NON-zero-width: it
   *   consumes the leading whitespace plus `& B ` chars so they are hidden
   *   from the parse tree.  Only single `&` (not `&&`) is handled; `&&` is
   *   not an intersection operator and must NOT trigger implicit close.
   *   Sets after_implicit=true so the following `<T>` is a start_tag_doc.
   *
   * We do not check for EOF — a tag at EOF errors out via normal recovery. */
  if (valid_symbols[IMPLICIT_CLOSE]) {
    /* Mark end FIRST so the token is zero-width from this position by
     * default.  For case 3 we update mark_end later to consume the skipped
     * `& identifier` text. */
    lexer->mark_end(lexer);

    if (lexer->lookahead == '<') {
      s->after_implicit = true;
      lexer->result_symbol = IMPLICIT_CLOSE;
      return true;
    }

    /* Skip ONE OR MORE whitespace chars. */
    bool skipped_ws = false;
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
      lexer->advance(lexer, false);
      skipped_ws = true;
    }
    if (!skipped_ws) return false;

    /* Case 3: TS intersection `& identifier` sequences followed by '<'.
     * After consuming `& B ` (non-zero-width), the subsequent `<T>` can be
     * parsed as a start_tag_doc, matching the expected tree structure for
     * inputs like `as A & B<T>`. */
    if (lexer->lookahead == '&') {
      for (;;) {
        /* Consume the '&'. */
        lexer->advance(lexer, false);
        /* A second '&' means logical-AND (&&), not intersection — bail out.
         * The scanner position is reset to zero on false return. */
        if (lexer->lookahead == '&') return false;
        /* Require at least one space after '&'. */
        if (lexer->lookahead != ' ' && lexer->lookahead != '\t') return false;
        while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
          lexer->advance(lexer, false);
        }
        /* Consume the identifier after '& ' (must start with a letter/$ /_). */
        bool has_ident = false;
        while ((lexer->lookahead >= 'A' && lexer->lookahead <= 'Z') ||
               (lexer->lookahead >= 'a' && lexer->lookahead <= 'z') ||
               (lexer->lookahead >= '0' && lexer->lookahead <= '9') ||
               lexer->lookahead == '_' || lexer->lookahead == '$' ||
               lexer->lookahead == '-') {
          lexer->advance(lexer, false);
          has_ident = true;
        }
        if (!has_ident) return false;
        /* Skip trailing whitespace. */
        while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
          lexer->advance(lexer, false);
        }
        if (lexer->lookahead == '<') {
          /* Found `& ident <`: update mark_end (now non-zero-width) and fire. */
          lexer->mark_end(lexer);
          s->after_implicit = true;
          lexer->result_symbol = IMPLICIT_CLOSE;
          return true;
        }
        /* Allow chained intersections: `A & B & C<T>`. */
        if (lexer->lookahead != '&') return false;
      }
    }

    /* Case 2: TS arrow `=>` — requires whitespace before (already checked). */
    if (lexer->lookahead == '=') {
      lexer->advance(lexer, false);
      if (lexer->lookahead == '>') {
        s->after_implicit = false; /* no GT_AFTER_IMPLICIT needed */
        lexer->result_symbol = IMPLICIT_CLOSE;
        return true;
      }
    }
  }

  return false;
}
