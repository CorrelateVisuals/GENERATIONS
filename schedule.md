# GENERATIONS Agent Schedule

This document provides a centralized overview of all automated agents, their main tasks, and scheduling information.

## Agent Overview

The GENERATIONS project uses specialized agents to automate various development and maintenance tasks. Each agent has a specific specialty and set of responsibilities.

### 1. Build Agent
**Specialty:** Compilation and Build Management  
**Profile:** [.github/agents/build-agent.json](.github/agents/build-agent.json)

**Main Task:** Ensure the project builds successfully across all supported platforms (Linux/Windows) and keep all build artifacts up to date.

**Scheduled Tasks:**
- **Nightly Build Validation** (Daily at 2:00 AM UTC)
  - Full clean build to ensure compilation integrity
  - Tests both CMake/Linux and Visual Studio/Windows configurations
  - Reports build failures immediately
  
- **Shader Recompilation** (On file change)
  - Automatically recompiles GLSL shaders to SPIR-V when source files change
  - Validates shader syntax before compilation

---

### 2. Test Agent
**Specialty:** Validation and Testing  
**Profile:** [.github/agents/test-agent.json](.github/agents/test-agent.json)

**Main Task:** Validate functionality and ensure simulation accuracy through comprehensive testing.

**Scheduled Tasks:**
- **Daily Smoke Tests** (Daily at 8:00 AM UTC)
  - Quick validation of core functionality
  - Runs Conway's Game of Life simulation validation
  - Checks Vulkan initialization and basic rendering
  
- **Weekly Regression Testing** (Every Sunday at 10:00 AM UTC)
  - Full test suite execution
  - Performance benchmarking
  - Memory leak detection with valgrind

---

### 3. Documentation Agent
**Specialty:** Documentation Maintenance and Generation  
**Profile:** [.github/agents/documentation-agent.json](.github/agents/documentation-agent.json)

**Main Task:** Keep all project documentation accurate, up-to-date, and comprehensive.

**Scheduled Tasks:**
- **Weekly API Documentation Generation** (Every Monday at 9:00 AM UTC)
  - Regenerate API documentation from code comments
  - Update Doxygen-generated docs
  
- **Monthly Documentation Review** (1st of each month at 10:00 AM UTC)
  - Review and update README files
  - Validate all documentation links
  - Update architecture documentation

---

### 4. Code Quality Agent
**Specialty:** Code Analysis and Quality Assurance  
**Profile:** [.github/agents/code-quality-agent.json](.github/agents/code-quality-agent.json)

**Main Task:** Maintain high code quality standards through automated analysis and enforcement of best practices.

**Scheduled Tasks:**
- **Daily Code Quality Checks** (Daily at 6:00 AM UTC)
  - Run static analysis on recent changes
  - Check code formatting compliance
  - Validate naming conventions (snake_case)
  
- **Weekly Full Codebase Scan** (Every Saturday at 3:00 AM UTC)
  - Comprehensive code quality analysis
  - Code complexity metrics
  - Duplicate code detection

---

### 5. Asset Agent
**Specialty:** Shader and Asset Management  
**Profile:** [.github/agents/asset-agent.json](.github/agents/asset-agent.json)

**Main Task:** Manage and optimize shaders and game assets for optimal performance.

**Scheduled Tasks:**
- **Weekly Shader Optimization** (Every Wednesday at 11:00 AM UTC)
  - Analyze shader performance
  - Optimize SPIR-V bytecode
  - Generate optimization reports
  
- **Monthly Asset Cleanup** (Last day of month at 2:00 PM UTC)
  - Remove unused assets
  - Organize asset directory structure
  - Update asset inventory

---

## Cron Schedule Summary

All scheduled tasks use UTC timezone. See [.github/agents/crontab](.github/agents/crontab) for the complete cron configuration.

| Time (UTC) | Days | Agent | Task |
|------------|------|-------|------|
| 02:00 | Daily | Build | Nightly Build Validation |
| 06:00 | Daily | Code Quality | Daily Code Quality Checks |
| 08:00 | Daily | Test | Daily Smoke Tests |
| 09:00 | Monday | Documentation | API Documentation Generation |
| 10:00 | Sunday | Test | Weekly Regression Testing |
| 10:00 | 1st of month | Documentation | Monthly Documentation Review |
| 11:00 | Wednesday | Asset | Weekly Shader Optimization |
| 15:00 | Saturday | Code Quality | Weekly Full Codebase Scan |
| 14:00 | Last day of month | Asset | Monthly Asset Cleanup |

## Managing Agent Schedules

### Viewing Agent Information
Each agent has a JSON profile in `.github/agents/` containing:
- Specialty and description
- Capabilities
- Route tasks (regular responsibilities)
- Tools used
- Scheduled tasks with frequencies

### Modifying Schedules
1. Update the agent's JSON profile in `.github/agents/`
2. Update the cron configuration in `.github/agents/crontab`
3. Update this schedule.md document
4. Run the schedule manager: `./github/agents/manage-schedule.sh validate`

### Running Tasks Manually
Use the schedule manager to run any agent task manually:
```bash
./.github/agents/manage-schedule.sh run <agent-name> <task-name>
```

Example:
```bash
./.github/agents/manage-schedule.sh run build "Nightly build validation"
```

## Notes
- All agents run in isolated environments to prevent conflicts
- Logs are stored in `.github/agents/logs/`
- Failed tasks trigger notifications
- Schedules can be temporarily disabled by editing the crontab
