# Cursive Language Server

`Cursive_LSP` is the reusable Cursive language server. It speaks LSP over
standard input/output by default, so any editor that can launch a stdio language
server can use the same binary.

## Build

```powershell
cd C:\Dev\Cursive\cursive
cmake --build --preset windows-debug --target cursive-lsp
```

Default Windows debug binary:

```text
C:\Dev\Cursive\cursive\build\windows\Debug\Cursive_LSP.exe
```

## Server Options

```text
Cursive_LSP [--stdio] [--log-file <path>] [--target-profile <profile>]
Cursive_LSP --version
```

- `--stdio` runs LSP over stdin/stdout. This is the default.
- `--version` prints the server version and exits.
- `--log-file <path>` writes server lifecycle and project-analysis logs.
- `--target-profile <profile>` selects the target profile when the manifest does
  not set `[toolchain].target_profile`. Supported values match the compiler:
  `x86_64-sysv`, `x86_64-win64`, and `aarch64-aapcs64`.

## Editor Configuration

Use the same command shape in every editor:

```text
Cursive_LSP --stdio --target-profile x86_64-win64
```

Open a folder that contains a `Cursive.toml` manifest. The server also discovers
the nearest parent `Cursive.toml` for each opened `.cursive` file, so nested
projects and multi-root editor sessions can work as long as each source file is
under a manifest root.

If no manifest is found, the server publishes a diagnostic on the opened file
instead of silently doing nothing.

If neither `--target-profile` nor `[toolchain].target_profile` is set, the
server publishes the same `E-PRJ-0112` diagnostic as the compiler.

### Neovim

```lua
vim.filetype.add({
  extension = { cursive = "cursive" },
})

vim.api.nvim_create_autocmd("FileType", {
  pattern = "cursive",
  callback = function()
    local manifest = vim.fs.find({ "Cursive.toml" }, { upward = true })[1]
    local root = manifest and vim.fs.dirname(manifest) or vim.fn.getcwd()
    vim.lsp.start({
      name = "cursive-lsp",
      cmd = {
        "C:/Dev/Cursive/cursive/build/windows/Debug/Cursive_LSP.exe",
        "--stdio",
        "--target-profile",
        "x86_64-win64",
      },
      root_dir = root,
    })
  end,
})
```

### Helix

```toml
[language-server.cursive-lsp]
command = "C:/Dev/Cursive/cursive/build/windows/Debug/Cursive_LSP.exe"
args = ["--stdio", "--target-profile", "x86_64-win64"]

[[language]]
name = "cursive"
scope = "source.cursive"
file-types = ["cursive"]
language-servers = ["cursive-lsp"]
```

## Current Capabilities

- diagnostics
- hover
- go to definition
- document symbols
- workspace symbols
- document highlights
- references
- completion
- quick-fix code action for invalid `main` signature
- semantic tokens

Diagnostics, hover, definition, references, document highlights, semantic token
classification, and compiler-authored quick fixes are backed by the normal
compiler project loading, parsing, compile-time expansion, resolution, and
typechecking pipeline. The LSP layer adapts the compiler snapshot to protocol
responses; it is not a separate parser or name resolver.

Completion currently combines Cursive keywords with symbols from the compiler
language-service index. Rename and formatting are intentionally not advertised
yet.

## Validate

```powershell
cd C:\Dev\Cursive
python tests/lsp/test_initialize.py
python tests/lsp/test_valid_diagnostics.py
python tests/lsp/test_diagnostics.py
python tests/lsp/test_navigation.py
python tests/lsp/test_features.py
python tests/lsp/test_manifest_root.py
python tests/lsp/test_no_manifest.py
python tests/lsp/test_code_action.py
python tests/lsp/test_qualified_navigation.py
python tests/lsp/test_semantic_diagnostics.py
python tests/lsp/test_version.py
```
