#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace omnifix::old_charts
{
    inline constexpr auto matches(std::int32_t mid) -> bool
    {
        return mid >= 0 && mid % 1000 >= 500;
    }

    struct result
    {
        bool valid {};
        std::size_t songs {};
    };

    // Operate on the bytes actually returned by the MDB read, never file data
    // on disk. Validate the entire directory before changing any ratings.
    inline auto clear_ratings(std::span<std::uint8_t> bytes) -> result
    {
        constexpr std::size_t header_size = 0x10;
        constexpr std::size_t level_offset = 0x3EC;
        constexpr std::size_t chart_count = 10;
        if (bytes.size() < header_size || std::memcmp(bytes.data(), "IIDX", 4))
            return {};
        auto read_i32 = [&](std::size_t offset) {
            std::int32_t value;
            std::memcpy(&value, bytes.data() + offset, sizeof(value));
            return value;
        };
        auto const version = read_i32(4);
        auto const entries = read_i32(8);
        auto const max_entries = read_i32(12);
        if (version < 32 || version > 34 || entries <= 0 || max_entries <= 0 ||
            static_cast<std::size_t>(max_entries) > (bytes.size() - header_size) / 4)
            return {};
        auto const first = header_size + static_cast<std::size_t>(max_entries) * 4;
        auto const payload = bytes.size() - first;
        if (payload % static_cast<std::size_t>(entries))
            return {};
        auto const stride = payload / static_cast<std::size_t>(entries);
        if (stride < level_offset + chart_count)
            return {};
        for (std::int32_t mid = 0; mid < max_entries; ++mid)
        {
            auto const index = read_i32(header_size + static_cast<std::size_t>(mid) * 4);
            if (index < -1 || index >= entries)
                return {};
        }
        // Match the existing MDB loader's first-ID-per-entry convention.
        auto seen = std::vector<bool>(static_cast<std::size_t>(entries), false);
        auto output = result { true, 0 };
        for (std::int32_t mid = 0; mid < max_entries; ++mid)
        {
            auto const index = read_i32(header_size + static_cast<std::size_t>(mid) * 4);
            if (index < 0 || seen[index])
                continue;
            seen[index] = true;
            if (!matches(mid))
                continue;
            std::memset(bytes.data() + first + static_cast<std::size_t>(index) * stride +
                level_offset, 0, chart_count);
            ++output.songs;
        }
        return output;
    }
}
