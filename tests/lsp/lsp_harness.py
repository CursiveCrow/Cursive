import json
import os
import pathlib
import subprocess
import sys
import time


REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]


def default_server_path():
    env_path = os.environ.get("CURSIVE_LSP")
    if env_path:
        return pathlib.Path(env_path)
    candidates = [
        REPO_ROOT / "cursive" / "build" / "windows" / "Debug" / "Cursive_LSP.exe",
        REPO_ROOT / "cursive" / "build" / "windows" / "Cursive_LSP.exe",
        REPO_ROOT / "cursive" / "build" / "linux" / "Cursive_LSP",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise RuntimeError("Cursive_LSP executable not found; set CURSIVE_LSP")


class LspClient:
    def __init__(self, root):
        self.root = pathlib.Path(root)
        self.proc = subprocess.Popen(
            [str(default_server_path()), "--stdio"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=str(REPO_ROOT),
        )
        self.next_id = 1

    def close(self):
        if self.proc.poll() is None:
            try:
                self.notify("exit", {})
            except Exception:
                pass
            try:
                self.proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.proc.kill()

    def send(self, payload):
        body = json.dumps(payload, separators=(",", ":")).encode("utf-8")
        header = f"Content-Length: {len(body)}\r\n\r\n".encode("ascii")
        self.proc.stdin.write(header + body)
        self.proc.stdin.flush()

    def request(self, method, params):
        request_id = self.next_id
        self.next_id += 1
        self.send({"jsonrpc": "2.0", "id": request_id, "method": method, "params": params})
        while True:
            message = self.read_message()
            if message.get("id") == request_id:
                return message

    def notify(self, method, params):
        self.send({"jsonrpc": "2.0", "method": method, "params": params})

    def read_message(self):
        headers = {}
        while True:
            line = self.proc.stdout.readline()
            if not line:
                stderr = self.proc.stderr.read().decode("utf-8", errors="replace")
                raise RuntimeError(f"LSP server closed stdout. stderr:\n{stderr}")
            line = line.decode("ascii").strip()
            if not line:
                break
            name, value = line.split(":", 1)
            headers[name.lower()] = value.strip()
        length = int(headers["content-length"])
        body = self.proc.stdout.read(length)
        return json.loads(body.decode("utf-8"))

    def initialize(self):
        response = self.request(
            "initialize",
            {
                "processId": os.getpid(),
                "rootUri": self.root.as_uri(),
                "capabilities": {},
            },
        )
        self.notify("initialized", {})
        return response

    def shutdown(self):
        response = self.request("shutdown", {})
        self.notify("exit", {})
        self.proc.wait(timeout=5)
        return response

    def open_file(self, path):
        path = pathlib.Path(path)
        self.notify(
            "textDocument/didOpen",
            {
                "textDocument": {
                    "uri": path.as_uri(),
                    "languageId": "cursive",
                    "version": 1,
                    "text": path.read_text(encoding="utf-8"),
                }
            },
        )

    def wait_for_diagnostics(self, uri, timeout=10):
        deadline = time.time() + timeout
        while time.time() < deadline:
            message = self.read_message()
            if message.get("method") != "textDocument/publishDiagnostics":
                continue
            params = message.get("params", {})
            if params.get("uri") == uri:
                return params.get("diagnostics", [])
        raise TimeoutError(f"no diagnostics for {uri}")


def fixture(name):
    return REPO_ROOT / "tests" / "lsp" / "fixtures" / name


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: lsp_harness.py <test-name>")
    test_name = sys.argv[1]
    if test_name == "initialize":
        root = fixture("valid_project")
        client = LspClient(root)
        try:
            response = client.initialize()
            caps = response["result"]["capabilities"]
            assert caps["hoverProvider"] is True
            assert caps["definitionProvider"] is True
            assert caps["documentSymbolProvider"] is True
            assert caps["workspaceSymbolProvider"] is True
            assert caps["documentHighlightProvider"] is True
            assert caps["referencesProvider"] is True
            assert "completionProvider" in caps
            assert "codeActionProvider" in caps
            assert "semanticTokensProvider" in caps
            client.shutdown()
        finally:
            client.close()
        return
    if test_name == "diagnostics":
        root = fixture("syntax_error_project")
        path = root / "src" / "Main.cursive"
        client = LspClient(root)
        try:
            client.initialize()
            client.open_file(path)
            diagnostics = client.wait_for_diagnostics(path.as_uri())
            assert diagnostics, "expected syntax diagnostics"
            assert diagnostics[0]["source"] == "cursive"
            client.shutdown()
        finally:
            client.close()
        return
    if test_name == "valid-diagnostics":
        root = fixture("valid_project")
        path = root / "src" / "Main.cursive"
        client = LspClient(root)
        try:
            client.initialize()
            client.open_file(path)
            diagnostics = client.wait_for_diagnostics(path.as_uri())
            assert diagnostics == [], diagnostics
            uri = path.as_uri()
            symbols = client.request(
                "textDocument/documentSymbol",
                {"textDocument": {"uri": uri}},
            )["result"]
            assert [symbol["name"] for symbol in symbols] == [
                "helper",
                "main",
                "value",
            ]
            client.shutdown()
        finally:
            client.close()
        return
    if test_name == "navigation":
        root = fixture("valid_project")
        path = root / "src" / "Main.cursive"
        client = LspClient(root)
        try:
            client.initialize()
            client.open_file(path)
            uri = path.as_uri()
            symbols = client.request(
                "textDocument/documentSymbol",
                {"textDocument": {"uri": uri}},
            )["result"]
            assert [symbol["name"] for symbol in symbols] == [
                "helper",
                "main",
                "value",
            ]

            helper_position = {
                "textDocument": {"uri": uri},
                "position": {"line": 5, "character": 22},
            }
            definition = client.request(
                "textDocument/definition",
                helper_position,
            )["result"]
            assert definition["uri"] == uri
            assert definition["range"]["start"]["line"] == 0

            hover = client.request("textDocument/hover", helper_position)["result"]
            assert "ValidProject::helper" in hover["contents"]["value"]
            assert hover["range"]["start"] == {"line": 5, "character": 21}
            assert hover["range"]["end"] == {"line": 5, "character": 27}

            value_hover = client.request(
                "textDocument/hover",
                {
                    "textDocument": {"uri": uri},
                    "position": {"line": 6, "character": 12},
                },
            )["result"]
            assert "ValidProject::value" in value_hover["contents"]["value"]
            assert ": i32" in value_hover["contents"]["value"]

            semantic = client.request(
                "textDocument/semanticTokens/full",
                {"textDocument": {"uri": uri}},
            )["result"]
            assert semantic["data"], "expected semantic token data"
            client.shutdown()
        finally:
            client.close()
        return
    if test_name == "features":
        root = fixture("valid_project")
        path = root / "src" / "Main.cursive"
        client = LspClient(root)
        try:
            client.initialize()
            client.open_file(path)
            uri = path.as_uri()
            helper_position = {
                "textDocument": {"uri": uri},
                "position": {"line": 5, "character": 22},
            }

            highlights = client.request(
                "textDocument/documentHighlight",
                helper_position,
            )["result"]
            assert highlights, "expected document highlights"

            references = client.request(
                "textDocument/references",
                {**helper_position, "context": {"includeDeclaration": True}},
            )["result"]
            assert any(ref["range"]["start"]["line"] == 0 for ref in references)
            assert any(ref["range"]["start"]["line"] == 5 for ref in references)

            workspace_symbols = client.request(
                "workspace/symbol",
                {"query": "helper"},
            )["result"]
            assert any(symbol["name"] == "helper" for symbol in workspace_symbols)

            completions = client.request(
                "textDocument/completion",
                helper_position,
            )["result"]
            labels = [item["label"] for item in completions["items"]]
            assert "procedure" in labels
            assert "helper" in labels
            assert "value" in labels
            assert labels.index("value") < labels.index("helper")
            client.shutdown()
        finally:
            client.close()
        return
    if test_name == "qualified-navigation":
        root = fixture("qualified_project")
        path = root / "src" / "Main.cursive"
        client = LspClient(root)
        try:
            client.initialize()
            client.open_file(path)
            uri = path.as_uri()
            diagnostics = client.wait_for_diagnostics(uri)
            assert diagnostics == [], diagnostics

            some_position = {
                "textDocument": {"uri": uri},
                "position": {"line": 19, "character": 34},
            }
            definition = client.request(
                "textDocument/definition",
                some_position,
            )["result"]
            assert definition["uri"] == uri
            assert definition["range"]["start"]["line"] == 1

            hover = client.request("textDocument/hover", some_position)["result"]
            assert "enum variant" in hover["contents"]["value"]
            assert "QualifiedProject::Choice::Some" in hover["contents"]["value"]

            references = client.request(
                "textDocument/references",
                {**some_position, "context": {"includeDeclaration": True}},
            )["result"]
            assert any(ref["range"]["start"]["line"] == 1 for ref in references)
            assert any(ref["range"]["start"]["line"] == 19 for ref in references)
            client.shutdown()
        finally:
            client.close()
        return
    if test_name == "manifest-root":
        root = REPO_ROOT
        path = fixture("valid_project") / "src" / "Main.cursive"
        client = LspClient(root)
        try:
            client.initialize()
            client.open_file(path)
            diagnostics = client.wait_for_diagnostics(path.as_uri())
            assert diagnostics == [], diagnostics
            client.shutdown()
        finally:
            client.close()
        return
    if test_name == "no-manifest":
        root = REPO_ROOT
        path = REPO_ROOT / "tests" / "lsp" / "fixtures" / "loose" / "Main.cursive"
        client = LspClient(root)
        try:
            client.initialize()
            client.open_file(path)
            diagnostics = client.wait_for_diagnostics(path.as_uri())
            assert diagnostics
            assert "No Cursive.toml" in diagnostics[0]["message"]
            client.shutdown()
        finally:
            client.close()
        return
    if test_name == "code-action":
        root = fixture("invalid_main_project")
        path = root / "src" / "Main.cursive"
        client = LspClient(root)
        try:
            client.initialize()
            client.open_file(path)
            uri = path.as_uri()
            diagnostics = client.wait_for_diagnostics(uri)
            invalid_main = [
                diagnostic
                for diagnostic in diagnostics
                if diagnostic.get("code") == "E-MOD-2431"
            ]
            assert invalid_main, diagnostics
            actions = client.request(
                "textDocument/codeAction",
                {
                    "textDocument": {"uri": uri},
                    "range": invalid_main[0]["range"],
                    "context": {"diagnostics": invalid_main},
                },
            )["result"]
            assert actions
            assert actions[0]["title"] == "Fix main signature"
            edits = actions[0]["edit"]["changes"][uri]
            assert "public procedure main(move ctx: Context)" in edits[0]["newText"]
            client.shutdown()
        finally:
            client.close()
        return
    if test_name == "version":
        proc = subprocess.run(
            [str(default_server_path()), "--version"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True,
        )
        assert "Cursive" in proc.stdout
        assert "language server" in proc.stdout
        return
    raise SystemExit(f"unknown test {test_name}")


if __name__ == "__main__":
    main()
