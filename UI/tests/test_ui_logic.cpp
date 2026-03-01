/**
 * @file test_ui_logic.cpp
 * @brief Unit tests for UI module rendering logic (TDD).
 * Before implementing the actual drawing function, the data conversion logic must be verified first.
 */

#include <iostream>
#include <cassert>
#include <algorithm>

/**
 * @brief Mock logic for UI height calculation.
 * Extract the calculation logic to ensure its suitability for different resolutions.
 */
int calculateBarHeight(float intensity, int maxHeight) {
    // Implementation logic: Clamp the input range and compute the pixel height.
    float clipped = std::max(0.0f, std::min(1.0f, intensity));
    return static_cast<int>(clipped * maxHeight);
}

/**
 * @brief Tests intensity mapping within the standard range [0.0, 1.0].
 */
void testStandardMapping() {
    std::cout << "Testing standard intensity mapping..." << std::endl;
    int maxHeight = 400;
    
    // 50% intensity should map to 200 pixels
    assert(calculateBarHeight(0.5f, maxHeight) == 200);
    // 100% intensity should map to 400 pixels
    assert(calculateBarHeight(1.0f, maxHeight) == 400);
    // 0% intensity should map to 0 pixels
    assert(calculateBarHeight(0.0f, maxHeight) == 0);
    
    std::cout << "Standard mapping test PASSED." << std::endl;
}

/**
 * @brief Tests robustness against out-of-bounds input.
 */
void testRobustness() {
    std::cout << "Testing robustness with out-of-bounds input..." << std::endl;
    int maxHeight = 400;

    // Input exceeding 1.0 should be clamped to maxHeight
    assert(calculateBarHeight(1.5f, maxHeight) == 400);
    // Negative input should be clamped to 0
    assert(calculateBarHeight(-0.5f, maxHeight) == 0);

    std::cout << "Robustness test PASSED." << std::endl;
}

int main() {
    std::cout << "======= Starting UI Module Unit Tests (TDD) =======" << std::endl;
    
    try {
        testStandardMapping();
        testRobustness();
        std::cout << "======= All UI Tests PASSED! =======" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test FAILED with exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}