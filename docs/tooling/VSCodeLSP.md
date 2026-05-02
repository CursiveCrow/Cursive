# Cursive LSP In VS Code

The Cursive language server is a stdio LSP server. VS Code needs the local
client extension in `editors/vscode-cursive` to launch it.

## Build The Server

```powershell
cd C:\Dev\Cursive\cursive
cmake --build --preset windows-debug --target cursive-lsp
```

The default extension configuration expects:

```text
C:\Dev\Cursive\cursive\build\windows\Debug\Cursive_LSP.exe
```

Set `cursive.lsp.path` in VS Code settings if the server is somewhere else.
Set `cursive.lsp.logFile` to pass `--log-file <path>` to the server.
Set `cursive.lsp.targetProfile` to pass `--target-profile <profile>` when the
project manifest does not set `[toolchain].target_profile`.

## Build The VS Code Client

```powershell
cd C:\Dev\Cursive\editors\vscode-cursive
npm install
npm run compile
```

## Launch VS Code With The Local Extension

Open a folder that contains a `Cursive.toml` manifest. For the smoke-test
fixture:

```powershell
code C:\Dev\Cursive\tests\lsp\fixtures\valid_project --extensionDevelopmentPath=C:\Dev\Cursive\editors\vscode-cursive
```

For a real project, replace the fixture path with that project's manifest root.

## Validate

First verify the server protocol tests:

```powershell
cd C:\Dev\Cursive
python tests/lsp/test_initialize.py
python tests/lsp/test_diagnostics.py
python tests/lsp/test_navigation.py
python tests/lsp/test_semantic_diagnostics.py
```

Then open:

```text
src/Main.cursive
```

Expected VS Code behavior:

- the file language mode is `Cursive`;
- hover on `helper(1)` shows `ValidProject::helper`;
- go to definition on `helper(1)` jumps to the `helper` procedure;
- document symbols include `helper`, `main`, and `value`;
- references and document highlights on `helper(1)` include the declaration and
  call site from the compiler language-service index;
- completion includes Cursive keywords and visible project symbols;
- workspace symbol search can find `helper`;
- semantic token data is available to VS Code.

Open the syntax-error fixture to check diagnostics:

```powershell
code C:\Dev\Cursive\tests\lsp\fixtures\syntax_error_project --extensionDevelopmentPath=C:\Dev\Cursive\editors\vscode-cursive
```

```text
src/Main.cursive
```

VS Code should report a diagnostic for the incomplete `let value: i32 =` line.

Open the invalid-main fixture to check compiler-authored quick fixes:

```powershell
code C:\Dev\Cursive\tests\lsp\fixtures\invalid_main_project --extensionDevelopmentPath=C:\Dev\Cursive\editors\vscode-cursive
```

VS Code should report `E-MOD-2431` and offer `Fix main signature` from the
diagnostic's compiler fix-it metadata.

Use `Output: Cursive Language Server` and `Developer: Toggle Developer Tools`
for client startup or protocol errors.
