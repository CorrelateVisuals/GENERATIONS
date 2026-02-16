# GENERATIONS Agents

This directory contains the configuration and management tools for automated agents that perform scheduled tasks for the GENERATIONS project.

## Agent Profiles

Each agent has a JSON profile that defines:
- **Name and Specialty**: What the agent does
- **Description**: Detailed explanation of the agent's purpose
- **Capabilities**: List of what the agent can do
- **Route Tasks**: Regular responsibilities the agent handles
- **Tools**: Software and utilities the agent uses
- **Scheduled Tasks**: Automated tasks that run on a schedule

### Available Agents

1. **build-agent.json** - Compilation and Build Management
2. **test-agent.json** - Validation and Testing
3. **documentation-agent.json** - Documentation Maintenance
4. **code-quality-agent.json** - Code Analysis and Quality Assurance
5. **asset-agent.json** - Shader and Asset Management

## Schedule Management

### Main Schedule Document

See [schedule.md](../../schedule.md) in the project root for a comprehensive overview of all agents and their scheduled tasks.

### Crontab Configuration

The `crontab` file contains the cron expressions for all scheduled tasks. All times are in UTC.

### Management Script

The `manage-schedule.sh` script provides a convenient interface for managing agents:

```bash
# List all available agents
./manage-schedule.sh list

# Show details of a specific agent
./manage-schedule.sh show build-agent

# Run a specific task manually
./manage-schedule.sh run build-agent "Nightly build validation"

# Validate the crontab configuration
./manage-schedule.sh validate

# Install the crontab for scheduled execution
./manage-schedule.sh install
```

## Adding New Agents

To add a new agent:

1. Create a new JSON profile in this directory (e.g., `my-agent.json`)
2. Define the agent's properties following the existing format
3. Add scheduled tasks to the `crontab` file
4. Update the `schedule.md` document in the project root
5. Add task implementation in `manage-schedule.sh`

### Agent Profile Template

```json
{
  "name": "Agent Name",
  "specialty": "Agent Specialty",
  "description": "Detailed description of what the agent does",
  "capabilities": [
    "Capability 1",
    "Capability 2"
  ],
  "route_tasks": [
    "Regular task 1",
    "Regular task 2"
  ],
  "tools": [
    "tool1",
    "tool2"
  ],
  "scheduled_tasks": [
    {
      "task": "Task name",
      "frequency": "daily|weekly|monthly",
      "description": "What this task does"
    }
  ]
}
```

## Logs

Task execution logs are stored in the `logs/` subdirectory with timestamps:
- Format: `{agent-name}_{timestamp}.log`
- Automatically created by the management script
- Useful for debugging and monitoring

## Requirements

- **jq**: JSON processor (required for the management script)
- **cron**: For scheduled execution
- Project-specific tools as defined in each agent's profile

## Installation

To set up the scheduled agents on a new system:

1. Install required dependencies:
   ```bash
   sudo apt-get install jq cron
   ```

2. Validate the configuration:
   ```bash
   ./manage-schedule.sh validate
   ```

3. Install the crontab:
   ```bash
   ./manage-schedule.sh install
   ```

4. Verify installation:
   ```bash
   crontab -l
   ```

## Notes

- All scheduled tasks run in UTC timezone
- The management script creates logs automatically
- Failed tasks should be investigated using the logs
- Modify `crontab` to adjust schedules
- Update `schedule.md` when making changes to keep documentation in sync
