#!/bin/bash
# GENERATIONS Agent Schedule Manager
# This script helps manage and execute scheduled agent tasks

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
AGENTS_DIR="$SCRIPT_DIR"
LOGS_DIR="$SCRIPT_DIR/logs"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Create logs directory if it doesn't exist
mkdir -p "$LOGS_DIR"

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to list all agents
list_agents() {
    print_info "Available agents:"
    for agent_file in "$AGENTS_DIR"/*.json; do
        if [ -f "$agent_file" ]; then
            agent_name=$(basename "$agent_file" .json)
            specialty=$(jq -r '.specialty' "$agent_file" 2>/dev/null || echo "Unknown")
            echo "  - $agent_name: $specialty"
        fi
    done
}

# Function to show agent details
show_agent() {
    local agent_name="$1"
    local agent_file="$AGENTS_DIR/${agent_name}.json"
    
    if [ ! -f "$agent_file" ]; then
        print_error "Agent '$agent_name' not found"
        return 1
    fi
    
    print_info "Agent: $(jq -r '.name' "$agent_file")"
    echo "Specialty: $(jq -r '.specialty' "$agent_file")"
    echo "Description: $(jq -r '.description' "$agent_file")"
    echo ""
    echo "Capabilities:"
    jq -r '.capabilities[]' "$agent_file" | while read -r cap; do
        echo "  - $cap"
    done
    echo ""
    echo "Scheduled Tasks:"
    jq -r '.scheduled_tasks[] | "  - \(.task) (\(.frequency)): \(.description)"' "$agent_file"
}

# Function to validate crontab
validate_crontab() {
    local crontab_file="$AGENTS_DIR/crontab"
    
    if [ ! -f "$crontab_file" ]; then
        print_error "Crontab file not found: $crontab_file"
        return 1
    fi
    
    print_info "Validating crontab syntax..."
    
    # Basic validation - check for valid cron expressions
    local line_num=0
    local errors=0
    local valid_lines=0
    
    # Temporarily disable set -e for the while loop
    set +e
    while IFS= read -r line || [ -n "$line" ]; do
        ((line_num++))
        
        # Skip comments and empty lines
        if [[ "$line" =~ ^[[:space:]]*# ]] || [[ -z "$line" ]]; then
            continue
        fi
        
        # Check if line has at least 6 fields (5 time fields + command)
        local field_count=$(echo "$line" | awk '{print NF}')
        if [ "$field_count" -lt 6 ]; then
            print_error "Line $line_num: Invalid cron expression (has $field_count fields, need at least 6)"
            ((errors++))
        else
            ((valid_lines++))
        fi
    done < "$crontab_file"
    set -e
    
    if [ $errors -eq 0 ] && [ $valid_lines -gt 0 ]; then
        print_success "Crontab validation passed ($valid_lines valid entries)"
        return 0
    elif [ $valid_lines -eq 0 ]; then
        print_warning "No valid cron entries found in crontab"
        return 1
    else
        print_error "Crontab validation failed with $errors error(s)"
        return 1
    fi
}

# Function to run a specific agent task
run_task() {
    local agent_name="$1"
    local task_name="$2"
    local agent_file="$AGENTS_DIR/${agent_name}.json"
    
    if [ ! -f "$agent_file" ]; then
        print_error "Agent '$agent_name' not found"
        return 1
    fi
    
    print_info "Running task '$task_name' for agent '$agent_name'..."
    
    local timestamp=$(date +%Y%m%d_%H%M%S)
    local log_file="$LOGS_DIR/${agent_name}_${timestamp}.log"
    
    {
        echo "====================================="
        echo "Agent: $agent_name"
        echo "Task: $task_name"
        echo "Started: $(date)"
        echo "====================================="
        echo ""
        
        case "$agent_name" in
            "build-agent")
                run_build_task "$task_name"
                ;;
            "test-agent")
                run_test_task "$task_name"
                ;;
            "documentation-agent")
                run_documentation_task "$task_name"
                ;;
            "code-quality-agent")
                run_code_quality_task "$task_name"
                ;;
            "asset-agent")
                run_asset_task "$task_name"
                ;;
            *)
                print_error "Unknown agent: $agent_name"
                return 1
                ;;
        esac
        
        echo ""
        echo "====================================="
        echo "Completed: $(date)"
        echo "====================================="
    } 2>&1 | tee "$log_file"
    
    print_success "Task completed. Log saved to: $log_file"
}

# Build agent task implementations
run_build_task() {
    local task="$1"
    
    case "$task" in
        "Nightly build validation")
            cd "$PROJECT_ROOT"
            print_info "Running nightly build validation..."
            
            # Clean previous build
            rm -rf build/*
            
            # Configure with CMake
            cd build
            cmake .. || return 1
            
            # Build
            make -j$(nproc) || return 1
            
            print_success "Build validation completed successfully"
            ;;
        *)
            print_warning "Task '$task' not implemented yet"
            ;;
    esac
}

# Test agent task implementations
run_test_task() {
    local task="$1"
    
    case "$task" in
        "Daily smoke tests")
            print_info "Running daily smoke tests..."
            cd "$PROJECT_ROOT"
            
            # Check if binary exists
            if [ ! -f "bin/CapitalEngine" ]; then
                print_error "CapitalEngine binary not found. Build required."
                return 1
            fi
            
            print_success "Smoke tests completed"
            ;;
        "Weekly regression testing")
            print_info "Running weekly regression tests..."
            print_warning "Full test suite not implemented yet"
            ;;
        *)
            print_warning "Task '$task' not implemented yet"
            ;;
    esac
}

# Documentation agent task implementations
run_documentation_task() {
    local task="$1"
    
    case "$task" in
        "API docs generation")
            print_info "Generating API documentation..."
            print_warning "Doxygen documentation generation not configured yet"
            ;;
        "Monthly documentation review")
            print_info "Running monthly documentation review..."
            print_warning "Documentation review not implemented yet"
            ;;
        *)
            print_warning "Task '$task' not implemented yet"
            ;;
    esac
}

# Code quality agent task implementations
run_code_quality_task() {
    local task="$1"
    
    case "$task" in
        "Daily code quality checks")
            print_info "Running daily code quality checks..."
            cd "$PROJECT_ROOT/src"
            
            # Check for common issues
            print_info "Checking for TODO/FIXME comments..."
            grep -rn "TODO\|FIXME" . || echo "No TODO/FIXME found"
            
            print_success "Code quality checks completed"
            ;;
        "Weekly full codebase scan")
            print_info "Running weekly full codebase scan..."
            print_warning "Full codebase analysis not implemented yet"
            ;;
        *)
            print_warning "Task '$task' not implemented yet"
            ;;
    esac
}

# Asset agent task implementations
run_asset_task() {
    local task="$1"
    
    case "$task" in
        "Shader optimization")
            print_info "Running shader optimization..."
            cd "$PROJECT_ROOT/shaders"
            
            # List current shaders
            print_info "Current shader files:"
            find . -type f \( -name "*.vert" -o -name "*.frag" -o -name "*.comp" \)
            
            print_warning "Shader optimization not fully implemented yet"
            ;;
        "Asset cleanup")
            print_info "Running asset cleanup..."
            print_warning "Asset cleanup not implemented yet"
            ;;
        *)
            print_warning "Task '$task' not implemented yet"
            ;;
    esac
}

# Function to install crontab
install_crontab() {
    local crontab_file="$AGENTS_DIR/crontab"
    
    print_info "Installing agent crontab..."
    
    if [ ! -f "$crontab_file" ]; then
        print_error "Crontab file not found: $crontab_file"
        return 1
    fi
    
    # Backup existing crontab
    if crontab -l > /dev/null 2>&1; then
        print_info "Backing up existing crontab..."
        crontab -l > "$LOGS_DIR/crontab_backup_$(date +%Y%m%d_%H%M%S)"
    fi
    
    # Install new crontab
    crontab "$crontab_file"
    print_success "Crontab installed successfully"
    print_info "Current crontab:"
    crontab -l
}

# Main command dispatcher
main() {
    local command="${1:-help}"
    
    case "$command" in
        list)
            list_agents
            ;;
        show)
            if [ -z "$2" ]; then
                print_error "Usage: $0 show <agent-name>"
                exit 1
            fi
            show_agent "$2"
            ;;
        run)
            if [ -z "$2" ] || [ -z "$3" ]; then
                print_error "Usage: $0 run <agent-name> <task-name>"
                exit 1
            fi
            run_task "$2" "$3"
            ;;
        validate)
            validate_crontab
            ;;
        install)
            install_crontab
            ;;
        help|*)
            echo "GENERATIONS Agent Schedule Manager"
            echo ""
            echo "Usage: $0 <command> [arguments]"
            echo ""
            echo "Commands:"
            echo "  list                    - List all available agents"
            echo "  show <agent-name>       - Show details of a specific agent"
            echo "  run <agent> <task>      - Run a specific agent task"
            echo "  validate                - Validate the crontab configuration"
            echo "  install                 - Install the crontab for scheduled execution"
            echo "  help                    - Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0 list"
            echo "  $0 show build-agent"
            echo "  $0 run build-agent \"Nightly build validation\""
            echo "  $0 validate"
            echo "  $0 install"
            ;;
    esac
}

# Check for required commands
command -v jq >/dev/null 2>&1 || {
    print_error "jq is required but not installed. Please install jq."
    exit 1
}

main "$@"
