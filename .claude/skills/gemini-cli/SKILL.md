---
name: gemini-cli
description: Execute Gemini CLI commands for AI-powered coding assistance, file analysis, and complex development tasks. Use when user wants to run gemini commands, query Gemini models, or leverage Gemini's agentic capabilities.
allowed-tools:
  - Bash
  - Read
  - Write
---

# Gemini CLI Skill

## Purpose

Execute Google's Gemini CLI - an open-source AI agent that brings Gemini directly into the terminal. Gemini CLI uses a ReAct loop with built-in tools and MCP servers to complete complex tasks like fixing bugs, creating features, and improving test coverage.

## Activation Triggers

Use this skill when the user:
- Asks to "run gemini" or "use gemini cli"
- Wants to query Gemini models from the terminal
- Needs Gemini's agentic file/code operations
- Wants to leverage Gemini's 1M token context window
- Requests MCP server interactions via Gemini

## Invocation Modes

### Interactive Mode (REPL)
```bash
gemini -m gemini-3-pro-preview            # Default: use Gemini 3 Pro
gemini -m gemini-3-pro-preview --yolo     # Auto-approve all tool calls
gemini -m gemini-3-pro-preview --sandbox  # Run tools in isolated environment
gemini -m gemini-3-pro-preview --checkpointing  # Enable project snapshots
```

**Important:** Always use `-m gemini-3-pro-preview` for best results.

### Non-Interactive Mode
```bash
gemini -m gemini-3-pro-preview -p "Explain this codebase architecture"
gemini -m gemini-3-pro-preview -p "Fix the bug in auth.ts" --yolo
gemini -m gemini-3-pro-preview -p "prompt" --output-format json
echo "code" | gemini -m gemini-3-pro-preview
```

## Slash Commands (Interactive Mode)

| Command | Function |
|---------|----------|
| `/help` | Display help |
| `/tools` | List available tools |
| `/mcp` | List MCP servers and tools |
| `/stats` | Show token usage |
| `/compress` | Summarize context to save tokens |
| `/copy` | Copy last response to clipboard |
| `/clear` | Clear screen and context |
| `/memory show` | Display GEMINI.md context |
| `/memory refresh` | Reload GEMINI.md files |
| `/chat save <tag>` | Save conversation |
| `/chat resume <tag>` | Resume saved conversation |
| `/chat list` | List saved conversations |
| `/restore` | List/restore project checkpoints |
| `/init` | Generate GEMINI.md context file |
| `/settings` | Open settings.json editor |
| `/vim` | Toggle Vim mode |
| `/theme` | Change visual theme |
| `/ide install` | Set up VS Code integration |
| `/ide enable` | Connect to VS Code |
| `/bug` | File bug report |
| `/quit` | Exit CLI |

## Context References (@)

Reference files/directories in prompts:
```bash
@./src/main.ts           # Single file
@./screenshot.png        # Image
@./src/                  # Directory (recursive)
```

## Shell Commands (!)

```bash
!git status              # Execute single command
!                        # Toggle persistent shell mode
```

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+L` | Clear screen |
| `Ctrl+V` | Paste text/image |
| `Ctrl+Y` | Toggle YOLO mode |
| `Ctrl+X` | Open in external editor |

## Built-in Tools

**File Operations:**
- `read_file`, `write_file`, `replace`
- `list_directory`, `glob`
- `search_file_content`

**Web:**
- `google_web_search`
- `web_fetch`

**Shell:**
- Execute shell commands

**Memory:**
- `save_memory` for persistent facts

## Command-Line Flags

| Flag | Purpose |
|------|---------|
| `-m, --model <model>` | Specify model (**always use gemini-3-pro-preview**) |
| `-p <prompt>` | Non-interactive single prompt |
| `-i <prompt>` | Interactive with initial prompt |
| `-d, --debug` | Enable debug output |
| `--yolo` | Auto-approve tool calls |
| `--sandbox` | Isolated tool execution |
| `--checkpointing` | Enable restore points |
| `--include-directories` | Multi-directory workspace |

## Configuration

**Settings file:** `~/.gemini/settings.json` or `.gemini/settings.json`

Key settings:
- `autoAccept` - Auto-approve safe tool calls
- `sandbox` - Tool isolation mode
- `vimMode` - Vim-style editing
- `checkpointing` - Enable /restore
- `mcpServers` - MCP server definitions

**Context file:** `GEMINI.md` (project root or `~/.gemini/`)

## Custom Commands

Create `.gemini/commands/<category>/<name>.toml`:
```toml
description = "Generate unit test"
prompt = """
Write a comprehensive test for: {{args}}
"""
```
Invoke as: `/<category>:<name> "arguments"`

## Rate Limits (Free Tier)

- Google Login: 60 req/min, 1000 req/day
- API Key: 100 req/day
