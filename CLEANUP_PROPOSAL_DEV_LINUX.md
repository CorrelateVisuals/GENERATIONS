# Cleanup Proposal (dev-linux)

This proposal lists likely duplicate or unused code discovered by scan, prioritized for low-risk cleanup.

## High Confidence: Remove dead commented blocks

1. Large legacy command-buffer implementation is fully commented out and duplicates active logic.
   - [src/Resources.cpp](src/Resources.cpp#L300-L476)
   - Recommendation: delete the entire commented block.

## High Confidence: Simplify always-true branch

3. `enablePostFX` is a local constant set to `true`, making branch conditions redundant.
   - [src/ShaderAccess.cpp](src/ShaderAccess.cpp#L46)
   - [src/ShaderAccess.cpp](src/ShaderAccess.cpp#L146-L176)
   - Recommendation: inline PostFX path (remove `if (enablePostFX)`), or promote to runtime setting if toggling is required.

## Medium Confidence: Duplicate/legacy scaffolding

4. Anonymous namespace contains commented placeholders only.
   - [src/Resources.cpp](src/Resources.cpp#L6-L10)
   - Recommendation: remove unused commented placeholders and empty namespace if no symbols remain.

## Optional refactor opportunities (behavior-preserving)

6. Repeated Vulkan draw-bind pattern in graphics recording could be extracted into tiny helpers to reduce repetition.
   - [src/ShaderAccess.cpp](src/ShaderAccess.cpp#L85-L143)
   - Recommendation: extract helper calls only if readability improves and profiling/validation stays unchanged.

## Validation checklist

- `cmake --build build`
- `./bin/CapitalEngine`
- Confirm no new validation-layer errors in startup logs.
