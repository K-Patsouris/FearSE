#include "Logger.h"

#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <ctime>
#include <array>

#include <Types/SyncTypes.h>


using std::chrono::system_clock;
using Severity = Log::Severity;
using in_msg_type = Log::in_msg_type;
using in_fmt_type = Log::in_fmt_type;
using spinlock = SyncTypes::spinlock;
using shared_spinlock = SyncTypes::shared_spinlock;


class SinkHolder {
public:
	using Callback_t = Log::Callback_t;
	using SinkID = Log::SinkID;
	using SinkToken = Log::SinkToken;
	using Severity = Log::Severity;
	using SeverityPair = Log::SeverityPair;

	struct Sink {
		SinkID id{ 0 };
		SeverityPair min_severities{ .skse = Severity::prohibit, .papyrus = Severity::prohibit };
		Callback_t* callback{ nullptr };
	};
	static_assert(std::is_aggregate_v<Sink>);
	static_assert(std::is_nothrow_default_constructible_v<Sink>);
	static_assert(std::is_nothrow_copy_constructible_v<Sink>);
	static_assert(std::is_nothrow_move_constructible_v<Sink>);
	static_assert(std::is_nothrow_copy_assignable_v<Sink>);
	static_assert(std::is_nothrow_move_assignable_v<Sink>);

	SinkHolder() noexcept = default;
	SinkHolder(const SinkHolder&) = delete;
	SinkHolder& operator=(const SinkHolder&) = delete;

	SinkID AddSink(Callback_t* callback, const SeverityPair min_severities) noexcept {
		if (callback != nullptr) {
			SinkID id{ next++ };
			exclusive_locker locker{ lock };
			try {
				sinks.emplace_back(id, min_severities, callback);
				return id;
			}
			catch (...) {}
		}
		return SinkID{ 0 };
	}

	void RemoveSink(const SinkID id) noexcept {
		if (id != 0) {
			exclusive_locker locker{ lock };
			for (auto& sink : sinks) {
				if (sink.id == id) {
					sink = std::move(sinks.back());
					sinks.pop_back();
				}
			}
		}
	}

	void DistributeSKSEMessage(in_msg_type msg, const Severity severity) const noexcept {
		if (severity < Severity::prohibit) {
			shared_locker locker{ lock };
			for (const auto& sink : sinks) {
				if (sink.min_severities.skse <= severity) {
					sink.callback(msg);
				}
			}
		}
	}
	void DistributePapyrusMessage(in_msg_type msg, const Severity severity) const noexcept {
		if (severity < Severity::prohibit) {
			shared_locker locker{ lock };
			for (const auto& sink : sinks) {
				if (sink.min_severities.papyrus <= severity) {
					sink.callback(msg);
				}
			}
		}
	}

private:
	using exclusive_locker = SyncTypes::noexlock_guard<shared_spinlock, SyncTypes::LockingMode::Exclusive>;
	using shared_locker = SyncTypes::noexlock_guard<shared_spinlock, SyncTypes::LockingMode::Shared>;
	mutable shared_spinlock lock{};
	std::uint64_t next{ 1 };
	std::vector<Sink> sinks{};
};
static SinkHolder sink_holder{};

class FileHandler {
private:
	class MessageQueue {
	public:
		explicit MessageQueue() noexcept {
			first_pending = queue.begin();
			first_free = queue.begin();
			fully_cleared = true;
		}
		MessageQueue(const MessageQueue&) = default;
		MessageQueue(MessageQueue&&) noexcept = default;
		MessageQueue& operator=(const MessageQueue&) = default;
		MessageQueue& operator=(MessageQueue&&) noexcept = default;
		~MessageQueue() noexcept = default;

		[[nodiscard]] bool Push(in_msg_type msg) noexcept {
			if (first_free != queue.end()) {
				try {
					*first_free = msg;
					fully_cleared = false;
					return (++first_free) == queue.end(); // Need flush
				}
				catch (...) {}	
			}
			return false;
		}

		bool Flush(std::ofstream& ofs) noexcept {
			if (!fully_cleared) {
				int success = 0;
				try {
					for (; first_pending != first_free; ++first_pending) {
						ofs << (*first_pending + '\n');
					}
					first_pending = queue.begin();
					first_free = queue.begin();
					success += 1;
				}
				catch (...) {}
				try { // Try flushing regardless of push failures.
					ofs.flush();
					success += 1;
				}
				catch (...) {}
				fully_cleared = (success == 2);
			}
			return fully_cleared;
		}

	private:
		std::array<string, 10> queue{};
		std::array<string, 10>::iterator first_pending{};
		std::array<string, 10>::iterator first_free{};
		bool fully_cleared{};
	};

public:
	FileHandler() noexcept {
		write_ptr.store(write_noop, std::memory_order_relaxed);
		flush_ptr.store(flush_noop, std::memory_order_relaxed);
	}
	FileHandler(const FileHandler&) = delete;
	FileHandler& operator=(const FileHandler&) = delete;

	bool Init(const std::filesystem::path& init_filepath) noexcept {
		exclusive_locker locker{ lock };
		if (ofs.is_open()) {
			return false;
		}
		try {
			if (ofs.open(init_filepath); ofs.is_open()) {
				write_ptr.store(write_impl, std::memory_order_relaxed);
				flush_ptr.store(flush_impl, std::memory_order_relaxed);
				std::thread{ [this] {
					std::this_thread::sleep_for(10s);
					for (;;) {
						auto interval = 3s;
						bool success = true;
						if (success = lock.try_lock(1us); success) {
							success = queue.Flush(ofs);
							lock.unlock();
						}
						if (!success) {
							interval = 1s;
						}
						std::this_thread::sleep_for(interval);
					}
				}}.detach();
				return true;
			}
		}
		catch (...) {}
		return false;
	}

	void Write(in_msg_type msg) noexcept { write_ptr.load(std::memory_order_relaxed)(this, msg); }
	bool Flush() noexcept { flush_ptr.load(std::memory_order_relaxed)(this); }

private:
	using exclusive_locker = SyncTypes::noexlock_guard<spinlock, SyncTypes::LockingMode::Exclusive>;
	spinlock lock{};
	MessageQueue queue{};
	std::ofstream ofs{};
	std::atomic<void(*)(FileHandler*, in_msg_type) noexcept> write_ptr;
	std::atomic<bool(*)(FileHandler*) noexcept> flush_ptr;

	static void write_noop(FileHandler*, in_msg_type) noexcept { return; }
	static bool flush_noop(FileHandler*) noexcept { return false; }
	static void write_impl(FileHandler* this_, in_msg_type msg) noexcept {
		exclusive_locker locker{ this_->lock };
		if (this_->queue.Push(msg)) {
			this_->queue.Flush(this_->ofs);
		}
	}
	static bool flush_impl(FileHandler* this_) noexcept {
		exclusive_locker locker{ this_->lock };
		return this_->queue.Flush(this_->ofs);
	}

};
static FileHandler file_handler{};


Log::SinkToken::SinkToken(const SinkID id_) noexcept : id(id_) {}
Log::SinkToken::SinkToken(SinkToken&& init) noexcept : id(init.id) { init.id = 0; }
Log::SinkToken& Log::SinkToken::operator=(SinkToken&& rhs) noexcept {
	if (this != std::addressof(rhs)) {
		this->id = rhs.id;
		rhs.id = 0;
	}
	return *this;
}
Log::SinkToken::~SinkToken() noexcept { sink_holder.RemoveSink(this->id); }
bool Log::SinkToken::IsValid() const noexcept { return id != 0; }



bool Log::Init(const std::string& init_filepath) noexcept { return file_handler.Init(init_filepath); }


Log::SinkToken Log::RegisterSink(Callback_t* callback, SeverityPair min_severities) noexcept {
	return SinkToken{ sink_holder.AddSink(callback, min_severities) };
}

void Log::FileCallback(in_msg_type msg) noexcept { file_handler.Write(std::move(msg)); }
void Log::ConsoleCallback(in_msg_type msg) noexcept {
	if (auto console = RE::ConsoleLog::GetSingleton(); console) {
		if (msg.length() > 1000) { // 1030 doesn't seem to crash. 1050 does. CBA and don't actually want to fine-grain it further. 1000 is more than enough.
			try {
				const string trimmed(msg, 0, 1000);
				console->Print(trimmed.c_str());
			}
			catch (...) {}
		}
		else {
			console->Print(msg.c_str());
		}
	}
}
void Log::HUDCallback(in_msg_type msg) noexcept {
	if (msg.length() > 100) { // I stopped testing at 4000 (works just fine) but even 100 chars are basically illegible in any reasonable HUD so chop it
		try {
			const string trimmed(msg, 0, 100);
			RE::DebugNotification(trimmed.c_str());
		}
		catch (...) {}
	}
	else {
		RE::DebugNotification(msg.c_str());
	}
}


void Log::NativeMessage(in_msg_type newmsg, const Severity severity) noexcept {
	try {
		static auto prefix = [](const Severity severity = Severity::prohibit) {
			static constexpr std::array<string_view, 5> SEVERITY { "<info> ", "<warning> ", "<error> ", "<critical> ", "" };
			std::tm tm{};
			auto time = system_clock::to_time_t(system_clock::now());
			localtime_s(&tm, &time);
			return string{ std::format("{:0>2}:{:0>2}:{:0>2} ", tm.tm_hour, tm.tm_min, tm.tm_sec) += SEVERITY[static_cast<std::size_t>(severity)] };
		};
		sink_holder.DistributeSKSEMessage({ prefix(severity) + newmsg }, severity);
	}
	catch (...) {}
}

void Log::Papyrus(in_msg_type newmsg, const Severity severity) noexcept {
	try {
		static auto prefix = [](const Severity severity = Severity::prohibit) {
			static constexpr std::array<string_view, 5> SEVERITY { "Papyrus <info> ", "Papyrus <warning> ", "Papyrus <error> ", "Papyrus <critical> ", "Papyrus " };
			std::tm tm{};
			auto time = system_clock::to_time_t(system_clock::now());
			localtime_s(&tm, &time);
			return string{ std::format("{:0>2}:{:0>2}:{:0>2} ", tm.tm_hour, tm.tm_min, tm.tm_sec) += SEVERITY[static_cast<std::size_t>(severity)] };
		};
		sink_holder.DistributePapyrusMessage({ prefix(severity) + newmsg }, severity);
	}
	catch (...) {}
}


