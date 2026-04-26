import { createHash } from "node:crypto"
import { existsSync, readFileSync } from "node:fs"
import * as path from "node:path"
import type { Plugin } from "@opencode-ai/plugin"

const DIRECTIVES_PATH = ".agents/DIRECTIVES"
const PLAN_PATH = ".agents/PLAN.md"
const MUTATING_TOOLS = new Set(["edit", "write", "apply_patch"])
const PLAN_STATUSES = new Set(["active", "completed", "invalidated"])
const PLAN_OPS = new Set(["create", "update", "delete", "move", "any"])

type WriteOp = "create" | "update" | "delete" | "move"
type AllowedOp = WriteOp | "any"

type Mutation = {
  path: string
  op: WriteOp
}

type PatchChange = {
  path: string
  op: WriteOp
  movePath: string | undefined
}

type ParsedPlan = {
  planID: string
  status: string
  allowedWrites: Map<string, Set<AllowedOp>>
  allowedHash: string
  errors: string[]
}

type PolicyContext = {
  directives: string | undefined
  planText: string | undefined
  plan: ParsedPlan | undefined
  text: string
  hash: string
}

type FrozenPlan = {
  planID: string
  allowedHash: string
}

type PendingPlanClosure = FrozenPlan

type SessionContextState = {
  injectedHash: string | undefined
  blockedHash: string | undefined
}

export const PreWritePlanGuard: Plugin = async ({ worktree }) => {
  const contextStateBySession = new Map<string, SessionContextState>()
  const frozenPlanBySession = new Map<string, FrozenPlan>()
  const pendingPlanClosureByCall = new Map<string, PendingPlanClosure>()

  return {
    "tool.definition": async (input, output) => {
      if (!MUTATING_TOOLS.has(input.toolID)) {
        return
      }

      output.description = [
        output.description,
        "",
        "Pre-write plan requirement: before using this tool for any non-plan file, create or update `.agents/PLAN.md` with YAML frontmatter, set `status: \"active\"`, and include every target path/operation under `allowed_writes`. Writes outside the active plan are blocked. `.agents/PLAN.md` must be written by itself before implementation writes.",
      ].join("\n")
    },

    "experimental.chat.system.transform": async (input, output) => {
      const context = buildPolicyContext(worktree)
      output.system.push(context.text)

      if (input.sessionID) {
        contextStateBySession.set(input.sessionID, {
          injectedHash: context.hash,
          blockedHash: undefined,
        })
      }
    },

    "tool.execute.before": async (input, output) => {
      if (input.tool === "bash") {
        guardBash(output.args)
        return
      }

      if (!MUTATING_TOOLS.has(input.tool)) {
        return
      }

      const mutations = extractMutations(input.tool, output.args, worktree)
      if (mutations.length === 0) {
        throw new Error(`Pre-write guard blocked ${input.tool}: no target path was found.`)
      }

      if (mutations.some((mutation) => mutation.path === DIRECTIVES_PATH)) {
        throw new Error(
          `Pre-write guard blocked modification of ${DIRECTIVES_PATH}. ` +
            "This file is user-owned policy and is not editable by agents.",
        )
      }

      const planMutations = mutations.filter((mutation) => mutation.path === PLAN_PATH)
      const nonPlanMutations = mutations.filter((mutation) => mutation.path !== PLAN_PATH)

      if (nonPlanMutations.length === 0) {
        assertPlanOnlyMutationIsSafe(planMutations)
        const pendingPlanClosure = validatePlanOnlyUpdate(
          input.sessionID,
          frozenPlanBySession,
          worktree,
          input.tool,
          output.args,
        )
        if (pendingPlanClosure) {
          pendingPlanClosureByCall.set(callKey(input.sessionID, input.callID), pendingPlanClosure)
        }
        return
      }

      if (planMutations.length > 0) {
        throw new Error(
          `Pre-write guard blocked ${input.tool}: ${PLAN_PATH} must be written separately ` +
            "before any implementation writes. Retry with a plan-only write first.",
        )
      }

      const context = buildPolicyContext(worktree)
      const state = getSessionContextState(contextStateBySession, input.sessionID)
      if (state.blockedHash === context.hash || state.injectedHash !== context.hash) {
        state.blockedHash = context.hash
        throw new Error(buildRetryError(context))
      }

      if (!context.directives) {
        throw new Error(`Pre-write guard blocked write: ${DIRECTIVES_PATH} is missing.`)
      }

      if (!context.planText || !context.plan) {
        throw new Error(`Pre-write guard blocked write: ${PLAN_PATH} is missing or unreadable.`)
      }

      if (context.plan.errors.length > 0) {
        throw new Error(
          [
            `Pre-write guard blocked write: ${PLAN_PATH} is invalid.`,
            ...context.plan.errors.map((error) => `- ${error}`),
          ].join("\n"),
        )
      }

      if (context.plan.status !== "active") {
        throw new Error(
          `Pre-write guard blocked write: ${PLAN_PATH} status is ${JSON.stringify(
            context.plan.status,
          )}; expected "active". Create a new active plan before writing.`,
        )
      }

      const frozenPlan = frozenPlanBySession.get(input.sessionID)
      if (frozenPlan) {
        if (
          frozenPlan.planID !== context.plan.planID ||
          frozenPlan.allowedHash !== context.plan.allowedHash
        ) {
          throw new Error(
            `Pre-write guard blocked write: active plan ${context.plan.planID} no longer ` +
              "matches the write set that was frozen when implementation writes began. " +
              `Set ${PLAN_PATH} to completed or invalidated, then create a new active plan.`,
          )
        }
      }

      const unauthorized = nonPlanMutations.filter(
        (mutation) => !isAuthorized(context.plan!, mutation),
      )
      if (unauthorized.length > 0) {
        throw new Error(
          [
            `Pre-write guard blocked ${input.tool}: write is outside the active plan.`,
            "Unauthorized mutations:",
            ...unauthorized.map((mutation) => `- ${mutation.op} ${mutation.path}`),
            "",
            `Update ${PLAN_PATH} only if scope changed; otherwise retry with paths listed in allowed_writes.`,
          ].join("\n"),
        )
      }

      if (!frozenPlan) {
        frozenPlanBySession.set(input.sessionID, {
          planID: context.plan.planID,
          allowedHash: context.plan.allowedHash,
        })
      }
    },

    "tool.execute.after": async (input) => {
      const pendingPlanClosure = pendingPlanClosureByCall.get(callKey(input.sessionID, input.callID))
      if (!pendingPlanClosure) {
        return
      }

      const frozenPlan = frozenPlanBySession.get(input.sessionID)
      if (
        frozenPlan &&
        frozenPlan.planID === pendingPlanClosure.planID &&
        frozenPlan.allowedHash === pendingPlanClosure.allowedHash &&
        isFrozenPlanClosedOnDisk(worktree, pendingPlanClosure)
      ) {
        frozenPlanBySession.delete(input.sessionID)
        pendingPlanClosureByCall.delete(callKey(input.sessionID, input.callID))
      }
    },
  }
}

function callKey(sessionID: string, callID: string): string {
  return `${sessionID}:${callID}`
}

function getSessionContextState(
  contextStateBySession: Map<string, SessionContextState>,
  sessionID: string,
): SessionContextState {
  let state = contextStateBySession.get(sessionID)
  if (!state) {
    state = {
      injectedHash: undefined,
      blockedHash: undefined,
    }
    contextStateBySession.set(sessionID, state)
  }

  return state
}

function buildPolicyContext(worktree: string): PolicyContext {
  const directives = readWorkspaceFile(worktree, DIRECTIVES_PATH)
  const planText = readWorkspaceFile(worktree, PLAN_PATH)
  const plan = planText ? parsePlan(planText, worktree) : undefined

  const sections = [
    "## Mandatory OpenCode Pre-Write Context",
    "The pre-write hook enforces this context before any non-plan write tool executes.",
    `### ${DIRECTIVES_PATH}`,
    directives ?? `[missing: create ${DIRECTIVES_PATH} before relying on this hook]`,
    `### ${PLAN_PATH}`,
    planText ?? `[missing: write ${PLAN_PATH} before any implementation write]`,
    "### Hook Protocol",
    [
      `- ${PLAN_PATH} may be written by itself to create, complete, or invalidate a plan.`,
      `- ${DIRECTIVES_PATH} is user-owned policy and must not be edited by agents.`,
      "- Non-plan writes require an active plan whose allowed_writes exactly authorizes every path and operation.",
      "- If this context changed after the model turn started, the hook blocks the write and the agent must retry after reading the injected context.",
      "- After implementation writes begin, the allowed write set is frozen for the session.",
    ].join("\n"),
  ]

  if (plan?.errors.length) {
    sections.push(
      "### Current Plan Validation Errors",
      plan.errors.map((error) => `- ${error}`).join("\n"),
    )
  }

  const text = sections.join("\n\n")
  return {
    directives,
    planText,
    plan,
    text,
    hash: sha256(text),
  }
}

function buildRetryError(context: PolicyContext): string {
  return [
    "Pre-write guard blocked the current write phase because the current directive/plan context was not injected before this assistant turn authored writes.",
    "Do not retry another write in this same turn. Start a new model turn after incorporating the mandatory context below, then retry the planned write.",
    "",
    context.text,
  ].join("\n")
}

function readWorkspaceFile(worktree: string, relativePath: string): string | undefined {
  const fullPath = path.resolve(worktree, relativePath)
  if (!existsSync(fullPath)) {
    return undefined
  }

  return readFileSync(fullPath, "utf8")
}

function parsePlan(text: string, worktree: string): ParsedPlan {
  const errors: string[] = []
  const frontmatter = extractFrontmatter(text)
  if (!frontmatter) {
    return emptyPlan([`${PLAN_PATH} must start with YAML frontmatter delimited by --- lines.`])
  }

  const planID = parseScalar(frontmatter, "plan_id")
  const status = parseScalar(frontmatter, "status")
  const completionCondition = hasTopLevelKey(frontmatter, "completion_condition")
  const invalidationTriggers = hasTopLevelKey(frontmatter, "invalidation_triggers")
  const allowedWritesBlock = extractTopLevelBlock(frontmatter, "allowed_writes")

  if (!planID) {
    errors.push("frontmatter key `plan_id` is required.")
  }

  if (!status) {
    errors.push("frontmatter key `status` is required.")
  } else if (!PLAN_STATUSES.has(status)) {
    errors.push("frontmatter key `status` must be one of active, completed, or invalidated.")
  }

  if (!completionCondition) {
    errors.push("frontmatter key `completion_condition` is required.")
  }

  if (!invalidationTriggers) {
    errors.push("frontmatter key `invalidation_triggers` is required.")
  }

  if (!allowedWritesBlock) {
    errors.push("frontmatter key `allowed_writes` is required.")
  }

  const allowedWrites = allowedWritesBlock
    ? parseAllowedWrites(allowedWritesBlock, worktree, errors)
    : new Map<string, Set<AllowedOp>>()

  if (allowedWrites.size === 0) {
    errors.push("`allowed_writes` must contain at least one path entry.")
  }

  return {
    planID: planID ?? "",
    status: status ?? "",
    allowedWrites,
    allowedHash: hashAllowedWrites(allowedWrites),
    errors,
  }
}

function emptyPlan(errors: string[]): ParsedPlan {
  return {
    planID: "",
    status: "",
    allowedWrites: new Map(),
    allowedHash: hashAllowedWrites(new Map()),
    errors,
  }
}

function extractFrontmatter(text: string): string | undefined {
  const normalized = text.replace(/\r\n/g, "\n")
  const match = normalized.match(/^---\n([\s\S]*?)\n---(?:\n|$)/)
  return match?.[1]
}

function parseScalar(frontmatter: string, key: string): string | undefined {
  const escapedKey = key.replace(/[.*+?^${}()|[\]\\]/g, "\\$&")
  const match = frontmatter.match(new RegExp(`^${escapedKey}:\\s*(.*?)\\s*$`, "m"))
  const raw = match?.[1]?.trim()
  if (!raw) {
    return undefined
  }

  return stripQuotes(raw)
}

function hasTopLevelKey(frontmatter: string, key: string): boolean {
  const escapedKey = key.replace(/[.*+?^${}()|[\]\\]/g, "\\$&")
  return new RegExp(`^${escapedKey}:`, "m").test(frontmatter)
}

function extractTopLevelBlock(frontmatter: string, key: string): string | undefined {
  const lines = frontmatter.split("\n")
  const start = lines.findIndex((line) => line.trim() === `${key}:`)
  if (start < 0) {
    return undefined
  }

  const block: string[] = []
  for (const line of lines.slice(start + 1)) {
    if (/^[A-Za-z_][A-Za-z0-9_-]*:\s*/.test(line)) {
      break
    }

    block.push(line)
  }

  return block.join("\n")
}

function parseAllowedWrites(
  block: string,
  worktree: string,
  errors: string[],
): Map<string, Set<AllowedOp>> {
  const allowedWrites = new Map<string, Set<AllowedOp>>()
  const entries: Array<{ path?: string; ops: AllowedOp[] }> = []
  let current: { path?: string; ops: AllowedOp[] } | undefined
  let collectingOps = false

  for (const line of block.split("\n")) {
    const pathMatch = line.match(/^\s*-\s*path:\s*(.+?)\s*$/)
    if (pathMatch) {
      if (current) {
        entries.push(current)
      }

      current = {
        path: normalizeRepoPath(stripQuotes(pathMatch[1].trim()), worktree),
        ops: [],
      }
      collectingOps = false
      continue
    }

    const opsMatch = line.match(/^\s*ops:\s*(.*?)\s*$/)
    if (opsMatch && current) {
      current.ops.push(...parseOps(opsMatch[1]))
      collectingOps = opsMatch[1].trim() === ""
      continue
    }

    const opItemMatch = line.match(/^\s*-\s*(.+?)\s*$/)
    if (collectingOps && current && opItemMatch) {
      current.ops.push(...parseOps(opItemMatch[1]))
    }
  }

  if (current) {
    entries.push(current)
  }

  for (const entry of entries) {
    if (!entry.path) {
      errors.push("each allowed_writes entry must include a non-empty path.")
      continue
    }

    if (entry.path.startsWith("../") || entry.path === "..") {
      errors.push(`allowed_writes path ${JSON.stringify(entry.path)} escapes the worktree.`)
      continue
    }

    if (entry.ops.length === 0) {
      errors.push(`allowed_writes entry for ${entry.path} must include at least one op.`)
      continue
    }

    const ops = allowedWrites.get(entry.path) ?? new Set<AllowedOp>()
    for (const op of entry.ops) {
      if (!PLAN_OPS.has(op)) {
        errors.push(`allowed_writes entry for ${entry.path} has invalid op ${JSON.stringify(op)}.`)
        continue
      }

      ops.add(op)
    }

    allowedWrites.set(entry.path, ops)
  }

  return allowedWrites
}

function parseOps(value: string): AllowedOp[] {
  const trimmed = value.trim()
  if (!trimmed) {
    return []
  }

  if (trimmed.startsWith("[") && trimmed.endsWith("]")) {
    return trimmed
      .slice(1, -1)
      .split(",")
      .map((part) => stripQuotes(part.trim()))
      .filter((part): part is AllowedOp => part.length > 0)
  }

  return [stripQuotes(trimmed) as AllowedOp]
}

function stripQuotes(value: string): string {
  const trimmed = value.trim()
  if (
    (trimmed.startsWith('"') && trimmed.endsWith('"')) ||
    (trimmed.startsWith("'") && trimmed.endsWith("'"))
  ) {
    return trimmed.slice(1, -1)
  }

  return trimmed
}

function hashAllowedWrites(allowedWrites: Map<string, Set<AllowedOp>>): string {
  const encoded = [...allowedWrites.entries()]
    .sort(([left], [right]) => left.localeCompare(right))
    .map(([entryPath, ops]) => `${entryPath}:${[...ops].sort().join(",")}`)
    .join("|")

  return sha256(encoded)
}

function extractMutations(tool: string, args: any, worktree: string): Mutation[] {
  if (tool === "apply_patch") {
    return extractPatchMutations(String(args?.patchText ?? ""), worktree)
  }

  const rawPath = args?.filePath ?? args?.path ?? args?.file ?? args?.filename
  if (!rawPath) {
    return []
  }

  const normalizedPath = normalizeRepoPath(String(rawPath), worktree)
  const fullPath = path.resolve(worktree, normalizedPath)
  const op = classifyDirectMutation(tool, args, fullPath)
  return [{ path: normalizedPath, op }]
}

function classifyDirectMutation(tool: string, args: any, fullPath: string): WriteOp {
  if (tool === "write") {
    return existsSync(fullPath) ? "update" : "create"
  }

  if (tool === "edit" && args?.oldString === "") {
    return existsSync(fullPath) ? "update" : "create"
  }

  return "update"
}

function extractPatchMutations(patchText: string, worktree: string): Mutation[] {
  const changes: PatchChange[] = []
  let currentChange: PatchChange | undefined

  for (const rawLine of patchText.replace(/\r\n/g, "\n").split("\n")) {
    const line = rawLine.trimEnd()
    const addMatch = line.match(/^\*\*\* Add File:\s+(.+)$/)
    if (addMatch) {
      currentChange = { path: normalizeRepoPath(addMatch[1], worktree), op: "create", movePath: undefined }
      changes.push(currentChange)
      continue
    }

    const updateMatch = line.match(/^\*\*\* Update File:\s+(.+)$/)
    if (updateMatch) {
      currentChange = { path: normalizeRepoPath(updateMatch[1], worktree), op: "update", movePath: undefined }
      changes.push(currentChange)
      continue
    }

    const deleteMatch = line.match(/^\*\*\* Delete File:\s+(.+)$/)
    if (deleteMatch) {
      currentChange = { path: normalizeRepoPath(deleteMatch[1], worktree), op: "delete", movePath: undefined }
      changes.push(currentChange)
      continue
    }

    const moveMatch = line.match(/^\*\*\* Move to:\s+(.+)$/)
    if (moveMatch) {
      if (currentChange) {
        currentChange.op = "move"
        currentChange.movePath = normalizeRepoPath(moveMatch[1], worktree)
      }
    }
  }

  return changes.flatMap((change) => {
    if (change.op === "move" && change.movePath) {
      return [
        { path: change.path, op: "move" as const },
        { path: change.movePath, op: "move" as const },
      ]
    }

    return [{ path: change.path, op: change.op }]
  })
}

function normalizeRepoPath(rawPath: string, worktree: string): string {
  const cleaned = stripQuotes(rawPath.trim()).replace(/\\/g, "/")
  const absolutePath = path.isAbsolute(cleaned)
    ? path.resolve(cleaned)
    : path.resolve(worktree, cleaned)
  const relativePath = path.relative(worktree, absolutePath).replace(/\\/g, "/")
  return relativePath.replace(/^\.\//, "")
}

function assertPlanOnlyMutationIsSafe(mutations: Mutation[]): void {
  const unsafe = mutations.filter((mutation) => mutation.op === "delete" || mutation.op === "move")
  if (unsafe.length > 0) {
    throw new Error(
      [
        `Pre-write guard blocked destructive mutation of ${PLAN_PATH}.`,
        ...unsafe.map((mutation) => `- ${mutation.op} ${mutation.path}`),
      ].join("\n"),
    )
  }
}

function validatePlanOnlyUpdate(
  sessionID: string,
  frozenPlanBySession: Map<string, FrozenPlan>,
  worktree: string,
  tool: string,
  args: any,
): PendingPlanClosure | undefined {
  const frozenPlan = frozenPlanBySession.get(sessionID)
  if (!frozenPlan) {
    return undefined
  }

  const planText = nextPlanText(tool, args, worktree)
  if (!planText) {
    throw new Error(
      `Pre-write guard blocked ${PLAN_PATH} update: implementation writes already began, ` +
        "and the hook cannot derive the resulting plan text for this operation. " +
        "Use write or edit to set the frozen plan to completed or invalidated.",
    )
  }

  const plan = parsePlan(planText, worktree)
  if (plan.errors.length > 0) {
    throw new Error(
      [
        `Pre-write guard blocked ${PLAN_PATH} update: resulting plan is invalid.`,
        ...plan.errors.map((error) => `- ${error}`),
      ].join("\n"),
    )
  }

  if (
    plan.planID === frozenPlan.planID &&
    plan.allowedHash === frozenPlan.allowedHash &&
    (plan.status === "completed" || plan.status === "invalidated")
  ) {
    return frozenPlan
  }

  if (plan.planID !== frozenPlan.planID || plan.allowedHash !== frozenPlan.allowedHash) {
    throw new Error(
      `Pre-write guard blocked ${PLAN_PATH} update: implementation writes already began ` +
        "for the frozen plan, so the plan_id and allowed_writes set cannot change. " +
        "Set the existing frozen plan to completed or invalidated first, then create a new active plan.",
    )
  }

  throw new Error(
    `Pre-write guard blocked ${PLAN_PATH} update: implementation writes already began, ` +
      "so the frozen plan can only be marked completed or invalidated without changing plan_id or allowed_writes.",
  )
}

function isFrozenPlanClosedOnDisk(worktree: string, pendingPlanClosure: PendingPlanClosure): boolean {
  const planText = readWorkspaceFile(worktree, PLAN_PATH)
  if (!planText) {
    return false
  }

  const plan = parsePlan(planText, worktree)
  return (
    plan.errors.length === 0 &&
    plan.planID === pendingPlanClosure.planID &&
    plan.allowedHash === pendingPlanClosure.allowedHash &&
    (plan.status === "completed" || plan.status === "invalidated")
  )
}

function nextPlanText(tool: string, args: any, worktree: string): string | undefined {
  if (tool === "write") {
    return typeof args?.content === "string" ? args.content : undefined
  }

  if (tool === "edit") {
    if (typeof args?.newString !== "string") {
      return undefined
    }

    const current = readWorkspaceFile(worktree, PLAN_PATH)
    if (current === undefined || typeof args?.oldString !== "string") {
      return args.newString
    }

    if (args.oldString === "") {
      return args.newString
    }

    if (!current.includes(args.oldString)) {
      return undefined
    }

    if (args.replaceAll === true) {
      return current.split(args.oldString).join(args.newString)
    }

    if (current.indexOf(args.oldString) !== current.lastIndexOf(args.oldString)) {
      return undefined
    }

    return current.replace(args.oldString, args.newString)
  }

  return undefined
}

function isAuthorized(plan: ParsedPlan, mutation: Mutation): boolean {
  const ops = plan.allowedWrites.get(mutation.path)
  if (!ops) {
    return false
  }

  return ops.has("any") || ops.has(mutation.op)
}

function guardBash(args: any): void {
  const command = String(args?.command ?? "")
  if (!command) {
    return
  }

  if (isReadOnlyBashCommand(command)) {
    return
  }

  throw new Error(
    "Pre-write guard blocked this shell command. Shell execution is restricted " +
      "to a small read-only allowlist because arbitrary shell commands can bypass " +
      "path-level write planning. Use edit/write/apply_patch for planned writes.",
  )
}

function isReadOnlyBashCommand(command: string): boolean {
  const trimmed = command.trim()
  return [
    /^git\s+(status|diff|log|show|branch|rev-parse|ls-files)\b/i,
    /^opencode\s+debug\s+config\b/i,
    /^node\s+--version\s*$/i,
    /^npm\s+--version\s*$/i,
    /^bun\s+--version\s*$/i,
  ].some((pattern) => pattern.test(trimmed))
}

function sha256(value: string): string {
  return createHash("sha256").update(value).digest("hex")
}
