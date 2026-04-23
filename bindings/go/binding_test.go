package tree_sitter_marko_test

import (
	"testing"

	tree_sitter "github.com/tree-sitter/go-tree-sitter"
	tree_sitter_marko "github.com/tree-sitter/tree-sitter-marko/bindings/go"
)

func TestCanLoadGrammar(t *testing.T) {
	language := tree_sitter.NewLanguage(tree_sitter_marko.Language())
	if language == nil {
		t.Errorf("Error loading Marko grammar")
	}
}
