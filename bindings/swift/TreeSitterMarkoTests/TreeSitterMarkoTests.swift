import XCTest
import SwiftTreeSitter
import TreeSitterMarko

final class TreeSitterMarkoTests: XCTestCase {
    func testCanLoadGrammar() throws {
        let parser = Parser()
        let language = Language(language: tree_sitter_marko())
        XCTAssertNoThrow(try parser.setLanguage(language),
                         "Error loading Marko grammar")
    }
}
