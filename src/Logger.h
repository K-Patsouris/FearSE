#pragma once
#include <string>
#include <string_view>
#include <format>

class Log {
public:
	enum class Severity : std::size_t {
		info = 0,
		warning = 1,
		error = 2,
		critical = 3,

		prohibit
	};

	using in_msg_type = const std::string&;
	using in_fmt_type = const std::string_view&;
	using Callback_t = void(in_msg_type) noexcept;
	using SinkID = std::uint64_t;

	class SinkToken {
	public:
		SinkToken(SinkToken&& init) noexcept;
		SinkToken& operator=(SinkToken&& rhs) noexcept;
		~SinkToken() noexcept;

		bool IsValid() const noexcept;

		SinkToken() = delete;
		SinkToken(const SinkToken&) = delete;
		SinkToken& operator=(const SinkToken&) = delete;
	private:
		friend class Log;
		SinkToken(const SinkID id_) noexcept;
		SinkID id{};
	};

	struct SeverityPair {
		Severity skse{ Severity::info };
		Severity papyrus{ Severity::info };
	};
	static SinkToken RegisterSink(Callback_t* callback, SeverityPair min_severities) noexcept;

	// Ready-made callbacks for file/console/HUD output, usable with RegisterSink.
	static void FileCallback(in_msg_type msg) noexcept;
	static void ConsoleCallback(in_msg_type msg) noexcept;
	static void HUDCallback(in_msg_type msg) noexcept;

private:
	static void NativeMessage(in_msg_type msg, const Severity severity) noexcept;

public:
	// Call to initialize filestream.
	// Take std::string as argument so we don't have to include <filesystem>, which would reduce compilation time of Log users.
	static bool Init(const std::string& init_filepath) noexcept;

	static void Info(in_fmt_type fmt, auto&&... args) noexcept {
		try { return NativeMessage(std::vformat(fmt, std::make_format_args(args...)), Severity::info); }
		catch (...) {}
	}
	static void Warning(in_fmt_type fmt, auto&&... args) noexcept {
		try { return NativeMessage(std::vformat(fmt, std::make_format_args(args...)), Severity::warning); }
		catch (...) {}
	}
	static void Error(in_fmt_type fmt, auto&&... args) noexcept {
		try { return NativeMessage(std::vformat(fmt, std::make_format_args(args...)), Severity::error); }
		catch (...) {}
	}
	static void Critical(in_fmt_type fmt, auto&&... args) noexcept {
		try { NativeMessage(std::vformat(fmt, std::make_format_args(args...)), Severity::critical); }
		catch (...) {}
	}

	// Meant to be used through Papyrus.
	static void Papyrus(in_msg_type msg, const Severity severity) noexcept;


	// Direct format and print to console, for convenience.
	static void ToConsole(in_fmt_type fmt, auto&&... args) noexcept {
		try { ConsoleCallback(std::vformat(fmt, std::make_format_args(args...))); }
		catch (...) {}
	}
	// Direct format and display notification, for convenience.
	static void ToHUD(in_fmt_type fmt, auto&&... args) noexcept {
		try { HUDCallback(std::vformat(fmt, std::make_format_args(args...))); }
		catch (...) {}
	}

};

