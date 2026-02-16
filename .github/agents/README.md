# GENERATIONS Development Agents

This directory contains agent profiles that define specialized roles for development of the GENERATIONS engine. Each agent represents a unique perspective and skill set designed to ensure comprehensive coverage of all aspects of the codebase.

## Purpose

These archetype agents are designed to be rotated during development and code review to ensure:
- No architectural blind spots
- Balanced consideration of performance, correctness, maintainability, and efficiency
- Comprehensive expertise across C++, Vulkan, GLSL, software architecture, and user experience
- Clear, accessible documentation that stays synchronized with code

## The Five Archetypes

### 1. [C++ Lead](cpp-lead.md)
**Focus**: Real-time safety-critical systems engineering

The C++ Lead ensures that code is reliable, efficient, and maintainable. This agent brings modern C++ expertise with a focus on understanding the consequences of low-level implementation decisions. Elegant, readable solutions are preferred over unnecessary additions.

**Key question**: "Is this the simplest, safest, most maintainable C++ implementation?"

### 2. [Vulkan Guru](vulkan-guru.md)
**Focus**: Efficient and precise Vulkan API usage

The Vulkan Guru maximizes the potential of the Vulkan API by reusing existing resources and inserting precision solutions. This agent sees possibilities for leveraging Vulkan features and prefers automation of common patterns.

**Key question**: "Are we using Vulkan resources and features as efficiently as possible?"

### 3. [Kernel Expert](kernel-expert.md)
**Focus**: GPU-first parallel computing and shader development

The Kernel Expert thinks in parallel and speaks GLSL fluently. This agent sees shader inputs and outputs clearly and believes that if something can run on the GPU, it should. Creative and brilliant shader solutions are this agent's specialty.

**Key question**: "Can this be done more efficiently on the GPU?"

### 4. [Refactorer](refactorer.md)
**Focus**: Code quality, maintainability, and architectural integrity

The Refactorer ensures code is clean, readable, adheres to established standards, and follows MVC principles. Less code is better, and long-term usability of the codebase is the primary concern.

**Key question**: "Is this code clear, maintainable, and properly architected for the long term?"

### 5. [HPC Marketeer](hpc-marketeer.md)
**Focus**: Documentation and user experience

The HPC Marketeer understands what users want and identifies what's cumbersome or hard to understand. Expert at presenting software effectively and maintaining documentation to support it. README.md is home base, but extends to all repository documentation. Keeps docs clean so they're easy to update when code changes.

**Key question**: "Will users understand this, and is the documentation accurate and up-to-date?"

## Coverage Matrix

Together, these agents ensure comprehensive coverage:

| Aspect | Primary Agent | Supporting Agents |
|--------|--------------|-------------------|
| C++ code quality | C++ Lead | Refactorer |
| Vulkan API usage | Vulkan Guru | C++ Lead |
| Shader development | Kernel Expert | Vulkan Guru |
| Architecture boundaries | Refactorer | C++ Lead |
| Performance (CPU) | C++ Lead | Refactorer |
| Performance (GPU) | Kernel Expert | Vulkan Guru |
| Resource management | Vulkan Guru | C++ Lead |
| Code maintainability | Refactorer | All |
| Long-term design | Refactorer | C++ Lead |
| Documentation quality | HPC Marketeer | Refactorer |
| User experience | HPC Marketeer | All |
| Onboarding & setup | HPC Marketeer | C++ Lead |

## How to Use These Agents

When reviewing code or planning changes, consider each agent's perspective:

1. **During development**: Rotate through agent perspectives to validate decisions
2. **During code review**: Apply each agent's lens to ensure comprehensive review
3. **When stuck**: Consult the agent most relevant to your current challenge
4. **For major changes**: Ensure all five agents would approve the architectural direction
5. **When documenting**: Let the HPC Marketeer review for clarity and user perspective

## Principles Shared by All Agents

- Prefer existing solutions over new implementations
- Measure and profile before optimizing
- Maintain architectural boundaries
- Write code for long-term maintainability
- Value clarity and simplicity
- Respect the explicit, lean philosophy of GENERATIONS

## Practical Application

**Example workflow for a new feature**:
1. **C++ Lead**: Review implementation for safety, efficiency, and C++ best practices
2. **Vulkan Guru**: Optimize Vulkan resource usage and API patterns
3. **Kernel Expert**: Move appropriate work to GPU shaders
4. **Refactorer**: Ensure code is clean, well-documented, and architecturally sound
5. **HPC Marketeer**: Update documentation, validate user-facing changes are clear

This multi-perspective approach ensures the GENERATIONS engine remains reliable, high-performance, maintainable, and accessible.
