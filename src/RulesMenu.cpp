#include "RulesMenu.h"
#include "Common.h"
#include "Logger.h"
#include <atomic>
#include <shared_mutex>

namespace Data::PlayerRules {
	static constexpr string_view swf_name{ "EVM_MBVertical"sv };
	using rules_info = ::Data::rules_info;

	class screen_manager {
	public:

		rules_info show_menu(rules_info prules) noexcept {
			const SKSE::TaskInterface* tasker_local{ nullptr };
			RE::UIMessageQueue* uiMQ_local{ nullptr };
			{
				std::lock_guard locker{ lock };

				tasker = SKSE::GetTaskInterface();
				if (!tasker) {
					Log::Critical("Data::PlayerRules::screen_manager::show_menu: Failed to get TaskInterface! Menu display aborted!"sv);
					return prules;
				}
				uiMQ = RE::UIMessageQueue::GetSingleton();
				if (!uiMQ) {
					Log::Critical("Data::PlayerRules::screen_manager::show_menu: Failed to get UIMessageQueue! Menu display aborted!"sv);
					return prules;
				}
				tasker_local = tasker;
				uiMQ_local = uiMQ;

				info = std::move(prules); // Move redundant, but I want compiler warnings in case I ever accidentally use prules.

				// Setup screens
			}

			done_flag.store(false, std::memory_order_relaxed);

			tasker_local->AddUITask([=] {
				uiMQ_local->AddMessage(swf_name.data(), RE::UI_MESSAGE_TYPE::kShow, nullptr);
				// Log::Info(">> show_menu: Showing in thread <{}>"sv, std::this_thread::get_id());
			});

			done_flag.wait(false, std::memory_order_relaxed);

			std::lock_guard locker{ lock };
			view.reset();
			return info;
		}

		void notify_menu_opened(RE::GPtr<RE::GFxMovieView> menu_view) {
			std::lock_guard locker{ lock };
			view = std::move(menu_view);
			// Invoke swf init

			lazy_vector<RE::GFxValue> args{};
			args.resize(24);
			args[0].SetString("asd\n<font color = '#FF21A43F'>a\na\na\na\na\n<font color = '#FFFF419D'>a\na\na\na\n1\n2\n3\n4\n5\n6666666666666666666666666 66666666666666666666666666666 66666666666666666666666666 666666666666666666666666 6666666666666666666666666666666");
			args[1].SetString("22");				// FontSize
			args[2].SetString("1");				// Align	0:left 1:center 2:right		Don't do to_string. Need living string when invoking.
			args[3].SetString("1");				// CancelIsFirstButton
			args[4].SetString("28");				// Enter
			args[5].SetString("1");				// Escape
			args[6].SetString("15");				// Tab
			args[7].SetString("54");				// Right Shift
			args[8].SetString("42");				// Left Shift
			args[9].SetString("first");
			args[10].SetString("second");
			args[11].SetString("1111111111111a1111111111111 2222222222222b2222222222222 3333333333333c3333333333333 4444444444444d4444444444444 5555555555555e5555555555555 6666666666666f6666666666666");
			args[12].SetString("<font color = '#FF21FF3F'>fourth");
			args[13].SetString("<font color = '#FFFFFF3F'>fifth");
			args[14].SetString("<font color = '#FFDD403F'>sixth");
			args[15].SetString("<font color = '#FF2150FF'>seventh");
			args[16].SetString("<font color = '#FFAAFF10'>eight");
			args[17].SetString("<font color = '#FFBCDEF0'>ninth");
			args[18].SetString("<font color = '#FF01FDBC'>tenth");
			args[19].SetString("<font color = '#FF01FDAA'>eleventh");
			args[20].SetString("<font color = '#FFA10D50'>twelfth");
			args[21].SetString("<font color = '#FFF1AD80'>thirteenth");
			args[22].SetString("<font color = '#FF508DF0'>fourteenth");
			args[23].SetString("<font color = '#FF99FDA0'>fifteenth");

			const auto result = view->Invoke("_root.EVM_MBVerticalMC.InitMessageBox", nullptr, args.data(), static_cast<u32>(args.size()));

			 Log::Info(">> notify_menu_opened: Invoke <{}> in thread <{}>"sv, result, (std::this_thread::get_id()));

		}

		void notify_button_chosen(const u32 btn_id) noexcept {
			btn_id;
			// Log::Info(">> notify_button_chosen: Called with button <{}>"sv, btn_id);
			// If closing
			RE::UIMessageQueue* uiMQ_local{ nullptr };
			{
				std::lock_guard locker{ lock };
				uiMQ_local = uiMQ;
			}

			uiMQ_local->AddMessage(swf_name.data(), RE::UI_MESSAGE_TYPE::kHide, nullptr);
			// Log::Info(">> notify_button_chosen: Hiding in thread <{}>"sv, std::this_thread::get_id());

			done_flag.store(true, std::memory_order_relaxed);
			done_flag.notify_all();
		}

	private:

		enum class screen_id {
			base,
			all_punishers,
			all_granters,
			punisher_details,
			granter_details,
			granter_shop,
			granter_preview,
			granter_customization,

			invalid,

			total
		};

		class base_screen {
		public:

			// void display(const rules_info& info) {}


		};

	private:
		std::atomic<bool> done_flag{};

		std::shared_mutex lock{};
		const SKSE::TaskInterface* tasker{ nullptr };
		RE::UIMessageQueue* uiMQ{ nullptr };
		RE::GPtr<RE::GFxMovieView> view{ nullptr };
		rules_info info{};
	};

	static screen_manager screens{};


	class prmenu : public RE::IMenu {
	public:
		using super = RE::IMenu;
		using msg_res = RE::UI_MESSAGE_RESULTS;
		using msg_type = RE::UI_MESSAGE_TYPE;
		using GRefCountBaseStatImpl::operator new;
		using GRefCountBaseStatImpl::operator delete;

		prmenu() noexcept {
			depthPriority = 0xA;
			inputContext = Context::kMenuMode;
			menuFlags.set(Flag::kPausesGame, Flag::kModal);

			if (const auto input = RE::BSInputDeviceManager::GetSingleton(); !input or !input->IsGamepadEnabled()) {
				menuFlags.set(Flag::kUpdateUsesCursor, Flag::kUsesCursor);
			}

			if (const auto sm = RE::BSScaleformManager::GetSingleton(); sm) {
				if (!sm->LoadMovie(this, uiMovie, swf_name.data())) {
					Log::Error("Data::PlayerRules::prmenu::ctor: Menu constructor failed to load movie <{}>!"sv, swf_name);
				}
			}
			else {
				Log::Error("Data::PlayerRules::prmenu::ctor: Menu constructor failed to get BSScaleformManager!"sv);
			}
		}

		msg_res ProcessMessage(RE::UIMessage& msg) override {
			if (msg.type.get() == msg_type::kShow) {
				// Log::Info(">> ProcessMessage: Notifying menu opened because of message <{}>"sv, static_cast<u32>(msg.type.get()));
				screens.notify_menu_opened(uiMovie);
			}
			else {
				// Log::Info(">>> ProcessMessage: Received and ignored message <{}>"sv, static_cast<u32>(msg.type.get()));
			}
			return super::ProcessMessage(msg);
		}

		static RE::IMenu* creator() noexcept { return new prmenu; }
		static void register_menu() noexcept {
			if (const auto ui = RE::UI::GetSingleton(); ui) {
				// Make it static because a nameless inline lambda would get destroyed at the end of Register(), I think.
				// static auto creator = []() -> RE::IMenu* { return new prmenu; };
				ui->Register(swf_name, creator);
				Log::Info("Data::PlayerRules::RegisterODMenu: Registered <{}>"sv, swf_name);
			}
			else {
				Log::Error("Data::PlayerRules::RegisterODMenu: Failed to obtain UI singleton. OD Menu registration aborted!"sv);
			}
		}

	};


	
	// Interface

	void register_pr_menu() noexcept { prmenu::register_menu(); }

	rules_info show_menu(rules_info prules) noexcept { return screens.show_menu(prules); }

	void menu_button_chosen(const u32 btn_id) noexcept { screens.notify_button_chosen(btn_id); }





}




