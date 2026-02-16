# Agent Quick Reference

## Agents Overview

| Agent | Specialty | Main Task |
|-------|-----------|-----------|
| Build Agent | Compilation & Build Management | Ensure successful builds across all platforms |
| Test Agent | Validation & Testing | Validate functionality and simulation accuracy |
| Documentation Agent | Documentation Maintenance | Keep all project docs up-to-date |
| Code Quality Agent | Code Analysis & QA | Maintain high code quality standards |
| Asset Agent | Shader & Asset Management | Manage and optimize shaders and assets |

## Quick Commands

```bash
# List all agents
./.github/agents/manage-schedule.sh list

# Show agent details
./.github/agents/manage-schedule.sh show <agent-name>

# Run a task manually
./.github/agents/manage-schedule.sh run <agent-name> "<task-name>"

# Validate crontab
./.github/agents/manage-schedule.sh validate

# Install crontab (for scheduling)
./.github/agents/manage-schedule.sh install
```

## Example Usage

```bash
# Check build agent capabilities
./.github/agents/manage-schedule.sh show build-agent

# Manually run a build validation
./.github/agents/manage-schedule.sh run build-agent "Nightly build validation"

# Run code quality checks
./.github/agents/manage-schedule.sh run code-quality-agent "Daily code quality checks"

# Optimize shaders
./.github/agents/manage-schedule.sh run asset-agent "Shader optimization"
```

## Scheduled Tasks

All scheduled tasks are defined in `.github/agents/crontab` and documented in `schedule.md`.

### Daily Tasks
- 02:00 UTC - Build: Nightly Build Validation
- 06:00 UTC - Code Quality: Daily Code Quality Checks
- 08:00 UTC - Test: Daily Smoke Tests

### Weekly Tasks
- Monday 09:00 UTC - Documentation: API Documentation Generation
- Wednesday 11:00 UTC - Asset: Shader Optimization
- Saturday 03:00 UTC - Code Quality: Full Codebase Scan
- Sunday 10:00 UTC - Test: Weekly Regression Testing

### Monthly Tasks
- 1st of month 10:00 UTC - Documentation: Monthly Documentation Review
- Last day of month 14:00 UTC - Asset: Asset Cleanup

## Files and Locations

- **Agent Profiles**: `.github/agents/*.json`
- **Cron Schedule**: `.github/agents/crontab`
- **Management Script**: `.github/agents/manage-schedule.sh`
- **Task Logs**: `.github/agents/logs/`
- **Main Schedule**: `schedule.md`
- **Agent README**: `.github/agents/README.md`

## Adding New Tasks

1. Update the agent's JSON profile in `.github/agents/`
2. Add cron entry to `.github/agents/crontab`
3. Implement task in `manage-schedule.sh`
4. Update `schedule.md`
5. Test with `./manage-schedule.sh run <agent> "<task>"`

---

For detailed information, see:
- [schedule.md](schedule.md) - Complete schedule and task descriptions
- [.github/agents/README.md](.github/agents/README.md) - Detailed agent documentation
