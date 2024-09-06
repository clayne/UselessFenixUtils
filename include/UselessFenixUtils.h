#pragma once
#include <boost\type_traits\function_traits.hpp>
#define NOGDI
#pragma warning(push)
#pragma warning(disable: 4702)
#include <xbyak\xbyak.h>
#pragma warning(pop)

#include <glm/glm.hpp>

#include "json/json.h"
#include "magic_enum.hpp"

#ifdef WITH_IMGUI
#include "UselessImguiUtils.h"
#endif  // WITH_IMGUI
#ifdef WITH_DRAWING
#	include "UselessDebugRenderUtils.h"
#endif  // WITH_DRAWING

namespace FenixUtils
{
	template <int id, typename x_Function>
	class _generic_foo_;

	template <int id, typename T, typename... Args>
	class _generic_foo_<id, T(Args...)>
	{
	public:
		static T eval(Args... args)
		{
			using func_t = T(Args...);
			REL::Relocation<func_t> func{ REL::ID(id) };
			return func(std::forward<Args>(args)...);
		}
	};

	template <size_t BRANCH_TYPE, uint64_t ID, size_t offset = 0, bool call = false>
	auto add_trampoline(Xbyak::CodeGenerator* xbyakCode)
	{
		constexpr REL::ID funcOffset = REL::ID(ID);
		auto funcAddr = funcOffset.address();
		auto size = xbyakCode->getSize();
		auto& trampoline = SKSE::GetTrampoline();
		auto result = trampoline.allocate(size);
		std::memcpy(result, xbyakCode->getCode(), size);
		if constexpr (!call)
			return trampoline.write_branch<BRANCH_TYPE>(funcAddr + offset, (std::uintptr_t)result);
		else
			return trampoline.write_call<BRANCH_TYPE>(funcAddr + offset, (std::uintptr_t)result);
	}

	template <int ID, int offset = 0x0>
	void writebytes(const std::string_view& data)
	{
		REL::safe_write(REL::ID(ID).address() + offset, data.data(), data.size());
	}

	enum class LineOfSightLocation : uint32_t
	{
		kNone,
		kEyes,
		kHead,
		kTorso,
		kFeet
	};
	
	namespace Geom
	{
		// Axis: y -- forward, x -- right, z angle = 0 => y = 1, x = 0.
		// Angles: x:-pi/2 (up)..pi/2 (down), y:0, z:0 (along y)..2pi

		float NiASin(float alpha);

		// --- Vector (dir) to angles ---

		float GetUnclampedZAngleFromVector(const RE::NiPoint3& V);
		float GetZAngleFromVector(const RE::NiPoint3& V);
		void CombatUtilities__GetAimAnglesFromVector(const RE::NiPoint3& V, float& rotZ, float& rotX);
		RE::Projectile::ProjectileRot rot_at(const RE::NiPoint3& dir);
		RE::Projectile::ProjectileRot rot_at(const RE::NiPoint3& from, const RE::NiPoint3& to);

		// ^^^ Vector (dir) to angles ^^^

		// --- rotations ---

		RE::NiPoint3 angles2dir(const RE::NiPoint3& angles);
		// Get vector with len=r, with euler=angles
		RE::NiPoint3 rotate(float r, const RE::NiPoint3& angles);
		RE::NiPoint3 rotate(const RE::NiPoint3& A, const RE::NiPoint3& angles);
		RE::NiPoint3 rotate(const RE::NiPoint3& P, float alpha, const RE::NiPoint3& axis_dir);
		RE::NiPoint3 rotate(const RE::NiPoint3& P, float alpha, const RE::NiPoint3& origin, const RE::NiPoint3& axis_dir);
		// rotate vel_cur to dir_final with max angle max_alpha
		RE::NiPoint3 rotateVel(const RE::NiPoint3& vel_cur, float max_alpha, const RE::NiPoint3& dir_final);

		// ^^^ rotations ^^^

		namespace Projectile
		{
			void update_node_rotation(RE::Projectile* proj, const RE::NiPoint3& V);
			void update_node_rotation(RE::Projectile* proj);
			void aimToPoint(RE::Projectile* proj, const RE::NiPoint3& P);
		}

		namespace Actor
		{
			RE::NiPoint3 CalculateLOSLocation(RE::TESObjectREFR* refr, LineOfSightLocation los_loc);
			// get point `caster` is looking at
			RE::NiPoint3 raycast(RE::Actor* caster);
			RE::NiPoint3 AnticipatePos(RE::Actor* a, float dtime = 0);
			LineOfSightLocation Actor__CalculateLOS(RE::Actor* caster, RE::Actor* target, float viewCone);
			bool ActorInLOS(RE::Actor* caster, RE::Actor* target, float viewCone);
			// get polar_angle from los a to target_pos
			float Actor__CalculateAimDelta(RE::Actor* a, const RE::NiPoint3& target_pos);
			// get CalculateAimDelta and compare it with viewCone / 2
			bool Actor__IsPointInAimCone(RE::Actor* a, RE::NiPoint3* target_pos, float viewCone);
		}
	}

	namespace Random
	{
		// random(0..1) <= chance
		bool RandomBoolChance(float prop);
		float Float(float min, float max);
		float FloatChecked(float min, float max);
		float FloatTwoPi();
		float Float0To1();
		float FloatNeg1To1();
		int32_t random_range(int32_t min, int32_t max);
	}

	namespace Json
	{
		// Returns mod mask number or -1 if missing
		int get_mod_index(std::string_view name);
		// Returns number from `Skyrim.esm|0x1000` or `0x10100000`
		uint32_t get_formid(const std::string& name);

		// --- getting values ---

		RE::NiPoint3 getPoint3(const ::Json::Value& jobj, const std::string& field_name);
		template <RE::NiPoint3 default_val = RE::NiPoint3(0, 0, 0)>
		RE::NiPoint3 mb_getPoint3(const ::Json::Value& jobj, const std::string& field_name)
		{
			return jobj.isMember(field_name) ? getPoint3(jobj, field_name) : default_val;
		}
		RE::Projectile::ProjectileRot getPoint2(const ::Json::Value& jobj, const std::string& field_name);
		RE::Projectile::ProjectileRot mb_getPoint2(const ::Json::Value& jobj, const std::string& field_name);
		std::string getString(const ::Json::Value& jobj, const std::string& field_name);
		std::string mb_getString(const ::Json::Value& jobj, const std::string& field_name);
		float getFloat(const ::Json::Value& jobj, const std::string& field_name);
		template <float default_val = 0.0f>
		float mb_getFloat(const ::Json::Value& jobj, const std::string& field_name)
		{
			return jobj.isMember(field_name) ? getFloat(jobj, field_name) : default_val;
		}
		bool getBool(const ::Json::Value& jobj, const std::string& field_name);
		uint32_t getUint32(const ::Json::Value& jobj, const std::string& field_name);

		template <typename Enum>
		Enum string2enum(const std::string& val)
		{
			return magic_enum::enum_cast<Enum>(val).value();
		}

		// Get string data from val and return enum
		template <typename Enum>
		Enum read_enum(const ::Json::Value& val)
		{
			return string2enum<Enum>(val.asString());
		}

		// Get string data from jobj[field_name] and return enum
		template <typename Enum>
		Enum read_enum(const ::Json::Value& jobj, const std::string& field_name)
		{
			assert(jobj.isMember(field_name));
			return read_enum<Enum>(jobj[field_name]);
		}

		// Get data from jobj[field_name], if present, return def otherwise
		template <auto def>
		auto mb_read_field(const ::Json::Value& jobj, const std::string& field_name)
		{
			if constexpr (std::is_same_v<decltype(def), bool>) {
				if (jobj.isMember(field_name))
					return getBool(jobj, field_name);
				else
					return def;
			} else if constexpr (std::is_same_v<decltype(def), uint32_t>) {
				if (jobj.isMember(field_name))
					return getUint32(jobj, field_name);
				else
					return def;
			} else {
				if (jobj.isMember(field_name))
					return read_enum<decltype(def)>(jobj, field_name);
				else
					return def;
			}
		}

		// ^^^ getting values ^^^
	}

	namespace Timer
	{
		float get_time();
		bool passed(float timestamp, float interval);
		bool passed(RE::AITimer& timer);
		void updateAndWait(RE::AITimer& timer, float interval);
	}

	namespace Behavior
	{
		RE::hkbNode* lookup_node(RE::hkbBehaviorGraph* root_graph, const char* name);
		int32_t get_implicit_id_variable(RE::hkbBehaviorGraph* graph, const char* name);
		int32_t get_implicit_id_event(RE::hkbBehaviorGraph* graph, const char* name);
		const char* get_event_name_implicit(RE::hkbBehaviorGraph* graph, int32_t implicit_id);
		const char* get_event_name_explicit(RE::hkbBehaviorGraph* graph, int32_t explicit_id);
		const char* get_variable_name(RE::hkbBehaviorGraph* graph, int32_t ind);
	}

	struct Plinterp
	{
		std::vector<std::pair<float, float>> data;

		float eval(float val);

		void init(const ::Json::Value& points);
	private:
		std::pair<float, float> get_point2(const ::Json::Value& val) { return { val[0].asFloat(), val[1].asFloat() }; }
	};

	namespace IO
	{
		template <typename T>
		void writeT(std::ofstream& file, const T& val)
		{
			file.write((char*)&val, sizeof(T));
		}

		void write_string(std::ofstream& file, const std::string_view& sv);
		void write_string(std::ofstream& file, const char* s);
		std::string read_string(std::ifstream& file);

		template <typename T>
		T readT(std::ifstream& file)
		{
			T ans;
			file.read((char*)&ans, sizeof(T));
			return ans;
		}

		template <typename T>
		void readT(std::ifstream& file, T& ans)
		{
			file.read((char*)&ans, sizeof(T));
		}
	}

	void tolower(std::string& data);

	float Projectile__GetSpeed(RE::Projectile* proj);
	void Projectile__set_collision_layer(RE::Projectile* proj, RE::COL_LAYER collayer);

	void TESObjectREFR__SetAngleOnReferenceX(RE::TESObjectREFR* refr, float angle_x);
	void TESObjectREFR__SetAngleOnReferenceZ(RE::TESObjectREFR* refr, float angle_z);

	RE::TESObjectARMO* GetEquippedShield(RE::Actor* a);
	RE::EffectSetting* getAVEffectSetting(RE::MagicItem* mgitem);

	void damage_stamina_withdelay(RE::Actor* a, float val);
	void damageav_attacker(RE::Actor* victim, RE::ACTOR_VALUE_MODIFIERS::ACTOR_VALUE_MODIFIER i1, RE::ActorValue i2, float val, RE::Actor* attacker);
	void damageav(RE::Actor* a, RE::ActorValue av, float val);
	RE::TESObjectWEAP* get_UnarmedWeap();
	float PlayerCharacter__get_reach(RE::Actor* a);
	float GetHeadingAngle(RE::TESObjectREFR* a, const RE::NiPoint3& a_pos, bool a_abs);
	void UnequipItem(RE::Actor* a, RE::TESBoundObject* item);
	void knock(RE::Actor* target, RE::Actor* aggressor, float KnockDown);
	void cast_spell(RE::Actor* victim, RE::Actor* attacker, RE::SpellItem* spell);
	float clamp01(float t);
	float clamp(float t, float min, float max);

	template <float val = 0.5f>
	void stagger(RE::Actor* victim, RE::Actor* attacker = nullptr)
	{
		float stagDir = 0.0f;
		if (attacker && victim->GetHandle() != attacker->GetHandle()) {
			auto heading = GetHeadingAngle(victim, attacker->GetPosition(), false);
			stagDir = (heading >= 0.0f) ? heading / 360.0f : (360.0f + heading) / 360.0f;
		}

		victim->SetGraphVariableFloat("staggerDirection", stagDir);
		victim->SetGraphVariableFloat("staggerMagnitude", val);
		victim->NotifyAnimationGraph("staggerStart");
	}

	template<float min, float max>
	static float lerp(float k)
	{
		return min + (max - min) * k;
	}

	void play_sound(RE::TESObjectREFR* a, int formid);
	void play_impact(RE::TESObjectREFR* a, RE::BGSImpactData* impact, RE::NiPoint3* P_V, RE::NiPoint3* P_from, RE::NiNode* bone);
	bool PlayIdle(RE::AIProcess* proc, RE::Actor* attacker, RE::DEFAULT_OBJECT smth, RE::TESIdleForm* idle, bool a5, bool a6, RE::Actor* target);
	float get_total_av(RE::Actor* a, RE::ActorValue av);
	bool TESObjectREFR__HasEffectKeyword(RE::TESObjectREFR* a, RE::BGSKeyword* kwd);
	
	// ignore cells
	float get_dist2(RE::TESObjectREFR* a, RE::TESObjectREFR* b);


	RE::BGSAttackDataPtr get_attackData(RE::Actor* a);
	bool is_powerattacking(RE::Actor* a);
	RE::TESObjectWEAP* get_weapon(RE::Actor* a);
	bool is_human(RE::Actor* a);
	void set_RegenDelay(RE::AIProcess* proc, RE::ActorValue av, float time);
	void FlashHudMenuMeter__blink(RE::ActorValue av);
	float get_regen(RE::Actor* a, RE::ActorValue av);
	void damagestamina_delay_blink(RE::Actor* a, float cost);
	float getAV_pers(RE::Actor* a, RE::ActorValue av);
	float lerp(float x, float Ax, float Ay, float Bx, float By);

	void AddItem(RE::Actor* a, RE::TESBoundObject* item, RE::ExtraDataList* extraList, int count, RE::TESObjectREFR* fromRefr);
	void AddItemPlayer(RE::TESBoundObject* item, int count);
	int RemoveItemPlayer(RE::TESBoundObject* item, int count);

	int get_item_count(RE::Actor* a, RE::TESBoundObject* item);
	uint32_t* placeatme(RE::TESObjectREFR* a, uint32_t* handle, RE::TESBoundObject* form, RE::NiPoint3* pos, RE::NiPoint3* angle);
	RE::TESObjectREFR* placeatmepap(RE::TESObjectREFR* a, RE::TESBoundObject* form, int count);

	template <typename... Args>
	void notification(const char* format, Args... a)
	{
		char sMsgBuff[128] = { 0 };
		sprintf_s(sMsgBuff, format, a...);
		RE::DebugNotification(sMsgBuff);
	}

	template <typename... Args>
	std::string vformat(const std::string_view& format, Args&&... args)
	{
		try {
			return std::vformat(format, std::make_format_args(std::forward<Args>(args)...));
		} catch (...) {
			return "";
		}
	}

	template <typename... Args>
	void notification(const std::string_view& format, Args&&... a)
	{
		RE::DebugNotification(vformat(format, std::forward<Args>(a)...).c_str());
	}

	template <typename... Args>
	void messagebox(const std::string_view& format, Args&&... a)
	{
		RE::DebugMessageBox(vformat(format, std::forward<Args>(a)...).c_str());
	}

	template <typename Key, typename Value, std::size_t Size>
	struct Map
	{
		std::array<std::pair<Key, Value>, Size> data;

		[[nodiscard]] constexpr Value at(const Key& key) const
		{
			const auto itr = std::find_if(begin(data), end(data), [&key](const auto& v) { return v.first == key; });
			if (itr != end(data)) {
				return itr->second;
			} else {
				throw std::range_error("Not Found");
			}
		}

		[[nodiscard]] constexpr bool contains_value(const Value& val) const
		{
			const auto itr = std::find_if(begin(data), end(data), [&val](const auto& v) { return v.second == val; });
			return itr != end(data);
		}
	};

	bool is_playable_spel(RE::SpellItem* spel);
}
using FenixUtils::_generic_foo_;
using FenixUtils::add_trampoline;

template <>
struct fmt::formatter<RE::NiPoint3> : formatter<float>
{
	format_context::iterator format(const RE::NiPoint3& c, format_context& ctx) const;
};
template <>
struct fmt::formatter<RE::NiQuaternion> : formatter<float>
{
	format_context::iterator format(const RE::NiQuaternion& c, format_context& ctx) const;
};

constexpr uint32_t hash(const char* data, size_t const size) noexcept
{
	uint32_t hash = 5381;

	for (const char* c = data; c < data + size; ++c) {
		hash = ((hash << 5) + hash) + (unsigned char)*c;
	}

	return hash;
}

constexpr char to_lower(char c)
{
	if (c >= 'A' && c <= 'Z')
		c += 32;
	return c;
}

constexpr uint32_t hash_lowercase(const char* data, size_t const size) noexcept
{
	uint32_t hash = 5381;

	for (const char* c = data; c < data + size; ++c) {
		hash = ((hash << 5) + hash) + (unsigned char)to_lower(static_cast<unsigned char>(*c));
	}

	return hash;
}

constexpr uint32_t operator"" _h(const char* str, size_t size) noexcept
{
	return hash(str, size);
}

constexpr uint32_t operator"" _hl(const char* str, size_t size) noexcept
{
	return hash_lowercase(str, size);
}

#include "SimpleIni.h"
class SettingsBase
{
protected:
	class Hotkeys
	{
		static void strip(std::string& str)
		{
			if (str.length() == 0) {
				return;
			}

			auto start_it = str.begin();
			auto end_it = str.rbegin();
			while (std::isspace(*start_it)) {
				++start_it;
				if (start_it == str.end())
					break;
			}
			while (std::isspace(*end_it)) {
				++end_it;
				if (end_it == str.rend())
					break;
			}
			auto start_pos = start_it - str.begin();
			auto end_pos = end_it.base() - str.begin();
			str = start_pos <= end_pos ? std::string(start_it, end_it.base()) : "";
		}

		enum class AddKeys : uint32_t
		{
			Shift,
			Ctrl,
			Alt,

			Total
		};
		static inline constexpr size_t AddKeysTotal = static_cast<size_t>(AddKeys::Total);

		int key;
		std::array<int, AddKeysTotal> additionals = { {} };

		bool isPressed_adds()
		{
			bool ans = true;
			for (int k : additionals) {
				ans = ans && (k == 0 || RE::BSInputDeviceManager::GetSingleton()->GetKeyboard()->IsPressed(k));
			}
			return ans;
		}

	public:
		Hotkeys(int key = 71) : key(key) {}

		void load_key(std::string s)
		{
			using K = RE::BSKeyboardDevice::Key;

			strip(s);
			int keycode = std::stoi(s);

			AddKeys type = AddKeys::Total;
			switch (keycode) {
			case K::kRightAlt:
			case K::kLeftAlt:
				type = AddKeys::Alt;
				break;
			case K::kLeftControl:
			case K::kRightControl:
				type = AddKeys::Ctrl;
				break;
			case K::kLeftShift:
			case K::kRightShift:
				type = AddKeys::Shift;
				break;
			default:
				break;
			}

			if (type != AddKeys::Total) {
				additionals[static_cast<size_t>(type)] = keycode;
			} else {
				key = keycode;
			}
		}

		bool isPressed(int k) { return k == key && isPressed_adds(); }
	};

	static void ReadHotkey(const CSimpleIniA& ini, const char* section, const char* setting, Hotkeys& ans)
	{
		std::string keys;
		if (ReadString(ini, section, setting, keys)) {
			size_t pos = 0;
			std::string token;
			while ((pos = keys.find('+')) != std::string::npos) {
				ans.load_key(keys.substr(0, pos));
				keys.erase(0, pos + 1);
			}
			ans.load_key(keys);
		}
	}

	static bool ReadBool(const CSimpleIniA& ini, const char* section, const char* setting, bool& ans)
	{
		if (ini.GetValue(section, setting)) {
			ans = ini.GetBoolValue(section, setting);
			return true;
		}
		return false;
	}

	static bool ReadFloat(const CSimpleIniA& ini, const char* section, const char* setting, float& ans)
	{
		if (ini.GetValue(section, setting)) {
			ans = static_cast<float>(ini.GetDoubleValue(section, setting));
			return true;
		}
		return false;
	}

	static bool ReadUint32(const CSimpleIniA& ini, const char* section, const char* setting, uint32_t& ans)
	{
		if (ini.GetValue(section, setting)) {
			ans = static_cast<uint32_t>(ini.GetLongValue(section, setting));
			return true;
		}
		return false;
	}

	static bool ReadString(const CSimpleIniA& ini, const char* section, const char* setting, std::string& ans)
	{
		if (ini.GetValue(section, setting)) {
			auto tmp = ini.GetValue(section, setting);
			ans = std::string(tmp);
			return true;
		}
		return false;
	}
};
