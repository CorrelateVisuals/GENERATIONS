layout(push_constant, std430) uniform PushConstantsBlock {
    uint passedHours;
    float dayFraction;
} pushConstants;

#define passedHours pushConstants.passedHours
#define dayFraction pushConstants.dayFraction
