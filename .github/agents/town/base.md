# Agent Base Profile

This is the shared baseline that applies to all GENERATIONS agents.

## Shared Mission
- Reuse existing code first; add new code only when clearly required.
- Optimize runtime first, then build time, without increasing unnecessary complexity.
- Prefer explicit ownership (`private` > `protected` > `public`) and clear lifecycles.
- Use `const`/`constexpr` where it improves clarity and correctness.
- Keep docs and implementation synchronized.

## Execution Mode
- Agents are always driven by a **manually provided action/task**.
- Each agent analyzes that same task from its archetype perspective.
- Agents run **in sequence**, not in parallel.
- `C++ Lead` always runs first and creates handoff TODOs for all other agents.

## Shared Guardrails
1. Make the smallest change that solves the real root cause.
2. Avoid speculative abstractions and premature optimization.
3. Preserve architectural boundaries and clear ownership.
4. Keep changes cross-platform (Linux + Windows).
5. Prefer explicit behavior over implicit magic.

## Shared Review Checklist
- Is the change easy to reason about and maintain?
- Does it fit existing project conventions and style?
- Are runtime/resource implications understood?
- Are docs updated if behavior or workflows changed?
- Is there a simpler way using existing primitives?

## Escalation Rule
If an agent finds a concern outside its specialty, hand over to the relevant specialist agent and keep findings short and actionable.

## Guild System
Agents belong to **guilds** — cross-role specializations with shared procedures and vocabulary.
- Guild files: `.github/agents/guilds/*.md`
- Procedure log: `.github/agents/town/procedures.md`
- When a guild procedure exists for a task pattern, **use it** instead of inventing a new approach.
- When you discover a reusable pattern, record it in your output under `## Procedure Recording`.

## Structured Handoff Format
All TODO handoffs must follow this format for machine-parseable handoff:
```
### <Agent Name> TODOs
- [ ] `<file path>` — <one-line action> [RISK: low|med|high]
- [ ] `<file path>` — <one-line action> [RISK: low|med|high]
- [ ] GUILD:<guild-name> — <cross-role action requiring guild procedure>
```

## Standard Output Format (all agents)
1. Main task outcome (for the current manual task)
2. Secondary task outcomes
3. Risks and constraints
4. Actionable TODOs (file-scoped, use Structured Handoff Format)
5. Handoff note to next agent
6. Code Proposal
7. Procedure Recording (optional — only when a new reusable pattern was discovered)
