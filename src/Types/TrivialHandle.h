#pragma once

namespace TrivialHandle {
	
	struct trivial_handle {
		constexpr trivial_handle() noexcept = default;
		constexpr trivial_handle(trivial_handle&&) noexcept = default;
		constexpr trivial_handle(const trivial_handle&) noexcept = default;
		constexpr trivial_handle& operator=(trivial_handle&&) noexcept = default;
		constexpr trivial_handle& operator=(const trivial_handle&) noexcept = default;
		constexpr ~trivial_handle() noexcept = default;

		trivial_handle(const RE::ActorHandle init) noexcept : handle(init.native_handle()) {}
		trivial_handle& operator=(const RE::ActorHandle rhs) noexcept { this->handle = rhs.native_handle(); return *this; }

		trivial_handle(RE::Actor* act) noexcept : handle(RE::ActorHandle{ act }.native_handle()) {}
		trivial_handle& operator=(RE::Actor* rhs) noexcept { this->handle = RE::ActorHandle{ rhs }.native_handle(); return *this; }

		constexpr bool operator==(const trivial_handle& rhs) const noexcept { return this->handle == rhs.handle; }
		constexpr bool operator!=(const trivial_handle& rhs) const noexcept { return this->handle != rhs.handle; }

		bool operator==(const RE::ActorHandle& rhs) const noexcept { return this->handle == rhs.native_handle(); }
		bool operator!=(const RE::ActorHandle& rhs) const noexcept { return this->handle != rhs.native_handle(); }

		constexpr operator bool() const noexcept { return handle != 0; }
		constexpr bool is_valid() const noexcept { return handle != 0; }

		constexpr std::uint32_t native_handle() const noexcept { return handle; }

		constexpr void reset() noexcept { handle = 0; }
		RE::ActorPtr get() const noexcept { // Works
			RE::ActorPtr ptr;
			get_smart_ptr(*this, ptr);
			return ptr;
		}

		constexpr void swap(trivial_handle& other) noexcept {
			const std::uint32_t temp = this->handle;
			this->handle = other.handle;
			other.handle = temp;
		}

	private:
		std::uint32_t handle{};

		// Getting ActorHandle from Actor* by just copying BSPointerHandleManagerInterface again crashes. Probably cause non-trivial return type fuckery.
		// But getting ActorPtr from u32& works fine.
		static bool get_smart_ptr(const trivial_handle& hnd, RE::ActorPtr& ptr) noexcept {
			using func_t = decltype(&trivial_handle::get_smart_ptr);
			REL::Relocation<func_t> func{ RELOCATION_ID(12204, 12332) }; // Copied from BSPointerHandleManagerInterface in BSPointerHandle.h
			return func(hnd, ptr);
		}

	};
	static_assert(std::is_trivially_copyable_v<trivial_handle>);



}
