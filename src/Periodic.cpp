#pragma once
#include "Periodic.h"
#include "Logger.h"
#include "Data.h"
#include <shared_mutex>
#include <chrono>


namespace Periodic {

	namespace Timer {

		using std::chrono::steady_clock;
		using std::chrono::milliseconds;
		using std::chrono::seconds;
		using std::chrono::duration;
		using std::chrono::duration_cast;


		std::shared_mutex vars_lock{}; // pause_waiter and waiteds always accessed with vars_lock locked.
		bool active{ false };
		bool paused{ false };
		std::condition_variable_any update_waiter{};
		std::atomic<bool> pause_waiter{ false };
		Data::Shared::UpdateDeltas waiteds{};

		constexpr milliseconds IntervalDefault = 7s;
		constexpr milliseconds IntervalLong = 120s;


		static void Update() {
			Data::Shared::UpdateDeltas deltas{};
			while (true) {

				pause_waiter.wait(true); // Wait while paused

				auto start = steady_clock::now();
				std::unique_lock locker{ vars_lock };
				if (const milliseconds adjusted_interval{ (IntervalDefault - deltas.delta_default) }; adjusted_interval > 0ms) {
					// Log::Info("Timer::Update: waiting for {} ms"sv, adjusted_interval.count());
					update_waiter.wait_for(locker, adjusted_interval); // Only wait if not ready for default update
				}
				auto end = steady_clock::now();
				deltas += duration_cast<milliseconds>(end - start);

				if (!active) {
					// Log::Info("Timer::Update: deactivated. Returning..."sv);
					return;
				}
				if (paused) {
					// Log::Info("Timer::Update: interrupted by pause"sv);
					continue;
				}
				// Log::Info("Timer::Update: proceeding with update"sv);

				locker.unlock(); // Unlock after checking vars

				Data::Shared::UpdateKinds kinds{};
				Data::Shared::UpdateDeltas deltas_copy{ deltas };

				if (deltas.delta_default >= IntervalDefault) { // Start default updates no more often than once per IntervalDefault
					kinds.mark(Data::Shared::UpdateKinds::DefaultUpdate);
					deltas.delta_default = 0ms;
				}
				if (deltas.delta_long >= IntervalLong) { // Start long updates no more often than once per IntervalLong
					kinds.mark(Data::Shared::UpdateKinds::LongUpdate);
					deltas.delta_long = 0ms;
				}

				if (kinds.any_marked()) {
					// Log::Info("Timer:Update: starting update"sv);
					Data::Shared::Update(kinds, deltas_copy);
					// Log::Info("Timer::Update: done with update"sv);
					deltas += duration_cast<milliseconds>(steady_clock::now() - end); // Account for time spent doing updates. deltas and end are not shared so no lock needed.
				}

			}
		}



		void Start() noexcept {
			std::lock_guard locker{ vars_lock };
			if (active) {
				Log::Error("Timer::Start: Cannot start timer because it is already active!"sv);
				return;
			}
			if (auto ui = RE::UI::GetSingleton(); ui) {
				active = true;
				paused = (ui->numPausesGame == 0);
				pause_waiter.store(paused);
				std::thread{ &Update }.detach();
			}
			else {
				Log::Critical("Timer::Start: UI singleton null! Cannot start timer because game pause state cannot be determined!"sv);
			}
		}
		void Stop() noexcept { // Serialization::RevertCallback calls this!
			std::lock_guard locker{ vars_lock };
			active = false; // Log::Error("Timer::Stop: Cannot stop timer because it is not active! (safe to ignore)"sv);
			update_waiter.notify_all();
		}
		void MenuEvent() noexcept {
			// 	UI failure		UI pauses		Action
			// 		o				-			stop	safest?
			// 		x				o			stop
			// 		x				x			start
			std::lock_guard locker{ vars_lock };
			if (!active) {
				// Log::Error("Timer::MenuEvent: received event while not active! Aborted."sv);
				return;
			}
			if (auto ui = RE::UI::GetSingleton(); ui and (ui->numPausesGame == 0)) { // Not pausing
				paused = false;
				pause_waiter.store(paused);
				pause_waiter.notify_all();
				// Log::Info("Timer::MenuEvent: tried to unpause."sv);
			}
			else {
				if (!ui) {
					Log::Warning("Timer::MenuEvent: failed to get UI singleton! Temporarily force-stopping updates..."sv);
				}
				paused = true;
				pause_waiter.store(paused);
				update_waiter.notify_all();
				// Log::Info("Timer::MenuEvent: tried to interrupt and pause."sv);
			}
		}

	}

}
