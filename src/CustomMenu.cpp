#include "CustomMenu.h"
#include "Logger.h"

namespace CustomMenu {


	MenuManager& MenuManager::GetSingleton() {
		static MenuManager singleton;
		return singleton;
	}
	void MenuManager::RegisterMenu() noexcept { // static
		if (const auto ui = RE::UI::GetSingleton(); ui) {
			ui->Register(MenuName, Creator);
			Log::Info("Registered custom menu <{}>"sv, MenuName);
		}
		else {
			Log::Error("Failed to obtain UI singleton. Menu registration aborted!"sv);
		}
	}
	RE::IMenu* MenuManager::Creator() { // static
		return new Menu();
	}

	std::optional<string> MenuManager::GetMenuPath() const noexcept {
		R_locker locker{ lock };
		try {
			std::optional<string> copy{ MenuPath };
			return copy;
		}
		catch (...) {
			return std::nullopt;
		}
	}
	bool MenuManager::SetMenuPath(const string& new_path) noexcept {
		RW_locker locker{ lock };
		try {
			MenuPath = new_path;
			return true;
		}
		catch (...) {
			return false;
		}
	}
	void MenuManager::SetMenuPath(string&& new_path) noexcept {
		RW_locker locker{ lock };
		MenuPath = std::move(new_path);
	}


	Menu::Menu() : super() {
		auto menu = static_cast<super*>(this);
		menu->depthPriority = 0xA;
		menu->inputContext = Context::kMenuMode;
		menu->menuFlags.set(Flag::kPausesGame, Flag::kModal);

		if (const auto input = RE::BSInputDeviceManager::GetSingleton(); !input or !input->IsGamepadEnabled()) {
			menu->menuFlags.set(Flag::kUpdateUsesCursor, Flag::kUsesCursor);
		}

		if (const auto loader = RE::BSScaleformManager::GetSingleton(); loader) {
			if (const auto path = MenuManagerSingleton.GetMenuPath(); !path.has_value()) {
				Log::Error("ODCustomMenu: Menu constructor failed to get menu path (out of memory?)"sv);
			}
			else if (!loader->LoadMovie(menu, menu->uiMovie, path->c_str())) {
				Log::Error("ODCustomMenu: Menu constructor failed to load movie <{}>!"sv, path.value());
			}
		}
		else {
			Log::Error("ODCustomMenu: Menu constructor failed to get loader!"sv);
		}
	}

	RE::UI_MESSAGE_RESULTS Menu::ProcessMessage(RE::UIMessage& msg) {
		return super::ProcessMessage(msg);
		Log::Info(">> ProcessMessage({})"sv, static_cast<int>(msg.type.get()));


		if (msg.type.get() == RE::UI_MESSAGE_TYPE::kScaleformEvent) {
			const RE::BSUIScaleformData* data = static_cast<RE::BSUIScaleformData*>(msg.data);
			if (data and data->scaleformEvent) {
				static const std::array<string_view, 15> event_names{
					"kNone",
					"kMouseMove", "kMouseDown", "kMouseUp", "kMouseWheel",
					"kKeyDown", "kKeyUp",
					"kSceneResize", "kSetFocus", "kKillFocus",
					"kDoShowMouse", "kDoHideMouse", "kDoSetMouseCursor",
					"kCharEvent", "kIMEEvent"
				};
				using EventType = RE::GFxEvent::EventType;
				if (const auto type = data->scaleformEvent->type.get(); static_cast<u32>(type) < event_names.size()) {
					switch (type) {
					case EventType::kMouseMove:
					case EventType::kMouseDown:
					case EventType::kMouseUp:
					case EventType::kMouseWheel: {
						const RE::GFxMouseEvent* mouse = static_cast<RE::GFxMouseEvent*>(data->scaleformEvent);
						Log::Info(">>>> Scaleform Mouse event <{}>: scroll <{:.3f}>, button <{}>"sv,
								  event_names[static_cast<u32>(type)],
								  mouse->scrollDelta,
								  mouse->button
						);
						break;
					}
					case EventType::kKeyDown:
					case EventType::kKeyUp: {
						const RE::GFxKeyEvent* key = static_cast<RE::GFxKeyEvent*>(data->scaleformEvent);
						Log::Info(">>>> Scaleform Key event <{}>: code <{}>, ascii <{}>"sv,
								  event_names[static_cast<u32>(type)],
								  static_cast<u32>(key->keyCode),
								  key->asciiCode
						);
						break;
					}
					default: { Log::Info(">>>> Scaleform event <{}>"sv, event_names[static_cast<u32>(type)]); }
					}
				}
				else {
					Log::Warning(">>>> ProcessMessage Scaleform event but type <{}> out of array bounds!"sv, static_cast<u32>(type));
				}
			}
			else {
				Log::Warning(">>>> ProcessMessage Scaleform event but data or event null (data nullptr:{})!"sv, data == nullptr);
			}
		}

		return super::ProcessMessage(msg);
	}


}

