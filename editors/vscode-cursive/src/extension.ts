import * as fs from "fs";
import * as path from "path";
import * as vscode from "vscode";
import {
  LanguageClient,
  LanguageClientOptions,
  ServerOptions,
  Trace,
  TransportKind
} from "vscode-languageclient/node";

let client: LanguageClient | undefined;

function defaultServerPath(context: vscode.ExtensionContext): string {
  const repoRoot = path.resolve(context.extensionPath, "..", "..");
  const executable = process.platform === "win32" ? "Cursive_LSP.exe" : "Cursive_LSP";
  const buildHost = process.platform === "win32" ? "windows" : "linux";
  return path.join(repoRoot, "cursive", "build", buildHost, "Debug", executable);
}

function configuredServerPath(context: vscode.ExtensionContext): string {
  const config = vscode.workspace.getConfiguration("cursive");
  const configured = config.get<string>("lsp.path", "").trim();
  return configured.length > 0 ? configured : defaultServerPath(context);
}

function serverArgs(): string[] {
  const config = vscode.workspace.getConfiguration("cursive");
  const args = ["--stdio"];
  const logFile = config.get<string>("lsp.logFile", "").trim();
  if (logFile.length > 0) {
    args.push("--log-file", logFile);
  }
  return args;
}

function traceSetting(): Trace {
  const trace = vscode.workspace
    .getConfiguration("cursive")
    .get<string>("lsp.trace.server", "off");

  if (trace === "verbose") {
    return Trace.Verbose;
  }
  if (trace === "messages") {
    return Trace.Messages;
  }
  return Trace.Off;
}

export async function activate(context: vscode.ExtensionContext): Promise<void> {
  const serverPath = configuredServerPath(context);
  const outputChannel = vscode.window.createOutputChannel("Cursive Language Server");
  context.subscriptions.push(outputChannel);

  if (!fs.existsSync(serverPath)) {
    const message =
      `Cursive language server not found at ${serverPath}. ` +
      "Build it with: cmake --build --preset windows-debug --target cursive-lsp";
    outputChannel.appendLine(message);
    void vscode.window.showErrorMessage(message);
    return;
  }

  const serverOptions: ServerOptions = {
    run: {
      command: serverPath,
      args: serverArgs(),
      transport: TransportKind.stdio
    },
    debug: {
      command: serverPath,
      args: serverArgs(),
      transport: TransportKind.stdio
    }
  };

  const clientOptions: LanguageClientOptions = {
    documentSelector: [
      {
        scheme: "file",
        language: "cursive"
      }
    ],
    outputChannel,
    synchronize: {
      fileEvents: [
        vscode.workspace.createFileSystemWatcher("**/*.cursive"),
        vscode.workspace.createFileSystemWatcher("**/Cursive.toml")
      ]
    }
  };

  client = new LanguageClient(
    "cursive-lsp",
    "Cursive Language Server",
    serverOptions,
    clientOptions
  );
  await client.setTrace(traceSetting());

  context.subscriptions.push(client);
  await client.start();
}

export async function deactivate(): Promise<void> {
  if (client === undefined) {
    return;
  }
  const activeClient = client;
  client = undefined;
  await activeClient.stop();
}
