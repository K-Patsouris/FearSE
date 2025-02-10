#pragma once
#include "Common.h"
#include "Types/SyncTypes.h"

namespace CustomMenu {

	class MenuManager {
	public:
		static constexpr string_view MenuName{ "ODCustomMenu"sv };

		[[nodiscard]] static MenuManager& GetSingleton();
		static void RegisterMenu() noexcept;
		static RE::IMenu* Creator();

		[[nodiscard]] std::optional<string> GetMenuPath() const noexcept;
		bool SetMenuPath(const string& new_path) noexcept;
		void SetMenuPath(string&& new_path) noexcept;

	private:
		using R_locker = SyncTypes::noexlock_guard<SyncTypes::shared_spinlock, SyncTypes::LockingMode::Shared>;
		using RW_locker = SyncTypes::noexlock_guard<SyncTypes::shared_spinlock, SyncTypes::LockingMode::Exclusive>;
		mutable SyncTypes::shared_spinlock lock{};
		string MenuPath{ "Placeholder_call_SetMenuPath" };

		MenuManager() = default;
		MenuManager(const MenuManager&) = delete;
		MenuManager(MenuManager&&) = delete;
	};

	inline MenuManager& MenuManagerSingleton{ MenuManager::GetSingleton() };


	class Menu : public RE::IMenu {
	public:
		using super = RE::IMenu;
		using GRefCountBaseStatImpl::operator new;
		using GRefCountBaseStatImpl::operator delete;

		Menu();

		RE::UI_MESSAGE_RESULTS ProcessMessage(RE::UIMessage& msg) override;
	};

}
