"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
exports.activate = activate;
exports.deactivate = deactivate;
const fs = __importStar(require("fs"));
const path = __importStar(require("path"));
const vscode = __importStar(require("vscode"));
const node_1 = require("vscode-languageclient/node");
let client;
function defaultServerPath(context) {
    const repoRoot = path.resolve(context.extensionPath, "..", "..");
    if (process.platform === "win32") {
        return path.join(repoRoot, "cursive", "build", "windows", "Debug", "Cursive_LSP.exe");
    }
    return path.join(repoRoot, "cursive", "build", "linux", "Cursive_LSP");
}
function configuredServerPath(context) {
    const config = vscode.workspace.getConfiguration("cursive");
    const configured = config.get("lsp.path", "").trim();
    return configured.length > 0 ? configured : defaultServerPath(context);
}
function serverArgs() {
    const config = vscode.workspace.getConfiguration("cursive");
    const args = ["--stdio"];
    const logFile = config.get("lsp.logFile", "").trim();
    if (logFile.length > 0) {
        args.push("--log-file", logFile);
    }
    const targetProfile = config.get("lsp.targetProfile", "").trim();
    if (targetProfile.length > 0) {
        args.push("--target-profile", targetProfile);
    }
    return args;
}
function traceSetting() {
    const trace = vscode.workspace
        .getConfiguration("cursive")
        .get("lsp.trace.server", "off");
    if (trace === "verbose") {
        return node_1.Trace.Verbose;
    }
    if (trace === "messages") {
        return node_1.Trace.Messages;
    }
    return node_1.Trace.Off;
}
async function activate(context) {
    const serverPath = configuredServerPath(context);
    const outputChannel = vscode.window.createOutputChannel("Cursive Language Server");
    context.subscriptions.push(outputChannel);
    if (!fs.existsSync(serverPath)) {
        const message = `Cursive language server not found at ${serverPath}. ` +
            "Build it with: cmake --build --preset windows-debug --target cursive-lsp";
        outputChannel.appendLine(message);
        void vscode.window.showErrorMessage(message);
        return;
    }
    const serverOptions = {
        run: {
            command: serverPath,
            args: serverArgs(),
            transport: node_1.TransportKind.stdio
        },
        debug: {
            command: serverPath,
            args: serverArgs(),
            transport: node_1.TransportKind.stdio
        }
    };
    const clientOptions = {
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
    client = new node_1.LanguageClient("cursive-lsp", "Cursive Language Server", serverOptions, clientOptions);
    await client.setTrace(traceSetting());
    context.subscriptions.push(client);
    await client.start();
}
async function deactivate() {
    if (client === undefined) {
        return;
    }
    const activeClient = client;
    client = undefined;
    await activeClient.stop();
}
//# sourceMappingURL=extension.js.map