#include <tree_sitter/parser.h>
#include <ctype.h>

enum TokenType {
  EOF_TAG_END,
};

void *tree_sitter_marko_external_scanner_create(void) { return NULL; }
void tree_sitter_marko_external_scanner_destroy(void *p) { (void)p; }
void tree_sitter_marko_external_scanner_reset(void *p) { (void)p; }
unsigned tree_sitter_marko_external_scanner_serialize(void *p, char *buffer) {
  (void)p;
  (void)buffer;
  return 0;
}
void tree_sitter_marko_external_scanner_deserialize(void *p, const char *b, unsigned n) {
  (void)p;
  (void)b;
  (void)n;
}

bool tree_sitter_marko_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
  (void)payload;
  if (!valid_symbols[EOF_TAG_END]) return false;

  while (lexer->lookahead && isspace((unsigned char)lexer->lookahead)) {
    lexer->advance(lexer, true);
  }

  if (lexer->lookahead == 0) {
    lexer->result_symbol = EOF_TAG_END;
    return true;
  }

  return false;
}
