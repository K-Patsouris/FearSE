#include "MenuUtils.h"
#include "Logger.h"
#include "PrimitiveUtils.h"
#include "StringUtils.h"
#include "CustomMenu.h"

#include <shared_mutex>


namespace MenuUtils {
	using PrimitiveUtils::to_u32;
	using PrimitiveUtils::to_s32;
	using StringUtils::StrICmp;

	enum status_code {
		available,
		waiting_menu,
		waiting_modevent,
		event_received,

		invalid
	};
	enum menu_code {
		menu_closed,
		menu_opened,

		menu_invalid
	};
	enum modevent_code {
		evmmbv_frame,
		evmmbv_result,
		evmsm_frame,
		evmsm_result,

		evmmbbl_frame,
		evmmbbl_result,

		modevent_invalid
	};
	class EventManager {
	public:
		static EventManager& GetSingleton() {
			static EventManager singleton{};
			return singleton;
		}

		bool RegisterForMenuChange(const menu_code menucode) noexcept {
			if ((menucode == menu_invalid) or (menu_checker.load(std::memory_order_relaxed) != nullptr)) {
				return false;
			}
			switch (menucode) {
			case menu_closed: menu_checker.store(CheckMenuClosed, std::memory_order_relaxed); break;
			case menu_opened: menu_checker.store(CheckMenuOpened, std::memory_order_relaxed); break;
			default: __assume(0);
			}
			return true;
		}
		bool UnregisterForMenuChange() noexcept {
			bool success = false;
			lock.lock();
			if (statuscode != available) {
				Log::Error("MenuUtils::EventManager::UnregisterForMenuChange: resource unavailable! Unregister skipped to avoid endless waiting_menu"sv);
			}
			else {
				menu_checker.store(nullptr, std::memory_order_relaxed);
				success = true;
			}
			lock.unlock();
			return success;
		}
		void NotifyMenuChange(const char* name, const bool opened) noexcept {
			if (auto ptr = menu_checker.load(std::memory_order_relaxed); !ptr or !ptr(name, opened)) {
				return;
			}
			bool mustnotify = false;
			lock.lock();
			mustnotify = (statuscode == waiting_menu);
			statuscode = event_received;
			lock.unlock();
			if (mustnotify) {
				waiter.notify_all();
			}
		}
		bool WaitForRegisteredMenuChange(const std::chrono::milliseconds timeout, const bool unregister_after) {
			Uq_locker locker{ lock };
			if (statuscode == event_received) {
				Log::Warning("MenuUtils::EventManager::WaitForMenuChange: immediate success"sv);
				if (unregister_after) { menu_checker.store(nullptr, std::memory_order_relaxed); }
				return true;
			}
			if (statuscode != available) {
				Log::Error("MenuUtils::EventManager::WaitForMenuChange: not available"sv);
				return false;
			}
			statuscode = waiting_menu;
			waiter.wait_for(locker, timeout);
			const bool success = statuscode != waiting_menu;
			statuscode = available;
			if (unregister_after) { menu_checker.store(nullptr, std::memory_order_relaxed); }
			return success;
		}

		bool RegisterForModEvent(const modevent_code eventcode) noexcept {
			if ((eventcode == modevent_invalid) or (modevent_checker.load(std::memory_order_relaxed) != nullptr)) {
				return false;
			}
			switch (eventcode) {
			case evmmbv_frame: modevent_checker.store(CheckEVMMBVFrame, std::memory_order_relaxed); break;
			case evmmbv_result: modevent_checker.store(CheckEVMMBVClosed, std::memory_order_relaxed); break;
			case evmsm_frame: modevent_checker.store(CheckEVMSMFrame, std::memory_order_relaxed); break;
			case evmsm_result: modevent_checker.store(CheckEVMSMClosed, std::memory_order_relaxed); break;

			case evmmbbl_frame: modevent_checker.store(CheckEVMMBBLFrame, std::memory_order_relaxed); break;
			case evmmbbl_result: modevent_checker.store(CheckEVMMBBLClosed, std::memory_order_relaxed); break;
			default: __assume(0);
			}
			return true;
		}
		bool UnregisterForModEvent() noexcept {
			bool success = false;
			lock.lock();
			if (statuscode != available) {
				Log::Error("MenuUtils::EventManager::UnregisterForModEvent: resource unavailable! Unregister skipped to avoid endless waiting_menu"sv);
			}
			else {
				modevent_checker.store(nullptr, std::memory_order_relaxed);
				success = true;
			}
			lock.unlock();
			return success;
		}
		void NotifyModEvent(const SKSE::ModCallbackEvent* event) noexcept {
			if (auto ptr = modevent_checker.load(std::memory_order_relaxed); !ptr or !ptr(event->eventName.c_str())) {
				return;
			}
			bool mustnotify = false;
			lock.lock();
			mustnotify = (statuscode == waiting_modevent);
			modevent_copy = *event;
			statuscode = event_received;
			lock.unlock();
			if (mustnotify) {
				waiter.notify_all();
			}
		}
		std::optional<SKSE::ModCallbackEvent> WaitForRegisteredModEvent(const bool unregister_after) noexcept {
			Uq_locker locker{ lock };
			if (statuscode == event_received) {
				Log::Warning("MenuUtils::EventManager::WaitForRegisteredModEvent: immediate success"sv);
				if (unregister_after) { modevent_checker.store(nullptr, std::memory_order_relaxed); }
				return std::move(modevent_copy);
			}
			if (statuscode != available) {
				Log::Error("MenuUtils::EventManager::WaitModEvent: not available"sv);
				return std::nullopt;
			}
			statuscode = waiting_modevent;
			while (statuscode == waiting_modevent) { // Handle the conditions manually because I am too retarded to be trusted with the predicate
				waiter.wait(locker);
			}
			statuscode = available;
			if (unregister_after) { modevent_checker.store(nullptr, std::memory_order_relaxed); }
			return std::move(modevent_copy);
		}

	private:
		using Uq_locker = std::unique_lock<std::shared_mutex>;
		std::shared_mutex lock{};
		std::condition_variable_any waiter{};
		SKSE::ModCallbackEvent modevent_copy{};
		status_code statuscode{ available };
		std::atomic<bool(*)(const char*, const bool)> menu_checker{};
		std::atomic<bool(*)(const char*)> modevent_checker{};

		static bool CheckMenuClosed(const char* name, const bool opened) { return !opened and StrICmp(name, CustomMenu::MenuManager::MenuName.data()); }
		static bool CheckMenuOpened(const char* name, const bool opened) { return opened and StrICmp(name, CustomMenu::MenuManager::MenuName.data()); }

		static bool CheckEVMMBVFrame(const char* name) { return StrICmp(name, "EVM_MBVerticalFrame"); }
		static bool CheckEVMMBVClosed(const char* name) { return StrICmp(name, "EVM_MBVerticalClosed"); }
		static bool CheckEVMSMFrame(const char* name) { return StrICmp(name, "EVM_SMSenderFrame"); }
		static bool CheckEVMSMClosed(const char* name) { return StrICmp(name, "EVM_SMSenderClosed"); }

		static bool CheckEVMMBBLFrame(const char* name) { return StrICmp(name, "EVM_MBButtonlessFrame"); }
		static bool CheckEVMMBBLClosed(const char* name) { return StrICmp(name, "EVM_MBButtonlessClosed"); }

		EventManager() {
			menu_checker.store(nullptr, std::memory_order_relaxed);
			modevent_checker.store(nullptr, std::memory_order_relaxed);
		}
		EventManager(const EventManager&) = delete;
		EventManager(EventManager&&) = delete;

	};
	EventManager& Manager{ EventManager::GetSingleton() };

	void NotifyMenuChange(const char* name, const bool opened) noexcept { Manager.NotifyMenuChange(name, opened); }
	void NotifyModEvent(const SKSE::ModCallbackEvent* event) noexcept { Manager.NotifyModEvent(event); }

	// Returns true if: the menu is open and expected_open_status==true, or the menu is closed and expected_open_status==false. Returns false otherwise.
	bool CheckMenuStatus(const bool expected_open_status) noexcept {
		const auto tasker = SKSE::GetTaskInterface();
		const auto ui = RE::UI::GetSingleton();
		if (!tasker or !ui) {
			return false;
		}
		std::atomic<int> status{};
		status.store(-1, std::memory_order_release);
		tasker->AddUITask([&] {
			const int result = ui->IsMenuOpen(CustomMenu::MenuManager::MenuName); // closed:0, open:1
			status.store(result, std::memory_order_release);
			status.notify_all();
		});
		status.wait(-1, std::memory_order_acquire);
		return status.load(std::memory_order_acquire) == static_cast<int>(expected_open_status);
	}
	// Tries to show the menu if open==true, or hide the menu if open==false. Returns true for success.
	bool TryOpenCloseMenu(const bool open) noexcept {
		const auto tasker = SKSE::GetTaskInterface();
		const auto uiMQ = RE::UIMessageQueue::GetSingleton();
		const array<menu_code, 2> codes{ menu_closed, menu_opened };
		if (!tasker or !uiMQ or !Manager.RegisterForMenuChange(codes[open])) {
			return false;
		}

		tasker->AddUITask([&] {
			Log::Info(">> UI Thread adding message open=<{}>"sv, open);
			const array<RE::UI_MESSAGE_TYPE, 2> msgs{ RE::UI_MESSAGE_TYPE::kHide, RE::UI_MESSAGE_TYPE::kShow };
			uiMQ->AddMessage(CustomMenu::MenuManager::MenuName, msgs[open], nullptr);
		});

		return Manager.WaitForRegisteredMenuChange(500ms, true);
	}
	bool Show() noexcept {
		return !CheckMenuStatus(true) and TryOpenCloseMenu(true); // Must not already be open, and must open now.
	}
	bool Hide() noexcept {
		return CheckMenuStatus(false) or TryOpenCloseMenu(false); // Must either already be closed, or close now.
	}
	bool GetView(RE::GPtr<RE::GFxMovieView>& view) noexcept {
		const auto tasker = SKSE::GetTaskInterface();
		const auto ui = RE::UI::GetSingleton();
		if (!tasker or !ui) {
			return false;
		}
		
		int tries = 0;
		while (true) {
			std::atomic<bool> status{};
			status.store(false, std::memory_order_release);
			tasker->AddUITask([&] {
				view = ui->GetMovieView(CustomMenu::MenuManager::MenuName);
				status.store(true, std::memory_order_release);
				status.notify_all();
			});
			status.wait(false, std::memory_order_acquire);

			if ((++tries == 5) or (view.get() != nullptr)) { // Up to 5 tries, with 100ms between. Check like this to skip the last wait after timeout or success.
				break;
			}
			std::this_thread::sleep_for(100ms);
		}

		return view.get() != nullptr;
	}
	bool Invoke(const RE::GPtr<RE::GFxMovieView>& view, const string& method, const lazy_vector<RE::GFxValue>& args) noexcept {
		if (const auto tasker = SKSE::GetTaskInterface(); tasker) {
			std::atomic<int> status{};
			status.store(-1, std::memory_order_release);
			tasker->AddUITask([&] {
				Log::Info(">> UI Thread invoking <{}>"sv, method);
				const auto result = view->Invoke(method.c_str(), nullptr, args.data(), to_u32(args.size()));
				status.store(static_cast<int>(result), std::memory_order_release);
				status.notify_all();
			});
			status.wait(-1, std::memory_order_acquire);
			return status.load(std::memory_order_acquire) == static_cast<int>(true);
		}
		return false;
	}


	bool MakeMessageboxArgs(const string& msg, const lazy_vector<string>& btns, const alignment align, lazy_vector<RE::GFxValue>& out) noexcept {
		enum : u64 { ButtonsOffset = 9 };
		if (!out.resize(ButtonsOffset + btns.size())) {
			return false;
		}
		static const array<const char*, 3> aligns{ "0", "1", "2" };
		out[0].SetString(msg.c_str());		// Message
		out[1].SetString("22");				// FontSize
		out[2].SetString(aligns[align]);	// Align	0:left 1:center 2:right		Don't do to_string. Need living string when invoking.
		out[3].SetString("1");				// CancelIsFirstButton
		out[4].SetString("28");				// Enter
		out[5].SetString("1");				// Escape
		out[6].SetString("15");				// Tab
		out[7].SetString("54");				// Right Shift
		out[8].SetString("42");				// Left Shift
		for (u64 i = 0; i < btns.size(); ++i) {
			out[ButtonsOffset + i].SetString(btns[i].c_str());
		}
		return true;
	}
	i32 WaitEVMMBV(const string& msg, const lazy_vector<string>& btns, const alignment align) noexcept {
		if (btns.empty()) {
			return -1;
		}

		// Set menu name, register for frame, init menu, get movie view, and wait for frame (automatically unregistering after the wait).
		CustomMenu::MenuManagerSingleton.SetMenuPath("EVM_MBVertical");
		RE::GPtr<RE::GFxMovieView> view{};
		if (!Manager.RegisterForModEvent(evmmbv_frame) or !Show() or !GetView(view) or !Manager.WaitForRegisteredModEvent(true).has_value()) {
			return -1;
		}

		// Make args, register for result, and invoke args.
		lazy_vector<RE::GFxValue> args{};
		if (!MakeMessageboxArgs(msg, btns, align, args) or !Manager.RegisterForModEvent(evmmbv_result) or !Invoke(view, "_root.EVM_MBVerticalMC.InitMessageBox", args)) {
			return -1;
		}

		// Wait for result (automatically unregistering after the wait), hide, and return.
		const auto result = Manager.WaitForRegisteredModEvent(true);
		Hide();
		if (!result.has_value()) {
			return -1;
		}
		return static_cast<i32>(result->numArg);
	}

	i32 WaitEVMMBBL(const string& msg, const lazy_vector<string>& btns, const alignment align) noexcept {
		if (btns.empty()) {
			return -1;
		}

		// Set menu name, register for frame, init menu, get movie view, and wait for frame (automatically unregistering after the wait).
		CustomMenu::MenuManagerSingleton.SetMenuPath("EVM_MBButtonless");
		RE::GPtr<RE::GFxMovieView> view{};
		if (!Manager.RegisterForModEvent(evmmbbl_frame) or !Show() or !GetView(view) or !Manager.WaitForRegisteredModEvent(true).has_value()) {
			return -1;
		}

		// Make args, register for result, and invoke args.
		lazy_vector<RE::GFxValue> args{};
		if (!MakeMessageboxArgs(msg, btns, align, args) or !Manager.RegisterForModEvent(evmmbbl_result) or !Invoke(view, "_root.EVM_MBButtonlessMC.InitMessageBox", args)) {
			return -1;
		}

		// Wait for result (automatically unregistering after the wait), hide, and return.
		const auto result = Manager.WaitForRegisteredModEvent(true);
		Hide();
		if (!result.has_value()) {
			return -1;
		}
		return static_cast<i32>(result->numArg);
	}

	bool MakeSliderArgs(const array<string, 3>& tac, const lazy_vector<slider_params>& params, lazy_vector<RE::GFxValue>& out) {
		enum : u64 { SliderInfoOffset = 11 };
		if (!out.resize(SliderInfoOffset + params.size())) {
			return false;
		}
		out[0].SetString(tac[0]);			// Title
		out[1].SetString(tac[1]);			// Accept text
		out[2].SetString(tac[2]);			// Cancel text
		out[3].SetString("0");				// Gamepad
		out[4].SetString("28");				// Enter
		out[5].SetString("1");				// Escape
		out[6].SetString("15");				// Tab
		out[7].SetString("54");				// Right Shift
		out[8].SetString("42");				// Left Shift
		out[9].SetString("276");			// Gamepad A
		out[10].SetString("277");			// GamepadB
		for (u64 i = 0; i < params.size(); ++i) {
			out[SliderInfoOffset + i].SetString(params[i].c_str());
		}
		return true;
	}
	lazy_vector<string> WaitEVSM(const array<string, 3>& tac, const lazy_vector<slider_params>& params) noexcept {
		if (params.empty()) {
			return {};
		}

		// Set menu name, register for frame, init menu, get movie view, and wait for frame (automatically unregistering after the wait).
		CustomMenu::MenuManagerSingleton.SetMenuPath("EVM_SMSender");
		RE::GPtr<RE::GFxMovieView> view{};
		if (!Manager.RegisterForModEvent(evmsm_frame) or !Show() or !GetView(view) or !Manager.WaitForRegisteredModEvent(true).has_value()) {
			return {};
		}

		// Make args, register for result, and invoke args.
		lazy_vector<RE::GFxValue> args{};
		if (!MakeSliderArgs(tac, params, args) or !Manager.RegisterForModEvent(evmsm_result) or !Invoke(view, "_root.EVM_SMSenderMC.InitSliderMenu", args)) {
			return {};
		}

		// Wait for result (automatically unregistering after the wait), hide, and return.
		const auto result = Manager.WaitForRegisteredModEvent(true);
		Hide();
		if (!result.has_value() or (result->numArg == 0.0f)) { // Failure or menu declined
			return {};
		}
		return LazyVector::Split(result->strArg.c_str(), "||");
	}

	lazy_vector<u32> WaitEVSM_U32(const array<string, 3u>& tac, const lazy_vector<slider_params>& params) noexcept { return LazyVector::StrToU32(WaitEVSM(tac, params)); }
	lazy_vector<i32> WaitEVSM_S32(const array<string, 3u>& tac, const lazy_vector<slider_params>& params) noexcept { return LazyVector::StrToS32(WaitEVSM(tac, params)); }
	lazy_vector<float> WaitEVSM_F(const array<string, 3u>& tac, const lazy_vector<slider_params>& params) noexcept { return LazyVector::StrToFl(WaitEVSM(tac, params)); }


}

