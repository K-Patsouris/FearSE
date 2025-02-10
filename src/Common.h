#pragma once

using std::array;
using std::vector;
using std::deque;
using std::initializer_list;
using std::map;
using std::set;
using std::pair;
using std::tuple;

using std::string;
using std::string_view;
using std::size_t;
using i64 = std::int64_t;
using u64 = std::uint64_t;
using i32 = std::int32_t;
using u32 = std::uint32_t;
using i16 = std::int16_t;
using u16 = std::uint16_t;
using i8 = std::int8_t;
using u8 = std::uint8_t;

using namespace std::literals;

using std::to_string;

using StaticFunc = RE::StaticFunctionTag*;
using IVM = RE::BSScript::IVirtualMachine;
using VM = RE::BSScript::Internal::VirtualMachine;
using StackID = RE::VMStackID;

