#pragma once

#include <cstdint>

// Guarantee the size of variable
using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderId = std::uint64_t;

// Define InvalidPrice. Used for market order
struct Constants
{
    static constexpr Price InvalidPrice { -1 };
};