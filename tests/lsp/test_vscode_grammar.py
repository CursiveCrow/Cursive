import json
import pathlib


REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]


if __name__ == "__main__":
    extension_root = REPO_ROOT / "editors" / "vscode-cursive"
    package = json.loads((extension_root / "package.json").read_text(encoding="utf-8"))
    contributes = package["contributes"]
    grammars = contributes["grammars"]
    cursive_grammar = next(
        grammar for grammar in grammars if grammar["language"] == "cursive"
    )
    assert cursive_grammar["scopeName"] == "source.cursive"
    grammar_path = extension_root / cursive_grammar["path"]
    grammar = json.loads(grammar_path.read_text(encoding="utf-8"))
    assert grammar["scopeName"] == "source.cursive"
    assert grammar["patterns"]
    assert contributes["configurationDefaults"]["[cursive]"][
        "editor.semanticHighlighting.enabled"
    ] is True
    semantic_scopes = contributes["semanticTokenScopes"]
    cursive_scopes = next(
        scope_map for scope_map in semantic_scopes if scope_map["language"] == "cursive"
    )
    assert cursive_scopes["scopes"]["function"] == [
        "entity.name.function.cursive"
    ]
    assert cursive_scopes["scopes"]["variable.readonly"] == [
        "variable.other.constant.cursive"
    ]
