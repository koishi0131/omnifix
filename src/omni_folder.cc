#include "old_charts.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstring>
#include <cstdint>
#include <format>
#include <initializer_list>
#include <memory>
#include <limits>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <windows.h>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <safetyhook.hpp>

#include "avs2.h"
#include "bm2dx.h"
#include "memory.h"
#include "omni_folder.h"

// Release build: diagnostic traces are compiled out at every log level.
// Keep the call type-checked without evaluating its arguments or emitting it.
#define OMNIFIX_FOLDER_TRACE(...) \
    do { if constexpr (false) { avs2::log::misc(__VA_ARGS__); } } while (false)

using namespace omnifix;

namespace omnifix::omni_folder
{
    namespace game
    {
        enum play_style
        {
            style_sp = 0,
            style_dp = 1,
        };

        enum category_group_type
        {
            group_version = 2,
            group_all = 6,
        };

        enum bar_entry_type
        {
            bar_type_header = 0,
            bar_type_chart = 1,
            bar_type_random = 2,
        };

        enum clear_type
        {
            clear_no_play = 0,
            clear_failed = 1,
            clear_assist = 2,
            clear_easy = 3,
            clear_normal = 4,
            clear_hard = 5,
            clear_ex_hard = 6,
            clear_full_combo = 7,
        };

        auto inline (*vector_ptr_insert) (void*, void*, void**) -> void* {};

        template <typename T>
        struct vector
        {
            [[nodiscard]] auto operator[] (std::size_t index) noexcept -> T&
                { return data[index]; }

            [[nodiscard]] auto size() const noexcept -> std::size_t
                { return static_cast<std::size_t>(end - data); }

            auto push_back(T&& ptr) -> T*
            {
                return insert(end, std::forward<T>(ptr));
            }

            auto insert(T* position, T&& ptr) -> T*
            {
                static_assert(std::is_pointer_v<T>, "pointer types only");

                if (position < data || position > end)
                    return nullptr;

                if (end >= capacity)
                    return static_cast<T*>(game::vector_ptr_insert
                        (this, position, reinterpret_cast<void**>(&ptr)));

                std::move_backward(position, end, end + 1);
                *position = ptr;
                ++end;
                return position;
            }

            /* 0x0000 */ T* data;
            /* 0x0008 */ T* end;
            /* 0x0010 */ T* capacity;
        }; static_assert(sizeof(vector<void*>) == 0x18);

        struct small_string
        {
            /* 0x0000 */ char buffer[16];
            /* 0x0010 */ std::size_t size;
            /* 0x0018 */ std::size_t capacity;
        }; static_assert(sizeof(small_string) == 0x20);

        struct category_definition
        {
            /* 0x0000 */ std::int32_t group_type;
            /* 0x0004 */ std::int32_t default_sort;
            /* 0x0008 */ std::uint8_t difficulty_switchable;
            /* 0x0009 */ std::uint8_t difficulty_locked;
            /* 0x000A */ std::uint8_t pad_000A[6];
            /* 0x0010 */ const char* texture_hover_header;
            /* 0x0018 */ const char* texture_hover_subheader;
            /* 0x0020 */ const char* texture_bar_text;
            /* 0x0028 */ const char* texture_bar_bg;
            /* 0x0030 */ const char* ticker_text;
            /* 0x0038 */ std::int32_t capacity;
            /* 0x003C */ std::uint8_t pad_003C[4];
        }; static_assert(sizeof(category_definition) == 0x40);
        static_assert(offsetof(category_definition, default_sort) == 0x04);

        struct header_bar
        {
            /* 0x0000 */ std::int32_t id;
            /* 0x0004 */ std::int32_t active_difficulty;
            /* 0x0008 */ std::uint8_t pad_0008[8];
            /* 0x0010 */ small_string string;
        }; static_assert(sizeof(header_bar) == 0x30);

        struct random_bar
        {
            /* 0x0000 */ std::int32_t category_id;
            /* 0x0004 */ std::uint8_t pad_0004[12];
        }; static_assert(sizeof(random_bar) == 0x10);

        struct chart_bar
        {
            /* 0x0000 */ void* music_entry;
            /* 0x0008 */ std::int32_t p1_difficulty;
            /* 0x000C */ std::int32_t p2_difficulty;
            /* 0x0010 */ std::int32_t p1_original_difficulty;
            /* 0x0014 */ std::int32_t p2_original_difficulty;
            /* 0x0018 */ std::int32_t play_style;
            /* 0x001C */ std::int32_t category_id;
            /* 0x0020 */ std::int32_t bar_style[5];
            /* 0x0034 */ std::uint8_t pad_0034[12];
        }; static_assert(sizeof(chart_bar) == 0x40);

        struct bar_entry
        {
            /* 0x0000 */ bar_entry_type type;
            /* 0x0004 */ std::uint32_t flags;
            /* 0x0008 */ void* data;
        }; static_assert(sizeof(bar_entry) == 0x10);

        struct category
        {
            /* 0x0000 */ std::int32_t id;
            /* 0x0004 */ std::uint8_t is_open;
            /* 0x0005 */ std::uint8_t pad_0005[3];
            /* 0x0008 */ std::int32_t bar_count;
            /* 0x000C */ std::int32_t chart_count;
            /* 0x0010 */ std::int32_t insert_count;
            /* 0x0014 */ std::int32_t active_bar_index;
            /* 0x0018 */ std::uint8_t pad_0018[8];
            /* 0x0020 */ vector<bar_entry> bar_list;
            /* 0x0038 */ header_bar header;
            /* 0x0068 */ random_bar random;
            /* 0x0078 */ vector<chart_bar> chart_list;
            /* 0x0090 */ std::uint8_t pad_0090[64];
        }; static_assert(sizeof(category) == 0xD0);

        struct category_group
        {
            /* 0x0000 */ void* vtable;
            /* 0x0008 */ std::int32_t group_id;
            /* 0x000C */ std::uint8_t pad_000C[108];
            /* 0x0078 */ vector<category*> sp_bars;
            /* 0x0090 */ vector<category*> dp_bars;
        }; static_assert(sizeof(category_group) == 0xA8);

        struct CCategoryGameData;
        struct CCustomizeGameData;

        struct system_sound_entry
        {
            /* 0x0000 */ const char* description;
            /* 0x0008 */ const char* filename;
            /* 0x0010 */ std::uint8_t pad_0010[0x278];
        }; static_assert(sizeof(system_sound_entry) == 0x288);
    }

    struct song_reference
    {
        std::int32_t music_id;
        std::uint8_t difficulty_mask;
        // Chart LEVEL values for the five difficulties in this play style.
        // These are the numeric levels shown by the game (1..12), not the
        // difficulty type/order such as NORMAL, HYPER, or ANOTHER.
        std::array<std::uint8_t, 5> levels {};
    };

    struct folder_variant
    {
        std::int32_t id {};
        std::int32_t play_style {};
        std::vector<song_reference> songs {};
        game::category category {};
        game::category_definition definition {};
        std::array<std::array<game::clear_type, 5>, 2> clear_lamps {};
        bool clear_lamps_valid {};
        bool inserted {};
    };

    struct targets
    {
        std::uint8_t* bar_populate {};
        std::uint8_t* get_definition {};
        std::uint8_t* category_lookup {};
        std::uint8_t* active_category {};
        std::uint8_t* folder_voice {};
        std::uint8_t* close_categories {};
        std::uint8_t* music_select_init {};
        std::uint8_t* clear_lamp {};
        // IIDX34 added a native Version-category insertion path that will
        // happily accept synthetic version 80.  Older builds do not need this
        // guard, so the target is optional and scanned only when present.
        std::uint8_t* version_category_insert {};
        std::uint8_t* badge_weekly {};
        std::uint8_t* badge_featured {};
        std::uint8_t* badge_tournament {};
        std::uint8_t* badge_kac {};
        std::ptrdiff_t active_category_id_offset {};

        auto (*sort_category) (game::category*, int, bool) -> void {};
        auto (*init_category_bar) (void*, int) -> bool {};
        auto (*get_music_entry) (int) -> void* {};
        auto (*bar_insert_chart) (const void* [5], int, int, int) -> bool {};
        auto (*get_score_data) (int, int, int, int, int*, int*, game::clear_type*) -> int {};

        std::unordered_map<std::string, int> system_sounds {};
    };

    struct phase2_targets
    {
        std::uint8_t* subscreen_call_rbx {};
        std::uint8_t* subscreen_call_r8 {};
        std::array<std::array<std::uint8_t*, 3>, 5> title_color_calls {};
        std::array<std::uint8_t*, 5> title_route_merge_points {};
        std::array<std::uint8_t, 5> title_route_stack_offsets {};
        std::uint8_t title_layout3_entry_offset {};
        // Additional title renderers used outside Music Select. Slots 0/1
        // are shared title renderers and slot 2 is the text_mu renderer.
        // Some older builds expose only the first shared renderer.
        std::array<std::array<std::uint8_t*, 3>, 3> extended_title_color_calls {};
        std::array<std::uint8_t*, 3> extended_title_route_merge_points {};
        // Route selector: 0 = stack, 1 = [RBP+4], 2 = [RSI+4].
        std::array<std::uint8_t, 3> extended_title_route_stack_offsets {};
        std::array<std::uint8_t, 3> extended_title_route_registers {};
        // Shared renderers carry the entry in RBX (0) or RDI (1).
        std::array<std::uint8_t, 2> extended_title_entry_registers {};
        bool subscreen_supported {};
        bool title_color_supported {};
        bool extended_title_color_supported {};
    };

    struct phase2_runtime
    {
        phase2_targets target {};

        SafetyHookMid hook_subscreen_call_rbx {};
        SafetyHookMid hook_subscreen_call_r8 {};
        std::array<std::array<SafetyHookMid, 3>, 5> hook_title_color {};
        std::array<SafetyHookMid, 5> hook_title_route_merge {};
        std::array<std::array<SafetyHookMid, 3>, 3> hook_extended_title_color {};
        std::array<SafetyHookMid, 3> hook_extended_title_route_merge {};
        std::array<bool, 3> extended_style_logged {};
        std::array<bool, 2> subscreen_logged {};
        std::array<bool, 8> title_color_logged {};
        // Diagnostic counters are per logical layout and native colour route:
        // 0 = Version 34 preview, 1 = current-version colour, 2 = default.
        std::array<std::array<std::uint32_t, 3>, 8> title_route_log_count {};
    };

    struct runtime
    {
        config settings {};
        targets target {};
        std::array<folder_variant, 2> folders {};
        int active_folder_style { game::style_sp };
        bool active_folder_style_valid {};
        // Stable INFINITAS whitelist keyed by MID. Runtime entry pointers are
        // mapped back to their MID after Music Select initializes, so every
        // feature shares the same whitelist instead of maintaining a separate
        // pointer-only target set.
        std::unordered_set<std::int32_t> infinitas_whitelist {};
        std::unordered_map<std::uintptr_t, std::int32_t> entry_to_mid {};
        const char* subscreen_texture_ptr {};
        std::unique_ptr<phase2_runtime> phase2 {};

        safetyhook::InlineHook hook_bar_populate {};
        safetyhook::InlineHook hook_get_definition {};
        safetyhook::InlineHook hook_category_lookup {};
        safetyhook::InlineHook hook_active_category {};
        safetyhook::InlineHook hook_folder_voice {};
        safetyhook::InlineHook hook_close_categories {};
        safetyhook::InlineHook hook_music_select_init {};
        safetyhook::InlineHook hook_clear_lamp {};
        safetyhook::InlineHook hook_version_category_insert {};
        std::array<SafetyHookMid, 2> hook_search_category {};
        safetyhook::InlineHook hook_badge_weekly {};
        safetyhook::InlineHook hook_badge_featured {};
        safetyhook::InlineHook hook_badge_tournament {};
        safetyhook::InlineHook hook_badge_kac {};
    };

    auto static g_runtime = std::unique_ptr<runtime> {};

    auto find_forward(auto addr, auto size, auto pattern)
        { return memory::find({ addr, addr + size }, pattern); }

    auto find_backward(auto addr, auto size, auto pattern)
        { return memory::rfind({ addr - size, addr }, pattern); }

    auto backtrack_prologue(auto addr, auto size)
    {
        if (!addr)
            throw std::runtime_error { "cannot backtrack from a null pattern result" };

        // Playlister may already have installed an inline detour before Omnifix
        // loads. In that case the first byte of the function is no longer 0x48
        // (usually it is a JMP), so the original "CC [48]" boundary pattern
        // fails even though the function itself is still present. Match the last
        // INT3 padding byte followed by any first opcode and return that opcode.
        auto const result = memory::rfind(
            { addr - size, addr }, "CC [?]", true);

        if (!result)
            throw std::runtime_error { "failed to locate function boundary" };

        if (*result != 0x48)
        {
            OMNIFIX_FOLDER_TRACE(
                "80th folder: function entry at {:#x} is already detoured (opcode {:#04x})",
                std::bit_cast<std::uintptr_t>(result), *result);
        }

        return result;
    }

    template <typename T = std::uint8_t*>
    auto follow_find(auto region, auto pattern) -> T
        { return reinterpret_cast<T>(memory::follow(memory::find(region, pattern))); }

    auto find_all(std::span<std::uint8_t> region, std::string_view pattern)
        -> std::vector<std::uint8_t*>
    {
        auto result = std::vector<std::uint8_t*> {};
        auto cursor = region.data();
        auto const end = region.data() + region.size();

        while (cursor < end)
        {
            auto const match = memory::find(
                { cursor, static_cast<std::size_t>(end - cursor) },
                pattern, true);

            if (!match)
                break;

            result.push_back(match);
            cursor = match + 1;
        }

        return result;
    }

    auto find_exact(std::span<std::uint8_t> region,
        std::string_view name, std::string_view pattern, std::size_t expected)
        -> std::vector<std::uint8_t*>
    {
        auto result = find_all(region, pattern);

        if (result.size() != expected)
        {
            throw std::runtime_error { std::format(
                "80th folder pattern '{}' expected {} match(es), found {}",
                name, expected, result.size()) };
        }

        for (auto const address: result)
        {
            OMNIFIX_FOLDER_TRACE("80th folder: {} at {:#x}",
                name, std::bit_cast<std::uintptr_t>(address));
        }

        return result;
    }

    auto find_vector_ptr_insert(std::span<std::uint8_t> region)
    {
        auto const base = follow_find(region,
            "E8 ? ? ? ? 90 80 7D ? ? 74 ? 48 8B 5D");

        auto result = find_forward(base, 128, "4C 8D 44 24 ? [E8]");
             result = memory::follow(result);

        return reinterpret_cast<decltype(game::vector_ptr_insert)>(result);
    }

    auto find_system_sounds(std::span<std::uint8_t> region)
        -> std::unordered_map<std::string, int>
    {
        auto const base = memory::find(region,
            "48 8D 05 ? ? ? ? 48 03 F8 81 C2", true);

        if (!base)
            return {};

        auto index = 0x2000;
        auto result = std::unordered_map<std::string, int> {};
        auto const head = reinterpret_cast<game::system_sound_entry*>
            (memory::follow(base));
        auto const tail = reinterpret_cast<const std::uint8_t*>
            (head->description);

        for (auto entry = head;
            reinterpret_cast<std::uint8_t*>(entry) < tail; ++entry)
        {
            if (!entry->filename || entry->description == entry->filename)
                continue;

            result[entry->filename] = index;
            index += 0x50;
        }

        return result;
    }

    auto find_badge_targets(std::span<std::uint8_t> region)
        -> std::array<std::uint8_t*, 4>
    {
        auto const matches = find_all(region,
            "49 69 C1 A8 00 00 00 0F B6 84 38");

        if (matches.size() != 4)
        {
            throw std::runtime_error { std::format(
                "expected 4 badge scan results, found {}", matches.size()) };
        }

        // Order: weekly, featured/recommend, tournament/shop, KAC.
        auto result = std::array<std::uint8_t*, 4> {};

        for (auto const match: matches)
        {
            auto const entry = backtrack_prologue(match, 64);

            switch (match[11] & 0x03)
            {
                case 0: result[0] = entry; break;
                case 1: result[1] = entry; break;
                case 2: result[2] = entry; break;
                case 3: result[3] = entry; break;
                default: break;
            }
        }

        if (std::ranges::any_of(result, [] (auto address) { return !address; }))
            throw std::runtime_error { "failed to classify all badge accessors" };

        return result;
    }

    auto scan_targets(std::span<std::uint8_t> region) -> targets
    {
        auto result = targets {};

        game::vector_ptr_insert = find_vector_ptr_insert(region);
        OMNIFIX_FOLDER_TRACE("80th folder: vector insert at {:#x}",
            std::bit_cast<std::uintptr_t>(game::vector_ptr_insert));

        result.bar_populate = follow_find(region,
            "E8 ? ? ? ? 48 C7 83 ? ? ? ? ? ? ? ? 48 8B C3");
        result.get_definition = follow_find(region,
            "E8 ? ? ? ? 0F B6 70");
        result.category_lookup = follow_find(region,
            "E8 ? ? ? ? 8B 4C 24 ? 44 8B CD 89 4C 24 ? 44 8B C6 48 8B C8 8B D7 E8");
        result.active_category = follow_find(region,
            "E8 ? ? ? ? 33 F6 48 8B F8");
        result.folder_voice = backtrack_prologue(memory::find(region,
            "8B 04 88 48 8B 5C 24"), 128);
        // IIDX34 rewrote the Music Select initializer and the close-category
        // call site. Prefer the older signatures when available, then fall back
        // to the IIDX34 forms used by 2026091600.
        if (auto old_close = memory::find(
            region, "E8 ? ? ? ? 66 C7 43 ? ? ? E8", true))
        {
            result.close_categories = reinterpret_cast<decltype(result.close_categories)>(
                memory::follow(old_close));
        }
        else
        {
            auto const close_call = memory::find(
                region,
                "BA 04 00 00 00 48 8B C8 [E8] ? ? ? ? E8 ? ? ? ? 48 8B 98");
            result.close_categories =
                reinterpret_cast<decltype(result.close_categories)>(
                    memory::follow(close_call));
        }

        if (auto old_init = memory::find(
            region, "48 83 EC 30 48 8B D9 89 11 E8", true))
        {
            result.music_select_init = backtrack_prologue(old_init, 32);
        }
        else
        {
            auto const init34 = memory::find(region, "48 8B F1 89 11 E8");
            result.music_select_init = backtrack_prologue(init34, 32);
        }
        result.clear_lamp = backtrack_prologue(memory::find(region,
            "48 6B CA ? 48 03 CE 48 8D 0C 4F"), 128);

        auto const badges = find_badge_targets(region);
        result.badge_weekly = badges[0];
        result.badge_featured = badges[1];
        result.badge_tournament = badges[2];
        result.badge_kac = badges[3];

        result.active_category_id_offset =
            *reinterpret_cast<std::uint32_t*>(memory::find(region,
                "89 88 [?] ? ? ? 48 8B 5C 24"));

        result.init_category_bar = follow_find<decltype(result.init_category_bar)>
            (region, "E8 ? ? ? ? FF C3 48 81 C7 ? ? ? ? 81 FB");
        result.get_music_entry = follow_find<decltype(result.get_music_entry)>
            (region, "E8 ? ? ? ? 0F BF B0");
        result.bar_insert_chart = follow_find<decltype(result.bar_insert_chart)>
            (region, "E8 ? ? ? ? 84 C0 75 ? FF C3 83 FB ? 7D");
        // Native Music Select sort call: (category, sort mode, bar-list mode).
        // Unique in both supplied IIDX33/34 DLLs. Resolve the relative CALL,
        // never reuse an RVA from another build. Missing signatures fail setup.
        result.sort_category = follow_find<decltype(result.sort_category)>
            (region, "8B D0 41 B0 01 48 8B CE [E8] ? ? ? ? 83 3E");
        OMNIFIX_FOLDER_TRACE("80th folder: native category sort at {:#x}",
            std::bit_cast<std::uintptr_t>(result.sort_category));
        result.get_score_data = follow_find<decltype(result.get_score_data)>
            (region, "E8 ? ? ? ? 85 C0 75 ? 8B 7C 24");

        // IIDX34 native Version-folder population helper.  The helper reads
        // music_entry+0x3DC and converts the raw version into a stock category
        // ID.  If synthetic version 80 reaches it, the song is inserted into
        // an extra native Version folder in addition to our shared category.
        // This pattern is intentionally optional so IIDX33 keeps its existing
        // working behaviour.
        if (auto const version_read = memory::find(region,
            "0F BF B9 DC 03 00 00 8B DA 48 8B F1 E8 ? ? ? ? 33 C9 8D 57 1A", true))
        {
            result.version_category_insert = backtrack_prologue(version_read, 32);
        }

        result.system_sounds = find_system_sounds(region);

        OMNIFIX_FOLDER_TRACE("80th folder: group populate at {:#x}",
            std::bit_cast<std::uintptr_t>(result.bar_populate));
        OMNIFIX_FOLDER_TRACE("80th folder: definition getter at {:#x}",
            std::bit_cast<std::uintptr_t>(result.get_definition));
        OMNIFIX_FOLDER_TRACE("80th folder: category lookup at {:#x}",
            std::bit_cast<std::uintptr_t>(result.category_lookup));
        OMNIFIX_FOLDER_TRACE("80th folder: active category at {:#x}",
            std::bit_cast<std::uintptr_t>(result.active_category));
        OMNIFIX_FOLDER_TRACE("80th folder: folder voice at {:#x}",
            std::bit_cast<std::uintptr_t>(result.folder_voice));
        OMNIFIX_FOLDER_TRACE("80th folder: category init at {:#x}",
            std::bit_cast<std::uintptr_t>(result.init_category_bar));
        OMNIFIX_FOLDER_TRACE("80th folder: music getter at {:#x}",
            std::bit_cast<std::uintptr_t>(result.get_music_entry));
        OMNIFIX_FOLDER_TRACE("80th folder: chart insertion at {:#x}",
            std::bit_cast<std::uintptr_t>(result.bar_insert_chart));
        OMNIFIX_FOLDER_TRACE("80th folder: clear lamp getter at {:#x}",
            std::bit_cast<std::uintptr_t>(result.clear_lamp));
        OMNIFIX_FOLDER_TRACE("80th folder: score data getter at {:#x}",
            std::bit_cast<std::uintptr_t>(result.get_score_data));
        if (result.version_category_insert)
        {
            OMNIFIX_FOLDER_TRACE(
                "80th folder: IIDX34 native Version-80 insertion guard at {:#x}",
                std::bit_cast<std::uintptr_t>(result.version_category_insert));
        }
        OMNIFIX_FOLDER_TRACE("80th folder: badge accessors weekly={:#x}, featured={:#x}, tournament={:#x}, kac={:#x}",
            std::bit_cast<std::uintptr_t>(result.badge_weekly),
            std::bit_cast<std::uintptr_t>(result.badge_featured),
            std::bit_cast<std::uintptr_t>(result.badge_tournament),
            std::bit_cast<std::uintptr_t>(result.badge_kac));

        return result;
    }

    template <typename header_type>
    auto parse_mdb_version_folder(std::span<const std::uint8_t> buffer,
        std::uint16_t target_version)
        -> std::array<std::vector<song_reference>, 2>
    {
        (void) target_version; // Kept for source/config compatibility.
        using index_type = typename header_type::index_type;

        auto const header = reinterpret_cast<const header_type*>(buffer.data());
        auto const header_size = sizeof(header_type);
        auto const index_size = sizeof(index_type);
        auto const entry_first = header_size +
            static_cast<std::size_t>(header->max_entries) * index_size;

        if (header->entries <= 0 || header->max_entries <= 0 ||
            entry_first >= buffer.size())
            throw std::runtime_error { "invalid music-data header" };

        auto const entry_size = (buffer.size() - entry_first) /
            static_cast<std::size_t>(header->entries);

        // IIDX 32/33: version field used by Altfix's Omni folder.
        auto constexpr ratings_offset = std::size_t { 0x3EC };
        auto constexpr chart_count = std::size_t { 10 };

        if (entry_size < ratings_offset + chart_count)
            throw std::runtime_error { "music-data entry is too small for v32 layout" };

        auto const directory = reinterpret_cast<const index_type*>
            (buffer.data() + header_size);
        auto result = std::array<std::vector<song_reference>, 2> {};
        auto seen = std::unordered_set<index_type> {};

        for (auto id = 0; id < header->max_entries; ++id)
        {
            auto const index = directory[id];

            if (index < 0 || seen.contains(index))
                continue;

            seen.insert(index);

            auto const entry = entry_first +
                static_cast<std::size_t>(index) * entry_size;

            if (entry + ratings_offset + chart_count > buffer.size())
                continue;

            // INFINITAS target rule:
            //   MID > 20000 and the last three digits are 200..499,
            //   plus the explicit additional MID 32761.
            // MID 33333 is explicitly excluded because it is the built-in
            // beginner tutorial song. The MDB version field is ignored.
            auto const suffix = id % 1000;

            if (id != 32761 &&
                (id == 33333 || id <= 20000 || suffix < 200 || suffix > 499))
                continue;

            auto const ratings = buffer.data() + entry + ratings_offset;
            auto sp_mask = std::uint8_t {};
            auto dp_mask = std::uint8_t {};

            for (auto difficulty = 0; difficulty < 5; ++difficulty)
            {
                if (ratings[difficulty] > 0)
                    sp_mask |= static_cast<std::uint8_t>(1u << difficulty);

                if (ratings[difficulty + 5] > 0)
                    dp_mask |= static_cast<std::uint8_t>(1u << difficulty);
            }

            // Native version folders contain one music bar per song. The bar
            // keeps a pointer to the full music entry, so the game can change
            // the displayed difficulty without requiring one bar per chart.
            if (sp_mask != 0)
            {
                auto levels = std::array<std::uint8_t, 5> {};
                std::copy_n(ratings, levels.size(), levels.begin());
                result[game::style_sp].push_back({ id, sp_mask, levels });
            }

            if (dp_mask != 0)
            {
                auto levels = std::array<std::uint8_t, 5> {};
                std::copy_n(ratings + 5, levels.size(), levels.begin());
                result[game::style_dp].push_back({ id, dp_mask, levels });
            }
        }

        return result;
    }

    auto load_folder_songs(std::string_view path, std::uint16_t target_version)
        -> std::array<std::vector<song_reference>, 2>
    {
        auto const buffer = avs2::file::read(path);

        if (buffer.size() < sizeof(bm2dx::mdb_common))
            throw std::runtime_error { "failed to read modified music data" };

        auto const common = reinterpret_cast<const bm2dx::mdb_common*>
            (buffer.data());

        if (std::memcmp(common->magic, "IIDX", 4) != 0)
            throw std::runtime_error { "modified music data has invalid magic" };

        if (!bm2dx::mdb_v32::is_supported(common->version))
            throw std::runtime_error {
                "80th folder currently requires IIDX music-data version 32, 33 or 34" };

        return parse_mdb_version_folder<bm2dx::mdb_v32>
            (buffer, target_version);
    }

    auto find_folder(int id) -> folder_variant*
    {
        if (!g_runtime)
            return nullptr;

        auto const it = std::ranges::find_if(g_runtime->folders,
            [id] (auto const& folder) { return folder.id == id; });

        return it != g_runtime->folders.end() ? &*it: nullptr;
    }

    auto current_play_style(game::CCategoryGameData* base) -> int
    {
        if (!g_runtime || !base)
            return game::style_sp;

        // The game stores its active category index at +0x29860 and the
        // current SP/DP index in the adjacent field. The native category
        // lookup uses the same field to resolve its special "all styles" mode.
        auto const style = *reinterpret_cast<const std::int32_t*>(
            reinterpret_cast<const std::uint8_t*>(base) +
            g_runtime->target.active_category_id_offset + sizeof(std::int32_t));
        return style == game::style_dp ? game::style_dp: game::style_sp;
    }

    auto find_folder(int id, int play_style) -> folder_variant*
    {
        if (!g_runtime || (play_style != game::style_sp &&
            play_style != game::style_dp))
            return nullptr;

        auto const it = std::ranges::find_if(g_runtime->folders,
            [id, play_style] (auto const& folder) {
                return folder.id == id && folder.play_style == play_style;
            });

        return it != g_runtime->folders.end() ? &*it: nullptr;
    }

    auto copy_folder_state(folder_variant const& from,
        folder_variant& to) -> void
    {
        to.category.is_open = from.category.is_open;
        to.category.active_bar_index = from.category.active_bar_index;
        to.category.header.active_difficulty =
            from.category.header.active_difficulty;
        to.category.header.string = from.category.header.string;
    }

    auto sync_folder_style(int play_style) -> void
    {
        if (!g_runtime || (play_style != game::style_sp &&
            play_style != game::style_dp))
            return;

        if (g_runtime->active_folder_style_valid &&
            g_runtime->active_folder_style != play_style)
        {
            auto const previous = g_runtime->active_folder_style;
            copy_folder_state(g_runtime->folders[previous],
                g_runtime->folders[play_style]);
        }

        g_runtime->active_folder_style = play_style;
        g_runtime->active_folder_style_valid = true;
    }

    auto close_infinitas_folder_views() -> void
    {
        if (!g_runtime)
            return;

        for (auto& folder: g_runtime->folders)
        {
            folder.category.is_open = false;
            folder.category.active_bar_index = 0;
        }
    }

    auto select_initial_difficulty(std::uint8_t mask, int requested) -> int
    {
        if (mask == 0)
            return -1;

        if (requested < 0 || requested > 4)
            requested = 1; // NORMAL is the safest fallback.

        if ((mask & (1u << requested)) != 0)
            return requested;

        // Match normal folder behaviour as closely as possible: prefer the
        // nearest lower difficulty, then the nearest higher difficulty.
        for (auto difficulty = requested - 1; difficulty >= 0; --difficulty)
        {
            if ((mask & (1u << difficulty)) != 0)
                return difficulty;
        }

        for (auto difficulty = requested + 1; difficulty < 5; ++difficulty)
        {
            if ((mask & (1u << difficulty)) != 0)
                return difficulty;
        }

        return -1;
    }

    auto remap_score_difficulty(int play_style, int difficulty) -> int
    {
        return difficulty +
            (play_style == game::style_dp ? 5: 0);
    }

    auto calculate_clear_lamps(folder_variant& folder) -> void
    {
        if (folder.clear_lamps_valid)
            return;

        for (auto player = 0; player < 2; ++player)
        {
            for (auto difficulty = 0; difficulty < 5; ++difficulty)
            {
                auto aggregate = game::clear_full_combo;
                auto has_chart = false;

                for (auto const& song: folder.songs)
                {
                    if ((song.difficulty_mask & (1u << difficulty)) == 0)
                        continue;

                    has_chart = true;
                    auto lamp = game::clear_no_play;
                    auto const result = g_runtime->target.get_score_data(
                        player, folder.play_style, song.music_id,
                        remap_score_difficulty(folder.play_style, difficulty),
                        nullptr, nullptr, &lamp);

                    if (result != 0)
                        lamp = game::clear_no_play;

                    if (static_cast<int>(lamp) < static_cast<int>(aggregate))
                        aggregate = lamp;
                }

                folder.clear_lamps[player][difficulty] = has_chart ?
                    aggregate: game::clear_no_play;
            }
        }

        folder.clear_lamps_valid = true;
        OMNIFIX_FOLDER_TRACE(
            "80th folder: recalculated clear lamps for category {}",
            folder.id);
    }

    auto insert_charts(folder_variant& folder) -> bool
    {
        if (folder.inserted)
            return true;

        auto& category = folder.category;
        auto inserted = 0;

        // Build the entries first. The game's comparator consumes chart_bar
        // objects, not song_reference records; do not pre-sort by MID.
        for (auto const& song: folder.songs)
        {
            auto const music = g_runtime->target.get_music_entry(song.music_id);

            if (!music)
            {
                avs2::log::misc(
                    "80th folder: failed to get music id {}", song.music_id);
                continue;
            }

            auto difficulty = select_initial_difficulty(
                song.difficulty_mask, category.header.active_difficulty);

            if (difficulty < 0)
                continue;

            auto constexpr flag = 0;
            auto category_ptr = &category;
            auto params = std::to_array<const void*>
                ({ &music, &category_ptr, &difficulty, &flag, &flag });

            if (!g_runtime->target.bar_insert_chart(params.data(),
                difficulty, folder.play_style, folder.id))
            {
                avs2::log::misc(
                    "80th folder: failed to insert music {} at difficulty {}",
                    song.music_id, difficulty);
                continue;
            }

            ++inserted;
            ++category.chart_count;
        }

        folder.inserted = true;

        auto bars = 0;
        category.bar_list[bars++] = {
            .type = game::bar_type_header,
            .data = &category.header,
        };

        if (category.chart_count > 1)
        {
            category.bar_list[bars++] = {
                .type = game::bar_type_random,
                .data = &category.random,
            };
        }

        for (auto i = 0; i < category.chart_count; ++i)
        {
            category.bar_list[bars++] = {
                .type = game::bar_type_chart,
                .data = &category.chart_list[i],
            };
        }

        category.bar_count = bars;

        // Sort the displayed bar references (not chart_list storage), preserving
        // the header/random bars and all chart pointers. Mode 2 is native
        // LEVEL -> music-name ordering. Subsequent Music Select sort/difficulty
        // changes use the normal game path; mode 0 reads definition.default_sort.
        g_runtime->target.sort_category(&category, 2, true);

        OMNIFIX_FOLDER_TRACE(
            "80th folder: category {} inserted {} of {} songs",
            folder.id, inserted, folder.songs.size());

        return inserted > 0;
    }

    auto hook_version_category_insert(void* music_entry, int difficulty) -> void
    {
        if (music_entry)
        {
            auto constexpr mid_offset = std::uintptr_t { 0x67C };
            auto constexpr version_offset = std::uintptr_t { 0x3DC };
            auto const address =
                reinterpret_cast<std::uintptr_t>(music_entry);

            // IIDX34 stores the song MID directly in the live music entry at
            // +0x67C (the game's own MID -> entry resolver validates this field).
            // Check the whitelist by MID here so exclusion does not depend on
            // whether the later live-version rewrite to 80 has happened yet.
            auto const mid = *reinterpret_cast<const std::int32_t*>(
                address + mid_offset);

            if (g_runtime &&
                g_runtime->infinitas_whitelist.contains(mid))
            {
                static std::size_t logged {};
                if (logged < 8)
                {
                    ++logged;
                    OMNIFIX_FOLDER_TRACE(
                        "80th folder: excluded MID {} from native Version folder ({}/8)",
                        mid, logged);
                }
                return;
            }

            // Fallback for already-reclassified entries.
            auto const version = *reinterpret_cast<const std::int16_t*>(
                address + version_offset);
            if (g_runtime &&
                version == static_cast<std::int16_t>(
                    g_runtime->settings.music_version))
                return;
        }

        g_runtime->hook_version_category_insert.call<void>(music_entry, difficulty);
    }

    auto hook_clear_lamp(game::CCategoryGameData* base, int category,
        int difficulty, int player) -> int
    {
        auto folder = find_folder(category, current_play_style(base));

        if (!folder)
        {
            return g_runtime->hook_clear_lamp.call<int>(
                base, category, difficulty, player);
        }

        if (player < 0 || player > 1 || difficulty < 0 || difficulty > 4)
            return game::clear_no_play;

        calculate_clear_lamps(*folder);
        return static_cast<int>(folder->clear_lamps[player][difficulty]);
    }

    auto hook_badge_weekly(game::CCategoryGameData* base, int category) -> int
    {
        if (find_folder(category, current_play_style(base)))
            return 0;
        return g_runtime->hook_badge_weekly.call<int>(base, category);
    }

    auto hook_badge_featured(game::CCategoryGameData* base, int category) -> int
    {
        if (find_folder(category, current_play_style(base)))
            return 0;
        return g_runtime->hook_badge_featured.call<int>(base, category);
    }

    auto hook_badge_tournament(game::CCategoryGameData* base, int category) -> int
    {
        if (find_folder(category, current_play_style(base)))
            return 0;
        return g_runtime->hook_badge_tournament.call<int>(base, category);
    }

    auto hook_badge_kac(game::CCategoryGameData* base, int category) -> int
    {
        if (find_folder(category, current_play_style(base)))
            return 0;
        return g_runtime->hook_badge_kac.call<int>(base, category);
    }

    auto hook_get_definition(game::CCategoryGameData* base, int id)
        -> game::category_definition*
    {
        if (auto folder = find_folder(id, current_play_style(base)))
            return &folder->definition;

        return g_runtime->hook_get_definition.call<game::category_definition*>
            (base, id);
    }

    auto hook_bar_populate(game::category_group* group) -> void*
    {
        auto const result = g_runtime->hook_bar_populate.call<void*>(group);

        if (group->group_id != game::group_version &&
            group->group_id != game::group_all)
            return result;

        for (auto& folder: g_runtime->folders)
        {
            if (!insert_charts(folder) || folder.category.chart_count == 0)
                continue;

            auto& destination = folder.play_style == game::style_sp ?
                group->sp_bars: group->dp_bars;

            if (group->group_id == game::group_version)
            {
                destination.push_back(&folder.category);
            }
            else
            {
                auto* insert_position = destination.end;
                auto found_version_category = false;

                for (auto* current = destination.data; current < destination.end; ++current)
                {
                    auto* category = *current;
                    if (!category)
                        continue;

                    auto* definition = g_runtime->hook_get_definition.call<
                        game::category_definition*>(nullptr, category->id);

                    if (!definition || !definition->texture_hover_header ||
                        !definition->texture_bar_bg)
                        continue;

                    if (std::strcmp(definition->texture_hover_header, "ss_version") == 0 &&
                        std::strcmp(definition->texture_bar_bg, "so_b_version") == 0)
                    {
                        insert_position = current + 1;
                        found_version_category = true;
                    }
                }

                if (!destination.insert(insert_position, &folder.category))
                {
                    avs2::log::misc(
                        "80th folder: middle insertion failed for category {}; falling back to push_back",
                        folder.id);

                    destination.push_back(&folder.category);
                }
                else
                {
                    OMNIFIX_FOLDER_TRACE(
                        "80th folder: category {} inserted {} last native Version category in ALL group",
                        folder.id,
                        found_version_category ? "after" : "at fallback position after");
                }
            }

            calculate_clear_lamps(folder);
        }

        return result;
    }

    auto hook_category_lookup(game::CCategoryGameData* base,
        int category, int a3) -> void*
    {
        // a3 is normally the SP/DP index. The native "all styles" path uses
        // 3 and resolves to the current style stored alongside the active ID.
        auto const style = a3 == 3 ? current_play_style(base): a3;
        if (auto folder = find_folder(category, style))
        {
            sync_folder_style(style);
            return &folder->category;
        }

        // Category IDs and MDB music versions are separate namespaces.
        // In IIDX34, native category 80 is an EVENT folder, not version 80.
        // Only our explicit custom IDs may resolve to the synthetic folders.
        return g_runtime->hook_category_lookup.call<void*>(base, category, a3);
    }

    auto hook_active_category(game::CCategoryGameData* base)
        -> game::category*
    {
        auto const style = current_play_style(base);
        auto const id = *reinterpret_cast<std::int32_t*>
            (reinterpret_cast<std::uint8_t*>(base) +
             g_runtime->target.active_category_id_offset);

        if (auto folder = find_folder(id, style))
        {
            sync_folder_style(style);
            return &folder->category;
        }

        // Both views have the same native ID. Clear their cached open state
        // only after the game has actually selected a different category.
        close_infinitas_folder_views();
        g_runtime->active_folder_style_valid = false;

        return g_runtime->hook_active_category.call<game::category*>(base);
    }

    auto hook_folder_voice(game::CCustomizeGameData* base, int category) -> int
    {
        auto const result = g_runtime->hook_folder_voice.call<int>(base, category);

        if (!find_folder(category))
            return result;

        auto const key = std::string { g_runtime->settings.folder_voice };
        auto const it = g_runtime->target.system_sounds.find(key);

        if (it != g_runtime->target.system_sounds.end())
            return it->second;

        return result;
    }

    auto hook_close_categories(game::CCategoryGameData* base,
        int category_flags) -> void*
    {
        // The second argument is a category flag mask, not SP/DP mode. This
        // routine also runs during style changes, so preserve folder state here.
        // hook_active_category clears it when the selected ID leaves the folder.
        return g_runtime->hook_close_categories.call<void*>(base, category_flags);
    }

    auto refresh_entry_mid_map(std::string_view reason) -> void
    {
        if (!g_runtime)
            return;

        g_runtime->entry_to_mid.clear();

        for (auto const music_id: g_runtime->infinitas_whitelist)
        {
            auto const entry = g_runtime->target.get_music_entry(music_id);

            if (entry)
                g_runtime->entry_to_mid.insert_or_assign(
                    reinterpret_cast<std::uintptr_t>(entry), music_id);
        }

        OMNIFIX_FOLDER_TRACE(
            "80th folder: mapped {} live entries from {} INFINITAS whitelist MIDs ({})",
            g_runtime->entry_to_mid.size(),
            g_runtime->infinitas_whitelist.size(), reason);
    }

    auto assign_infinitas_live_version(std::string_view reason) -> void
    {
        if (!g_runtime)
            return;

        // Hybrid 2.8/2.9.7 strategy: let the game's own native Version-folder
        // population logic perform the separation. Before Music Select builds
        // those folders, reclassify only the MID-rule INFINITAS entries as
        // version 80. This avoids editing native category vectors after they
        // have been constructed.
        auto constexpr version_offset = std::uintptr_t { 0x3DC };
        auto const target_version = static_cast<std::int16_t>(
            g_runtime->settings.music_version);
        auto changed = std::size_t {};
        auto failed = std::size_t {};

        for (auto const music_id: g_runtime->infinitas_whitelist)
        {
            auto* entry = g_runtime->target.get_music_entry(music_id);
            if (!entry)
            {
                ++failed;
                continue;
            }

            auto* version = reinterpret_cast<std::int16_t*>(
                reinterpret_cast<std::uintptr_t>(entry) + version_offset);

            DWORD old_protect {};
            if (!VirtualProtect(version, sizeof(*version), PAGE_READWRITE,
                &old_protect))
            {
                ++failed;
                continue;
            }

            if (*version != target_version)
            {
                *version = target_version;
                ++changed;
            }

            DWORD ignored {};
            VirtualProtect(version, sizeof(*version), old_protect, &ignored);
        }

        OMNIFIX_FOLDER_TRACE(
            "80th folder hybrid: assigned live version {} to {} INFINITAS entries; {} unchanged/failed ({})",
            static_cast<int>(target_version), changed,
            g_runtime->infinitas_whitelist.size() - changed, reason);

        if (failed != 0)
        {
            avs2::log::misc(
                "80th folder hybrid: failed to resolve or write {} INFINITAS entries ({})",
                failed, reason);
        }
    }

    auto hook_music_select_init(void* a1, int a2) -> void*
    {
        close_infinitas_folder_views();
        g_runtime->active_folder_style_valid = false;

        for (auto& folder: g_runtime->folders)
        {
            g_runtime->target.init_category_bar(&folder.category, folder.id);
            folder.inserted = false;
            folder.clear_lamps_valid = false;
            folder.category.chart_count = 0;
        }

        // Must happen before the original initializer populates stock Version
        // categories. The game will then place these entries only under the
        // synthetic version-80/INFINITAS category instead of their old version.
        assign_infinitas_live_version("before Music Select initialization");

        auto const result = g_runtime->hook_music_select_init.call<void*>(a1, a2);

        // The live music database is not guaranteed to be ready when Omnifix
        // itself is loaded. Resolve the selected MID set again after the game's
        // Music Select initializer has completed, so title-colour and ssm_80th
        // hooks compare against the exact entry pointers used by this session.
        refresh_entry_mid_map("after Music Select initialization");

        return result;
    }

    auto follow_relative_call(std::uint8_t* call) -> std::uint8_t*
    {
        if (!call || *call != 0xE8)
            return nullptr;

        auto const displacement =
            *reinterpret_cast<const std::int32_t*>(call + 1);
        return call + 5 + displacement;
    }

    auto find_all_native_color_calls(std::span<std::uint8_t> region,
        std::string_view name, std::string_view anchor_pattern,
        std::uint8_t* color_constructor,
        bool allow_missing_preview_route = false)
        -> std::array<std::uint8_t*, 3>
    {
        auto const anchors = find_exact(region, name, anchor_pattern, 1);
        auto const anchor = anchors.front();
        auto calls = std::array<std::uint8_t*, 3> {};
        auto count = std::size_t {};

        // Each native title renderer has three mutually exclusive colour
        // routes before they converge again:
        //   0: Version 34 preview whitelist
        //   1: current-version (Version 33) green
        //   2: normal/default version colour
        // The previous implementation only hooked route 2. Scan around the
        // version comparison and hook every call to the shared RGBA
        // constructor so INFINITAS purple can override all three routes.
        auto const begin = anchor >= region.data() + 128
            ? anchor - 128
            : region.data();
        auto const finish = std::min(anchor + 192,
            region.data() + region.size());

        for (auto cursor = begin; cursor + 5 <= finish; ++cursor)
        {
            if (*cursor != 0xE8 ||
                follow_relative_call(cursor) != color_constructor)
                continue;

            if (count >= calls.size())
                throw std::runtime_error { std::format(
                    "80th folder '{}' found more than three native colour calls",
                    name) };

            calls[count++] = cursor;
        }

        if (count == 2 && allow_missing_preview_route)
        {
            // 2026042200 predates the separate Version 34 preview route in
            // several title renderers/layouts. Its two constructor calls are the
            // current-version route followed by the normal/default route.
            // Preserve the logical route indexes used by the hook callbacks:
            //   0 = preview (absent), 1 = current version, 2 = default.
            calls[2] = calls[1];
            calls[1] = calls[0];
            calls[0] = nullptr;

            OMNIFIX_FOLDER_TRACE(
                "80th folder: {} has no native preview colour route; "
                "using version={:#x}, default={:#x}",
                name,
                std::bit_cast<std::uintptr_t>(calls[1]),
                std::bit_cast<std::uintptr_t>(calls[2]));
        }
        else if (count != calls.size())
        {
            throw std::runtime_error { std::format(
                "80th folder '{}' expected three native colour calls, found {}",
                name, count) };
        }

        OMNIFIX_FOLDER_TRACE(
            "80th folder: {} colour calls preview={:#x}, version={:#x}, default={:#x}",
            name,
            std::bit_cast<std::uintptr_t>(calls[0]),
            std::bit_cast<std::uintptr_t>(calls[1]),
            std::bit_cast<std::uintptr_t>(calls[2]));

        return calls;
    }

    auto find_title_route_merge(std::array<std::uint8_t*, 3> const& calls,
        std::string_view name) -> std::pair<std::uint8_t*, std::uint8_t>
    {
        auto const version_call = calls[1];
        if (!version_call)
            throw std::runtime_error { std::format(
                "80th folder '{}' has no Version route call", name) };

        // The Version route creates its RGBA value, copies it, writes route
        // discriminator 2 to [rsp+offset], then jumps to the common renderer.
        // Default route keeps the discriminator initialized to 1. At the
        // common target we can safely change only INFINITAS entries from 2 to
        // 1, retaining the purple RGBA but avoiding Version 33's green
        // material/palette treatment.
        auto const begin = version_call + 5;
        auto const finish = version_call + 64;
        std::uint8_t stack_offset = 0;
        std::uint8_t* jump = nullptr;

        for (auto cursor = begin; cursor + 8 <= finish; ++cursor)
        {
            if (cursor[0] == 0xC7 && cursor[1] == 0x44 && cursor[2] == 0x24 &&
                cursor[4] == 0x02 && cursor[5] == 0x00 &&
                cursor[6] == 0x00 && cursor[7] == 0x00)
            {
                stack_offset = cursor[3];
                auto after = cursor + 8;
                if (after[0] == 0xEB)
                    jump = after + 2 + static_cast<std::int8_t>(after[1]);
                else if (after[0] == 0xE9)
                    jump = after + 5 +
                        *reinterpret_cast<const std::int32_t*>(after + 1);
                break;
            }
        }

        if (!jump || !stack_offset)
            throw std::runtime_error { std::format(
                "80th folder '{}' could not locate Version route merge", name) };

        OMNIFIX_FOLDER_TRACE(
            "80th folder: {} route merge={:#x}, stack offset={:#x}",
            name, std::bit_cast<std::uintptr_t>(jump), stack_offset);
        return { jump, stack_offset };
    }

    auto find_extended_title_route_merge(std::uint8_t* version_call,
        std::string_view name)
        -> std::tuple<std::uint8_t*, std::uint8_t, std::uint8_t>
    {
        if (!version_call)
            throw std::runtime_error { std::format(
                "80th folder '{}' has no Version route call", name) };

        // Extended renderers store route 2 in a stack slot or in [RBP/RSI+4],
        // then jump to a common merge point. Identify the field from the
        // instruction and change only the INFINITAS Version route there.
        auto const begin = version_call + 5;
        auto const finish = version_call + 64;
        std::uint8_t stack_offset = 0;
        std::uint8_t route_register = 0;
        std::uint8_t* jump = nullptr;

        for (auto cursor = begin; cursor + 13 <= finish; ++cursor)
        {
            auto instruction_length = std::size_t {};
            if (cursor[0] == 0xC7 && cursor[1] == 0x45 &&
                cursor[2] == 0x04 && cursor[3] == 0x02 &&
                cursor[4] == 0x00 && cursor[5] == 0x00 && cursor[6] == 0x00)
            {
                route_register = 1; // [RBP+4]
                instruction_length = 7;
            }
            else if (cursor[0] == 0xC7 && cursor[1] == 0x46 &&
                cursor[2] == 0x04 && cursor[3] == 0x02 &&
                cursor[4] == 0x00 && cursor[5] == 0x00 && cursor[6] == 0x00)
            {
                route_register = 2; // [RSI+4]
                instruction_length = 7;
            }
            else if (cursor[0] == 0xC7 && cursor[1] == 0x44 &&
                cursor[2] == 0x24 && cursor[4] == 0x02 &&
                cursor[5] == 0x00 && cursor[6] == 0x00 && cursor[7] == 0x00)
            {
                route_register = 0; // [RSP+offset]
                stack_offset = cursor[3];
                instruction_length = 8;
            }
            if (!instruction_length)
                continue;

            auto const after = cursor + instruction_length;
            if (after[0] == 0xEB)
                jump = after + 2 + static_cast<std::int8_t>(after[1]);
            else if (after[0] == 0xE9)
                jump = after + 5 +
                    *reinterpret_cast<const std::int32_t*>(after + 1);
            break;
        }

        if (!jump || (route_register == 0 && !stack_offset))
            throw std::runtime_error { std::format(
                "80th folder '{}' could not locate extended Version route merge",
                name) };

        if (route_register == 1)
            OMNIFIX_FOLDER_TRACE(
                "80th folder: {} material route merge={:#x}, field=[RBP+4]",
                name, std::bit_cast<std::uintptr_t>(jump));
        else if (route_register == 2)
            OMNIFIX_FOLDER_TRACE(
                "80th folder: {} material route merge={:#x}, field=[RSI+4]",
                name, std::bit_cast<std::uintptr_t>(jump));
        else
            OMNIFIX_FOLDER_TRACE(
                "80th folder: {} material route merge={:#x}, stack offset={:#x}",
                name, std::bit_cast<std::uintptr_t>(jump), stack_offset);
        return { jump, stack_offset, route_register };
    }

    // Match a complete known renderer anchor; reject ambiguous/unknown builds.
    auto detect_title_version(std::span<std::uint8_t> region) -> int
    {
        auto const v33 = find_all(region,
            "66 83 BB DC 03 00 00 21 75 36 "
            "F3 0F 10 1D ? ? ? ? F3 0F 10 15 ? ? ? ? "
            "F3 0F 10 0D ? ? ? ?");
        auto const v34 = find_all(region,
            "66 83 BB DC 03 00 00 22 75 36 "
            "F3 0F 10 1D ? ? ? ? F3 0F 10 15 ? ? ? ? "
            "F3 0F 10 0D ? ? ? ?");
        if (v33.size() == 1 && v34.empty())
            return 33;
        if (v34.size() == 1 && v33.empty())
            return 34;
        throw std::runtime_error { "unknown or ambiguous IIDX title renderer layout" };
    }

    auto is_readable(std::uintptr_t address, std::size_t size) -> bool;

    // Search rebuilds its title/artist dictionaries from playable songs in
    // category MID / 1000 + 26. INFINITAS songs have been moved to our folder,
    // so the original category-membership check would discard them.
    auto hook_search_category(SafetyHookContext& ctx) -> void
    {
        if (!g_runtime || !is_readable(ctx.r12 + 8, sizeof(std::int32_t)))
            return;
        auto const mid = *reinterpret_cast<const std::int32_t*>(ctx.r12 + 8);
        if (g_runtime->infinitas_whitelist.contains(mid))
            ctx.r14 = static_cast<std::uint32_t>(g_runtime->settings.category_id);
    }

    auto setup_search_category_hooks(std::span<std::uint8_t> region) -> void
    {
        // Reviewed IIDX34 layout only; IIDX33 retains its existing behavior.
        if (detect_title_version(region) != 34)
            return;
        auto const sites = find_all(region,
            "B8 D3 4D 62 10 41 F7 6C 24 08 44 8B EA 41 C1 FD 06 "
            "41 8B C5 C1 E8 1F 44 03 E8 45 8D 75 1A "
            "E8 ? ? ? ? 44 8B F8 40 32 ED 33 F6 "
            "E8 ? ? ? ? C6 44 24 30 00 C6 44 24 28 00 "
            "44 89 74 24 20 45 8B CF 44 8B C6 41 8B 54 24 08 "
            "48 8B C8 E8 ? ? ? ? 84 C0 0F 94 C0 40 0A E8 "
            "FF C6 83 FE 05 7C CA");
        if (sites.size() != 2)
            throw std::runtime_error { "expected two IIDX34 search category sites" };
        // Hook after LEA R14D,[R13+26], before the current-style getter.
        // R12 points at MusicItem; its +8 field is the MID. R13 stays intact
        // for the native first-style fallback. All native eligibility checks
        // and dictionary insertion logic remain unchanged.
        std::array<SafetyHookMid, 2> hooks {};
        for (std::size_t i = 0; i < hooks.size(); ++i)
        {
            hooks[i] = safetyhook::create_mid(sites[i] + 30, &hook_search_category);
            if (!hooks[i])
                throw std::runtime_error { "could not install search category hook" };
        }
        g_runtime->hook_search_category = std::move(hooks);
        avs2::log::misc("80th folder: search category fix v1 enabled (title/artist)");
    }

    auto title_pattern(std::string_view pattern, int version) -> std::string
    {
        auto result = std::string { pattern };
        if (version == 33)
        {
            if (auto const pos = result.find("00 22 75"); pos != std::string::npos)
                result.replace(pos, 8, "00 21 75");
            if (auto const pos = result.find("49 8B 46 30"); pos != std::string::npos)
                result.replace(pos, 11, "49 8B 46 38");
        }
        return result;
    }

    auto scan_title_color_targets(std::span<std::uint8_t> region,
        std::uint8_t& layout3_entry_offset)
        -> std::array<std::array<std::uint8_t*, 3>, 5>
    {
        auto const version = detect_title_version(region);
        layout3_entry_offset = version == 33 ? 0x38 : 0x30;
        OMNIFIX_FOLDER_TRACE("80th folder: title compatibility: IIDX {}, layout 3 entry offset {:#x}",
            version, layout3_entry_offset);
        auto const color_constructor = find_exact(region,
            "RGBA float constructor",
            "F3 0F 10 44 24 28 48 8B C1 F3 0F 11 41 0C "
            "F3 0F 11 09 F3 0F 11 51 04 F3 0F 11 59 08 C3",
            1).front();

        OMNIFIX_FOLDER_TRACE(
            "80th folder: RGBA constructor at {:#x}",
            std::bit_cast<std::uintptr_t>(color_constructor));

        return {
            find_all_native_color_calls(region, "title layout RBX",
                title_pattern("66 83 BB DC 03 00 00 22 75 36 "
                "F3 0F 10 1D ? ? ? ? F3 0F 10 15 ? ? ? ? "
                "F3 0F 10 0D ? ? ? ?", version),
                color_constructor,
                true),
            find_all_native_color_calls(region, "title layout [RSI]",
                title_pattern("48 8B 06 66 83 B8 DC 03 00 00 22 75 ?", version),
                color_constructor,
                true),
            find_all_native_color_calls(region, "title layout [R14+28]",
                title_pattern("49 8B 46 28 66 83 B8 DC 03 00 00 22 75 ?", version),
                color_constructor,
                true),
            find_all_native_color_calls(region, "title layout R14 dynamic",
                title_pattern("49 8B 46 30 66 83 B8 DC 03 00 00 22 75 ?", version),
                color_constructor,
                true),
            find_all_native_color_calls(region, "title layout [R14+10]",
                title_pattern("49 8B 46 10 66 83 B8 DC 03 00 00 22 75 ?", version),
                color_constructor,
                true),
        };
    }

    auto scan_extended_title_color_targets(std::span<std::uint8_t> region,
        std::array<std::uint8_t*, 3>& route_merges,
        std::array<std::uint8_t, 3>& route_stack_offsets,
        std::array<std::uint8_t, 3>& route_registers,
        std::array<std::uint8_t, 2>& entry_registers)
        -> std::array<std::array<std::uint8_t*, 3>, 3>
    {
        auto const version = detect_title_version(region);
        auto const color_constructor = find_exact(region,
            "RGBA float constructor (extended title)",
            "F3 0F 10 44 24 28 48 8B C1 F3 0F 11 41 0C "
            "F3 0F 11 09 F3 0F 11 51 04 F3 0F 11 59 08 C3",
            1).front();

        auto result = std::array<std::array<std::uint8_t*, 3>, 3> {};

        auto common_anchors = find_all(region,
            title_pattern("66 83 BB DC 03 00 00 22 75 47", version));
        // IIDX34 moved one shared title renderer's live entry from RBX to
        // RDI. Match both complete anchors and remember the entry register;
        // the style record's register is decoded independently below.
        for (auto const anchor: find_all(region,
            title_pattern("66 83 BF DC 03 00 00 22 75 47", version)))
            common_anchors.push_back(anchor);
        std::ranges::sort(common_anchors);

        if (common_anchors.empty() || common_anchors.size() > 2)
            throw std::runtime_error { std::format(
                "80th folder shared title renderer expected 1-2 matches, found {}",
                common_anchors.size()) };

        for (auto index = std::size_t {}; index < common_anchors.size(); ++index)
        {
            // Use the exact anchor address as a one-byte bracket pattern cannot
            // be passed to find_exact directly. Scan the same local window.
            auto const anchor = common_anchors[index];
            entry_registers[index] = anchor[2] == 0xBF ? 1 : 0;
            OMNIFIX_FOLDER_TRACE(
                "80th folder: shared renderer {} anchor={:#x}, entry={}",
                index, std::bit_cast<std::uintptr_t>(anchor),
                entry_registers[index] == 1 ? "RDI" : "RBX");
            auto calls = std::array<std::uint8_t*, 3> {};
            auto count = std::size_t {};
            auto const begin = anchor >= region.data() + 128 ? anchor - 128 : region.data();
            auto const finish = std::min(anchor + 192, region.data() + region.size());
            for (auto cursor = begin; cursor + 5 <= finish; ++cursor)
            {
                if (*cursor == 0xE8 && follow_relative_call(cursor) == color_constructor)
                {
                    if (count >= calls.size())
                        throw std::runtime_error { "80th folder shared renderer has too many colour calls" };
                    calls[count++] = cursor;
                }
            }
            if (count == 2)
            {
                // 2026042200 has no separate Version 34 preview colour route.
                // Keep callback route indexes stable: preview=null, version=0, default=1.
                calls[2] = calls[1];
                calls[1] = calls[0];
                calls[0] = nullptr;
                OMNIFIX_FOLDER_TRACE(
                    "80th folder: shared renderer {} has no native preview colour route; using version={:#x}, default={:#x}",
                    index,
                    std::bit_cast<std::uintptr_t>(calls[1]),
                    std::bit_cast<std::uintptr_t>(calls[2]));
            }
            else if (count != calls.size())
                throw std::runtime_error { std::format(
                    "80th folder shared renderer {} expected two or three colour calls, found {}",
                    index, count) };
            result[index] = calls;
            auto const [merge, stack_offset, route_register] =
                find_extended_title_route_merge(calls[1],
                    std::format("shared renderer {}", index));
            route_merges[index] = merge;
            route_stack_offsets[index] = stack_offset;
            route_registers[index] = route_register;
        }

        auto const text_mu_anchor = find_exact(region,
            "extended text_mu title renderer",
            title_pattern("48 8B 07 66 83 B8 DC 03 00 00 22 75 ?", version), 1).front();
        {
            auto calls = std::array<std::uint8_t*, 3> {};
            auto count = std::size_t {};
            auto const begin = text_mu_anchor >= region.data() + 128 ? text_mu_anchor - 128 : region.data();
            auto const finish = std::min(text_mu_anchor + 192, region.data() + region.size());
            for (auto cursor = begin; cursor + 5 <= finish; ++cursor)
            {
                if (*cursor == 0xE8 && follow_relative_call(cursor) == color_constructor)
                {
                    if (count >= calls.size())
                        throw std::runtime_error { "80th folder text_mu has too many colour calls" };
                    calls[count++] = cursor;
                }
            }
            if (count == 2)
            {
                calls[2] = calls[1];
                calls[1] = calls[0];
                calls[0] = nullptr;
                OMNIFIX_FOLDER_TRACE(
                    "80th folder: text_mu has no native preview colour route; using version={:#x}, default={:#x}",
                    std::bit_cast<std::uintptr_t>(calls[1]),
                    std::bit_cast<std::uintptr_t>(calls[2]));
            }
            else if (count != calls.size())
                throw std::runtime_error { std::format(
                    "80th folder text_mu expected two or three colour calls, found {}", count) };
            result[2] = calls;
            auto const [merge, stack_offset, route_register] =
                find_extended_title_route_merge(calls[1], "text_mu");
            route_merges[2] = merge;
            route_stack_offsets[2] = stack_offset;
            route_registers[2] = route_register;
        }

        return result;
    }

    auto find_phase2_alternative(std::span<std::uint8_t> region,
        std::string_view name, std::initializer_list<std::string_view> patterns)
        -> std::uint8_t*
    {
        auto matches = std::vector<std::uint8_t*> {};

        for (auto const pattern: patterns)
        {
            for (auto const address: find_all(region, pattern))
            {
                if (std::ranges::find(matches, address) == matches.end())
                    matches.push_back(address);
            }
        }

        if (matches.size() != 1)
        {
            throw std::runtime_error { std::format(
                "80th folder pattern '{}' expected 1 match, found {}",
                name, matches.size()) };
        }

        OMNIFIX_FOLDER_TRACE("80th folder: {} at {:#x}",
            name, std::bit_cast<std::uintptr_t>(matches.front()));
        return matches.front();
    }

    auto scan_subscreen_call_targets(std::span<std::uint8_t> region)
        -> std::array<std::uint8_t*, 2>
    {
        // Hook the final texture setter call, not the version-indexed table
        // lookup. At the call site R8 already contains the selected texture
        // name, so replacing only R8 for a verified Version-80 music entry
        // leaves every other folder and every stock table lookup untouched.
        auto const rbx_call = find_exact(region,
            "Lightning texture setter call (RDI entry)",
            "4C 8B C3 48 8D 15 ? ? ? ? 48 8B C8 [E8] ? ? ? ?",
            1).front();

        auto const r8_call = find_phase2_alternative(region,
            "Lightning texture setter call (R14 entry)", {
                // 2026042200 / 2026062900: resource name and object are loaded
                // after the version table lookup.
                "4E 8B 04 C0 48 8D 15 ? ? ? ? 49 8B 0E [E8] ? ? ? ? 45 33 C0",
                // 2026072100: the resource name and object are prepared before
                // the 34th-song branch, so the call follows the lookup directly.
                "4E 8B 04 C0 [E8] ? ? ? ? 45 33 C0",
            });

        return { rbx_call, r8_call };
    }

    auto scan_phase2_targets(std::span<std::uint8_t> region)
        -> phase2_targets
    {
        auto result = phase2_targets {};

        try
        {
            auto const calls = scan_subscreen_call_targets(region);
            result.subscreen_call_rbx = calls[0];
            result.subscreen_call_r8 = calls[1];
            result.subscreen_supported = true;
        }
        catch (const std::exception& error)
        {
            avs2::log::misc(
                "80th folder: ssm_80th disabled on this build: {}",
                error.what());
        }

        try
        {
            result.title_color_calls = scan_title_color_targets(
                region, result.title_layout3_entry_offset);
            static constexpr std::array route_names {
                "Music Select layout RBX",
                "Music Select layout RSI",
                "Music Select layout R14+28",
                "Music Select layout R14 dynamic",
                "Music Select layout R14+10"
            };
            for (auto layout = std::size_t {}; layout < result.title_color_calls.size(); ++layout)
            {
                auto const [merge, offset] = find_title_route_merge(
                    result.title_color_calls[layout], route_names[layout]);
                result.title_route_merge_points[layout] = merge;
                result.title_route_stack_offsets[layout] = offset;
            }
            result.title_color_supported = true;
        }
        catch (const std::exception& error)
        {
            avs2::log::misc(
                "80th folder: Music Select purple title disabled on this build: {}",
                error.what());
        }

        try
        {
            result.extended_title_color_calls =
                scan_extended_title_color_targets(region,
                    result.extended_title_route_merge_points,
                    result.extended_title_route_stack_offsets,
                    result.extended_title_route_registers,
                    result.extended_title_entry_registers);
            result.extended_title_color_supported = true;
        }
        catch (const std::exception& error)
        {
            avs2::log::misc(
                "80th folder: play/result/sub-screen purple title disabled on this build: {}",
                error.what());
        }

        return result;
    }

    auto is_readable(std::uintptr_t address, std::size_t size) -> bool
    {
        if (!address || !size ||
            address > std::numeric_limits<std::uintptr_t>::max() - size)
            return false;

        auto info = MEMORY_BASIC_INFORMATION {};

        if (!VirtualQuery(reinterpret_cast<const void*>(address),
            &info, sizeof(info)))
            return false;

        auto const region_begin =
            reinterpret_cast<std::uintptr_t>(info.BaseAddress);
        auto const region_end = region_begin + info.RegionSize;
        auto const request_end = address + size;

        return info.State == MEM_COMMIT &&
            !(info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
            request_end <= region_end;
    }

    auto read_pointer(std::uintptr_t address) -> std::uintptr_t
    {
        if (!is_readable(address, sizeof(std::uintptr_t)))
            return 0;

        return *reinterpret_cast<const std::uintptr_t*>(address);
    }

    auto entry_is_infinitas_target(std::uintptr_t entry) -> bool
    {
        if (!g_runtime || !entry)
            return false;

        // IIDX34 live entries carry their MID at +0x67C. Prefer that direct
        // source so title-colour / subscreen hooks do not depend on the timing
        // of entry_to_mid map refresh.
        auto constexpr mid_offset = std::uintptr_t { 0x67C };
        if (is_readable(entry + mid_offset, sizeof(std::int32_t)))
        {
            auto const mid = *reinterpret_cast<const std::int32_t*>(
                entry + mid_offset);
            if (g_runtime->infinitas_whitelist.contains(mid))
                return true;
        }

        // Compatibility fallback for older builds.
        auto const it = g_runtime->entry_to_mid.find(entry);
        return it != g_runtime->entry_to_mid.end() &&
            g_runtime->infinitas_whitelist.contains(it->second);
    }

    auto mid_for_entry(std::uintptr_t entry) -> std::int32_t
    {
        if (!g_runtime || !entry)
            return -1;

        auto constexpr mid_offset = std::uintptr_t { 0x67C };
        if (is_readable(entry + mid_offset, sizeof(std::int32_t)))
        {
            auto const mid = *reinterpret_cast<const std::int32_t*>(
                entry + mid_offset);
            if (mid >= 0)
                return mid;
        }

        auto const it = g_runtime->entry_to_mid.find(entry);
        return it == g_runtime->entry_to_mid.end() ? -1 : it->second;
    }

    auto read_stack_alpha(const SafetyHookContext& ctx) -> float
    {
        auto const address = static_cast<std::uintptr_t>(ctx.rsp) + 0x20;
        if (!is_readable(address, sizeof(float)))
            return -1.0f;
        return *reinterpret_cast<const float*>(address);
    }

    auto apply_title_color(SafetyHookContext& ctx, std::uintptr_t entry,
        std::size_t layout, std::size_t route,
        bool preserve_native_priority = false) -> void
    {
        if (!g_runtime || !g_runtime->phase2 ||
            !entry_is_infinitas_target(entry))
            return;

        // Native special-colour states (notably LEGGENDARIA) have higher
        // priority than the INFINITAS purple override.  Several Music Select
        // layouts expose that state in a live register; if it is set, leave
        // the game's RGB untouched.
        if (preserve_native_priority)
            return;

        auto const mid = mid_for_entry(entry);
        auto const original_r = ctx.xmm1.f32[0];
        auto const original_g = ctx.xmm2.f32[0];
        auto const original_b = ctx.xmm3.f32[0];
        auto const alpha = read_stack_alpha(ctx);

        // Log a few hits from every native route. This identifies whether the
        // visible mixed colour is produced by another route/layer or by the
        // title texture/material after this constructor call.
        auto& count = g_runtime->phase2->title_route_log_count[layout][route];
        if (count < 8)
        {
            ++count;
            static constexpr std::array route_names {
                "preview", "version", "default"
            };
            OMNIFIX_FOLDER_TRACE(
                "80th folder: title diagnostic: MID {} layout {} route {} original rgba=({:.4f},{:.4f},{:.4f},{:.4f}) -> purple ({:.4f},{:.4f},{:.4f}) hit {}/8",
                mid, layout, route_names[route],
                original_r, original_g, original_b, alpha,
                g_runtime->settings.title_color_red,
                g_runtime->settings.title_color_green,
                g_runtime->settings.title_color_blue,
                count);
        }

        // Preserve native alpha so fades/selection animations remain intact.
        ctx.xmm1.f32[0] = g_runtime->settings.title_color_red;
        ctx.xmm2.f32[0] = g_runtime->settings.title_color_green;
        ctx.xmm3.f32[0] = g_runtime->settings.title_color_blue;

        if (!g_runtime->phase2->title_color_logged[layout])
        {
            g_runtime->phase2->title_color_logged[layout] = true;
            OMNIFIX_FOLDER_TRACE(
                "80th folder: title layout {} applied RGB #{:02X}{:02X}{:02X}",
                layout,
                static_cast<unsigned>(g_runtime->settings.title_color_red * 255.0f + 0.5f),
                static_cast<unsigned>(g_runtime->settings.title_color_green * 255.0f + 0.5f),
                static_cast<unsigned>(g_runtime->settings.title_color_blue * 255.0f + 0.5f));
        }
    }

    template<std::size_t Route>
    auto hook_title_color_rbx(SafetyHookContext& ctx) -> void
    {
        apply_title_color(ctx, ctx.rbx, 0, Route);
    }

    template<std::size_t Route>
    auto hook_title_color_rsi(SafetyHookContext& ctx) -> void
    {
        apply_title_color(ctx, read_pointer(ctx.rsi), 1, Route,
            (ctx.r14 & 0xFF) != 0);
    }

    template<std::size_t Route>
    auto hook_title_color_r14_28(SafetyHookContext& ctx) -> void
    {
        apply_title_color(ctx, read_pointer(ctx.r14 + 0x28), 2, Route,
            (ctx.rdi & 0xFF) != 0);
    }

    template<std::size_t Route>
    auto hook_title_color_r14_38(SafetyHookContext& ctx) -> void
    {
        if (!g_runtime || !g_runtime->phase2)
            return;
        // The matched renderer determines the pointer offset for IIDX33/34.
        apply_title_color(ctx, read_pointer(ctx.r14 +
            g_runtime->phase2->target.title_layout3_entry_offset), 3, Route,
            (ctx.rbx & 0xFF) != 0);
    }

    template<std::size_t Route>
    auto hook_title_color_r14_10(SafetyHookContext& ctx) -> void
    {
        apply_title_color(ctx, read_pointer(ctx.r14 + 0x10), 4, Route,
            (ctx.r13 & 0xFF) != 0);
    }

    auto shared_title_entry(const SafetyHookContext& ctx, std::size_t layout)
        -> std::uintptr_t
    {
        if (!g_runtime || !g_runtime->phase2 || layout >= 2)
            return 0;
        return g_runtime->phase2->target.extended_title_entry_registers[layout] == 1 ?
            ctx.rdi : ctx.rbx;
    }

    template<std::size_t Route>
    auto hook_extended_title_color_shared_0(SafetyHookContext& ctx) -> void
    {
        auto const preserve = !is_readable(ctx.rsp + 0x88, 1) ||
            *reinterpret_cast<const std::uint8_t*>(ctx.rsp + 0x88) != 0;
        apply_title_color(ctx, shared_title_entry(ctx, 0), 5, Route, preserve);
    }

    template<std::size_t Route>
    auto hook_extended_title_color_shared_1(SafetyHookContext& ctx) -> void
    {
        auto const preserve = !is_readable(ctx.rsp + 0x88, 1) ||
            *reinterpret_cast<const std::uint8_t*>(ctx.rsp + 0x88) != 0;
        apply_title_color(ctx, shared_title_entry(ctx, 1), 6, Route, preserve);
    }

    template<std::size_t Route>
    auto hook_extended_title_color_text_mu(SafetyHookContext& ctx) -> void
    {
        // The text_mu renderer receives a pointer-to-entry in RDI. Both
        // IIDX33 and IIDX34 dereference it before reading the entry's version
        // field, so pass the actual live entry to the INFINITAS MID check.
        apply_title_color(ctx, read_pointer(ctx.rdi), 7, Route,
            (ctx.r15 & 0xFF) != 0);
    }

    auto apply_extended_title_route(SafetyHookContext& ctx,
        std::uintptr_t entry, std::size_t layout, bool preserve_native_priority)
        -> void
    {
        if (!g_runtime || !g_runtime->phase2 || preserve_native_priority ||
            layout >= g_runtime->phase2->target.extended_title_route_registers.size() ||
            !entry_is_infinitas_target(entry))
            return;

        auto const& target = g_runtime->phase2->target;
        auto style_base = std::uintptr_t {};
        auto address = static_cast<std::uintptr_t>(ctx.rsp) +
            target.extended_title_route_stack_offsets[layout];
        if (target.extended_title_route_registers[layout] == 1)
        {
            style_base = static_cast<std::uintptr_t>(ctx.rbp);
            address = static_cast<std::uintptr_t>(ctx.rbp) + 4;
        }
        else if (target.extended_title_route_registers[layout] == 2)
        {
            style_base = static_cast<std::uintptr_t>(ctx.rsi);
            address = static_cast<std::uintptr_t>(ctx.rsi) + 4;
        }
        else
        {
            auto const stack_offset =
                target.extended_title_route_stack_offsets[layout];
            if (stack_offset < 4)
                return;
            // The route discriminator is the second dword in this temporary
            // style record. Its RGB floats are at record offset +0x18.
            style_base = static_cast<std::uintptr_t>(ctx.rsp) + stack_offset - 4;
        }

        if (!is_readable(address, sizeof(std::int32_t)))
            return;

        auto& route = *reinterpret_cast<std::int32_t*>(address);
        auto const original_route = route;
        if (route == 2)
        {
            route = 1;
            OMNIFIX_FOLDER_TRACE(
                "80th folder: MID {} extended title layout {} material route 2 -> default 1",
                mid_for_entry(entry), layout);
        }

        // The constructor hook above changes the arguments used to build this
        // renderer's RGBA, but the renderer subsequently carries its own style
        // record. Force the final record used by the draw path as well, while
        // retaining the game's alpha/fade value at +0x0C.
        auto const color = style_base + 0x18;
        if (!style_base || !is_readable(color, 3 * sizeof(float)))
            return;

        auto* rgb = reinterpret_cast<float*>(color);
        rgb[0] = g_runtime->settings.title_color_red;
        rgb[1] = g_runtime->settings.title_color_green;
        rgb[2] = g_runtime->settings.title_color_blue;

        if (!g_runtime->phase2->extended_style_logged[layout])
        {
            g_runtime->phase2->extended_style_logged[layout] = true;
            OMNIFIX_FOLDER_TRACE(
                "80th folder: MID {} extended title layout {} forced final RGB #{:02X}{:02X}{:02X} (route {})",
                mid_for_entry(entry), layout,
                static_cast<unsigned>(g_runtime->settings.title_color_red * 255.0f + 0.5f),
                static_cast<unsigned>(g_runtime->settings.title_color_green * 255.0f + 0.5f),
                static_cast<unsigned>(g_runtime->settings.title_color_blue * 255.0f + 0.5f),
                original_route);
        }
    }

    auto hook_extended_title_route_shared_0(SafetyHookContext& ctx) -> void
    {
        auto const preserve = !is_readable(ctx.rsp + 0x88, 1) ||
            *reinterpret_cast<const std::uint8_t*>(ctx.rsp + 0x88) != 0;
        apply_extended_title_route(ctx, shared_title_entry(ctx, 0), 0, preserve);
    }

    auto hook_extended_title_route_shared_1(SafetyHookContext& ctx) -> void
    {
        auto const preserve = !is_readable(ctx.rsp + 0x88, 1) ||
            *reinterpret_cast<const std::uint8_t*>(ctx.rsp + 0x88) != 0;
        apply_extended_title_route(ctx, shared_title_entry(ctx, 1), 1, preserve);
    }

    auto hook_extended_title_route_text_mu(SafetyHookContext& ctx) -> void
    {
        apply_extended_title_route(ctx, read_pointer(ctx.rdi), 2,
            (ctx.r15 & 0xFF) != 0);
    }

    auto apply_subscreen_texture(SafetyHookContext& ctx,
        std::uintptr_t entry, std::size_t path) -> void
    {
        if (!g_runtime || !g_runtime->phase2 ||
            !entry_is_infinitas_target(entry))
            return;

        // Windows x64 third argument: R8. The game has already completed its
        // normal table lookup at this point; substitute only the final texture
        // name passed to the setter for a verified INFINITAS music entry.
        ctx.r8 = std::bit_cast<std::uintptr_t>(
            g_runtime->subscreen_texture_ptr);

        if (!g_runtime->phase2->subscreen_logged[path])
        {
            g_runtime->phase2->subscreen_logged[path] = true;
            OMNIFIX_FOLDER_TRACE(
                "80th folder: Lightning path {} applied texture '{}'",
                path, g_runtime->settings.subscreen_texture);
        }
    }

    auto hook_phase2_subscreen_call_rbx(SafetyHookContext& ctx) -> void
    {
        // This layout keeps the current music entry at [RDI+0x10].
        apply_subscreen_texture(ctx, read_pointer(ctx.rdi + 0x10), 0);
    }

    auto hook_phase2_subscreen_call_r8(SafetyHookContext& ctx) -> void
    {
        // This layout keeps the current music entry at [R14+0x10].
        apply_subscreen_texture(ctx, read_pointer(ctx.r14 + 0x10), 1);
    }

    auto apply_default_title_route(SafetyHookContext& ctx,
        std::uintptr_t entry, std::size_t layout) -> void
    {
        if (!g_runtime || !g_runtime->phase2 ||
            !entry_is_infinitas_target(entry))
            return;

        auto const offset =
            g_runtime->phase2->target.title_route_stack_offsets[layout];
        auto const address = static_cast<std::uintptr_t>(ctx.rsp) + offset;
        if (!is_readable(address, sizeof(std::int32_t)))
            return;

        auto& route = *reinterpret_cast<std::int32_t*>(address);
        auto const original = route;

        // Phase 2.9.7 was designed specifically to bypass the native
        // current-Version material route (discriminator 2).  Do not coerce
        // route 0: on newer builds it is also used by higher-priority native
        // colour states such as LEGGENDARIA.  Route 1 is already default.
        if (original != 2)
            return;

        route = 1;
        OMNIFIX_FOLDER_TRACE(
            "80th folder: MID {} layout {} title material route 2 -> default 1",
            mid_for_entry(entry), layout);
    }

    auto hook_title_route_rbx(SafetyHookContext& ctx) -> void
    { apply_default_title_route(ctx, ctx.rbx, 0); }
    auto hook_title_route_rsi(SafetyHookContext& ctx) -> void
    { apply_default_title_route(ctx, read_pointer(ctx.rsi), 1); }
    auto hook_title_route_r14_28(SafetyHookContext& ctx) -> void
    { apply_default_title_route(ctx, read_pointer(ctx.r14 + 0x28), 2); }
    auto hook_title_route_r14_38(SafetyHookContext& ctx) -> void
    {
        if (!g_runtime || !g_runtime->phase2)
            return;
        apply_default_title_route(ctx, read_pointer(ctx.r14 +
            g_runtime->phase2->target.title_layout3_entry_offset), 3);
    }
    auto hook_title_route_r14_10(SafetyHookContext& ctx) -> void
    { apply_default_title_route(ctx, read_pointer(ctx.r14 + 0x10), 4); }

    auto setup_phase2(std::span<std::uint8_t> region) -> bool
    {
        if (!g_runtime || g_runtime->phase2)
            return g_runtime && static_cast<bool>(g_runtime->phase2);

        try
        {
            OMNIFIX_FOLDER_TRACE(
                "enabling 80th folder LEGGENDARIA-priority Version-route bypass (all native colour paths; all-screen title RGB #{:02X}{:02X}{:02X}; direct Lightning texture argument '{}')",
                static_cast<unsigned>(g_runtime->settings.title_color_red * 255.0f + 0.5f),
                static_cast<unsigned>(g_runtime->settings.title_color_green * 255.0f + 0.5f),
                static_cast<unsigned>(g_runtime->settings.title_color_blue * 255.0f + 0.5f),
                g_runtime->settings.subscreen_texture);

            auto next = std::make_unique<phase2_runtime>();
            next->target = scan_phase2_targets(region);

            if (g_runtime->settings.enable_subscreen_texture &&
                next->target.subscreen_supported)
            {
                next->hook_subscreen_call_rbx = safetyhook::create_mid(
                    next->target.subscreen_call_rbx,
                    &hook_phase2_subscreen_call_rbx);
                next->hook_subscreen_call_r8 = safetyhook::create_mid(
                    next->target.subscreen_call_r8,
                    &hook_phase2_subscreen_call_r8);
            }

            if (next->target.title_color_supported)
            {
                using callback = void (*)(SafetyHookContext&);
                constexpr std::array<std::array<callback, 3>, 5> callbacks {{
                    {{ &hook_title_color_rbx<0>, &hook_title_color_rbx<1>, &hook_title_color_rbx<2> }},
                    {{ &hook_title_color_rsi<0>, &hook_title_color_rsi<1>, &hook_title_color_rsi<2> }},
                    {{ &hook_title_color_r14_28<0>, &hook_title_color_r14_28<1>, &hook_title_color_r14_28<2> }},
                    {{ &hook_title_color_r14_38<0>, &hook_title_color_r14_38<1>, &hook_title_color_r14_38<2> }},
                    {{ &hook_title_color_r14_10<0>, &hook_title_color_r14_10<1>, &hook_title_color_r14_10<2> }},
                }};

                for (auto layout = std::size_t {}; layout < callbacks.size(); ++layout)
                {
                    for (auto route = std::size_t {}; route < 3; ++route)
                    {
                        auto const target =
                            next->target.title_color_calls[layout][route];
                        if (!target)
                            continue;

                        next->hook_title_color[layout][route] =
                            safetyhook::create_mid(
                                target,
                                callbacks[layout][route]);
                    }
                }
            }

            if (next->target.title_color_supported)
            {
                using callback = void (*)(SafetyHookContext&);
                constexpr std::array<callback, 5> callbacks {
                    &hook_title_route_rbx,
                    &hook_title_route_rsi,
                    &hook_title_route_r14_28,
                    &hook_title_route_r14_38,
                    &hook_title_route_r14_10,
                };

                for (auto layout = std::size_t {}; layout < callbacks.size(); ++layout)
                {
                    next->hook_title_route_merge[layout] = safetyhook::create_mid(
                        next->target.title_route_merge_points[layout],
                        callbacks[layout]);
                }
            }

            if (g_runtime->settings.enable_extended_title_color &&
                next->target.extended_title_color_supported)
            {
                using callback = void (*)(SafetyHookContext&);
                constexpr std::array<std::array<callback, 3>, 3> callbacks {{
                    {{ &hook_extended_title_color_shared_0<0>, &hook_extended_title_color_shared_0<1>, &hook_extended_title_color_shared_0<2> }},
                    {{ &hook_extended_title_color_shared_1<0>, &hook_extended_title_color_shared_1<1>, &hook_extended_title_color_shared_1<2> }},
                    {{ &hook_extended_title_color_text_mu<0>, &hook_extended_title_color_text_mu<1>, &hook_extended_title_color_text_mu<2> }},
                }};

                for (auto layout = std::size_t {}; layout < callbacks.size(); ++layout)
                {
                    for (auto route = std::size_t {}; route < 3; ++route)
                    {
                        auto const target =
                            next->target.extended_title_color_calls[layout][route];
                        if (!target)
                            continue;

                        next->hook_extended_title_color[layout][route] =
                            safetyhook::create_mid(target, callbacks[layout][route]);
                    }
                }

                constexpr std::array<callback, 3> route_callbacks {
                    &hook_extended_title_route_shared_0,
                    &hook_extended_title_route_shared_1,
                    &hook_extended_title_route_text_mu,
                };
                for (auto layout = std::size_t {}; layout < route_callbacks.size(); ++layout)
                {
                    auto const target =
                        next->target.extended_title_route_merge_points[layout];
                    if (!target)
                        continue;

                    next->hook_extended_title_route_merge[layout] =
                        safetyhook::create_mid(target, route_callbacks[layout]);
                }
            }

            g_runtime->phase2 = std::move(next);

            OMNIFIX_FOLDER_TRACE(
                "80th folder initialized: ssm_80th {}; Music Select purple title {}; play/result/sub-screen purple title {}",
                g_runtime->settings.enable_subscreen_texture &&
                    g_runtime->phase2->target.subscreen_supported ?
                    "enabled by direct call-argument hook": "disabled",
                g_runtime->phase2->target.title_color_supported ?
                    "enabled": "not available on this build",
                g_runtime->settings.enable_extended_title_color &&
                    g_runtime->phase2->target.extended_title_color_supported ?
                    "enabled": g_runtime->settings.enable_extended_title_color ?
                    "not available on this build": "disabled by option");
            return true;
        }
        catch (const std::exception& error)
        {
            // Phase 1 remains active. Any partially created Phase 2 hooks are
            // owned by the local state above and are removed during unwinding.
            avs2::log::misc(
                "80th folder disabled safely: {}", error.what());
            return false;
        }
    }

    auto setup(std::span<std::uint8_t> game_region,
        std::string_view modified_mdb_path, config settings) -> bool
    {
        if (g_runtime)
            return true;

        try
        {
            OMNIFIX_FOLDER_TRACE(
                "80th folder: enabling INFINITAS MID whitelist: MID > 20000, suffix 200..499, excluding MID 33333; plus MID 32761");

            auto songs = load_folder_songs(
                modified_mdb_path, settings.music_version);

            if (settings.disable_old_charts)
            {
                for (auto& style_songs: songs)
                    std::erase_if(style_songs, [](auto const& song) {
                        return old_charts::matches(song.music_id);
                    });
            }

            if (songs[game::style_sp].empty() &&
                songs[game::style_dp].empty())
            {
                avs2::log::misc(
                    "80th folder disabled: no songs matched MID > 20000 with suffix 200..499 (excluding MID 33333), or explicit MID 32761, in '{}'",
                    modified_mdb_path);
                return false;
            }

            auto next = std::make_unique<runtime>();
            next->settings = settings;
            next->subscreen_texture_ptr =
                next->settings.subscreen_texture.data();
            next->target = scan_targets(game_region);

            // Build one stable MID whitelist from the songs selected by the
            // shared INFINITAS rule. SP and DP may contain the same MID, so the
            // set naturally deduplicates them.
            for (auto const& style_songs: songs)
            {
                for (auto const& song: style_songs)
                    next->infinitas_whitelist.insert(song.music_id);
            }

            auto const setup_folder = [&] (folder_variant& folder,
                int id, int style, std::vector<song_reference> entries)
            {
                folder.id = id;
                folder.play_style = style;
                folder.songs = std::move(entries);
                // Keep the existing group metadata; expose the previously
                // unnamed +0x04 field so native DEFAULT resolves to LEVEL.
                folder.definition.default_sort = 2;
                folder.definition.difficulty_switchable = true;
                folder.definition.difficulty_locked = false;
                folder.definition.texture_hover_header =
                    next->settings.texture_header.data();
                folder.definition.texture_hover_subheader =
                    next->settings.texture_subheader.data();
                folder.definition.texture_bar_text =
                    next->settings.texture_bar_text.data();
                folder.definition.texture_bar_bg =
                    next->settings.texture_bar_background.data();
                folder.definition.ticker_text =
                    next->settings.ticker_text.data();
                folder.definition.capacity = std::max<std::int32_t>(1,
                    static_cast<std::int32_t>(folder.songs.size()));
            };

            setup_folder(next->folders[game::style_sp],
                settings.category_id, game::style_sp,
                std::move(songs[game::style_sp]));
            setup_folder(next->folders[game::style_dp],
                settings.category_id, game::style_dp,
                std::move(songs[game::style_dp]));

            OMNIFIX_FOLDER_TRACE(
                "80th folder: loaded {} SP and {} DP songs into shared category {} (one bar per song)",
                next->folders[game::style_sp].songs.size(),
                next->folders[game::style_dp].songs.size(),
                settings.category_id);

            OMNIFIX_FOLDER_TRACE(
                "80th folder: legacy version-to-category redirect disabled; native category IDs preserved");
            if (settings.enable_search_version_redirect)
            {
                OMNIFIX_FOLDER_TRACE(
                    "80th folder: ignoring retired enable_search_version_redirect option");
            }

            // Publish state before hook creation; on failure the hook objects are
            // destroyed and automatically removed when the state is reset.
            g_runtime = std::move(next);

            // Build the initial live-entry-to-MID map. It is refreshed again
            // after Music Select initialization, when the game database is ready.
            refresh_entry_mid_map("initial setup");

            OMNIFIX_FOLDER_TRACE(
                "80th folder: built INFINITAS whitelist with {} MIDs",
                g_runtime->infinitas_whitelist.size());

            g_runtime->hook_bar_populate = safetyhook::create_inline(
                g_runtime->target.bar_populate, &hook_bar_populate);
            g_runtime->hook_get_definition = safetyhook::create_inline(
                g_runtime->target.get_definition, &hook_get_definition);
            g_runtime->hook_category_lookup = safetyhook::create_inline(
                g_runtime->target.category_lookup, &hook_category_lookup);
            g_runtime->hook_active_category = safetyhook::create_inline(
                g_runtime->target.active_category, &hook_active_category);
            g_runtime->hook_folder_voice = safetyhook::create_inline(
                g_runtime->target.folder_voice, &hook_folder_voice);
            g_runtime->hook_close_categories = safetyhook::create_inline(
                g_runtime->target.close_categories, &hook_close_categories);
            g_runtime->hook_music_select_init = safetyhook::create_inline(
                g_runtime->target.music_select_init, &hook_music_select_init);
            g_runtime->hook_clear_lamp = safetyhook::create_inline(
                g_runtime->target.clear_lamp, &hook_clear_lamp);
            try
            {
                setup_search_category_hooks(game_region);
            }
            catch (const std::exception& e)
            {
                avs2::log::misc("80th folder: search category fix disabled: {}", e.what());
            }

            if (g_runtime->target.version_category_insert)
            {
                g_runtime->hook_version_category_insert = safetyhook::create_inline(
                    g_runtime->target.version_category_insert,
                    &hook_version_category_insert);
                OMNIFIX_FOLDER_TRACE(
                    "80th folder: IIDX34 native Version-80 duplicate exclusion enabled");
            }
            g_runtime->hook_badge_weekly = safetyhook::create_inline(
                g_runtime->target.badge_weekly, &hook_badge_weekly);
            g_runtime->hook_badge_featured = safetyhook::create_inline(
                g_runtime->target.badge_featured, &hook_badge_featured);
            g_runtime->hook_badge_tournament = safetyhook::create_inline(
                g_runtime->target.badge_tournament, &hook_badge_tournament);
            g_runtime->hook_badge_kac = safetyhook::create_inline(
                g_runtime->target.badge_kac, &hook_badge_kac);

            if (!g_runtime->target.system_sounds.contains(
                std::string { settings.folder_voice }))
            {
                avs2::log::misc(
                    "80th folder: voice '{}' not found; using stock voice",
                    settings.folder_voice);
            }

            if (settings.enable_phase2)
                setup_phase2(game_region);
            else
                avs2::log::misc(
                    "80th folder disabled by configuration");

            avs2::log::misc(
                "80th folder initialized with native Version texture slots: header=ss_version, subheader=ss_80th, bar_text=so_80th");
            return true;
        }
        catch (const std::exception& error)
        {
            g_runtime.reset();
            avs2::log::misc(
                "80th folder disabled safely: {}", error.what());
            return false;
        }
    }
}
