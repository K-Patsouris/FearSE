#pragma once
#include "DataDefs/PlayerRules.h"

namespace Data::PlayerRules {

	void register_pr_menu() noexcept;

	// Will block until the menu is closed by the user, so call on a thread that can wait blocked whil UI is doing stuff.
	// Takes a rules_info, and returns it modified according to user choices on the menu.
	::Data::rules_info show_menu(::Data::rules_info prules) noexcept;

	void menu_button_chosen(const std::uint32_t btn_id) noexcept;

}
