#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace omnifix::omni_folder
{
    struct config
    {
        std::uint16_t music_version = 80;
        // SP and DP are views of one version folder in the native game.
        // Keeping one ID lets Music Select retain the folder on style changes.
        std::int32_t category_id = 980;

        std::string_view texture_header = "ss_version";
        std::string_view texture_subheader = "ss_80th";
        std::string_view texture_bar_text = "so_80th";
        std::string_view texture_bar_background = "so_b_version";
        std::string_view subscreen_texture = "ssm_80th";
        std::string_view ticker_text = "SELECT FROM 80TH CATEGORY";
        std::string_view folder_voice = "syssd_category_voice_infinitas";

        // Phase 2.6: corrected native Version-folder texture slots, explicit
        // badge suppression, per-difficulty folder clear lamps, safe Lightning
        // call-argument texture replacement, and Version-80 title colour across
        // Music Select plus the shared play/result/sub-screen renderers.
        bool disable_old_charts = false;
        bool enable_phase2 = true;
        bool enable_subscreen_texture = true;
        bool enable_extended_title_color = true;

        // Retired: retained only for source compatibility and ignored.
        // Global version-to-category aliases collide with native EVENT IDs.
        bool enable_search_version_redirect = false;

        // Sampled from the lower filled area of the supplied INFINITAS title
        // image. Alpha remains controlled by the game at runtime.
        float title_color_red = 150.0f / 255.0f;
        float title_color_green = 117.0f / 255.0f;
        float title_color_blue = 252.0f / 255.0f;
    };

    /**
     * Create a version-based music-select folder using runtime pattern scans.
     *
     * This function is deliberately fail-safe: a missing pattern or unsupported
     * MDB layout disables only the folder module and does not abort Omnifix.
     */
    auto setup(std::span<std::uint8_t> game_region,
        std::string_view modified_mdb_path, config settings = {}) -> bool;
}
