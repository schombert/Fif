#pragma once

//
// This file was automatically generated from: dcon_stuff.txt
// EDIT AT YOUR OWN RISK; all changes will be lost upon regeneration
// NOT SUITABLE FOR USE IN CRITICAL SOFTWARE WHERE LIVES OR LIVELIHOODS DEPEND ON THE CORRECT OPERATION
//

#include <cstdint>
#include <cstddef>
#include <utility>
#include <vector>
#include <algorithm>
#include <array>
#include <memory>
#include <assert.h>
#include <cstring>
#include "common_types.hpp"
#ifndef DCON_NO_VE
#include "ve.hpp"
#endif

#ifdef NDEBUG
#ifdef _MSC_VER
#define DCON_RELEASE_INLINE __forceinline
#else
#define DCON_RELEASE_INLINE inline __attribute__((always_inline))
#endif
#else
#define DCON_RELEASE_INLINE inline
#endif
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4324 )
#endif

namespace fif { std::string container_interface(); }

namespace dcon {
	struct load_record {
		bool thingy : 1;
		bool thingy_some_value : 1;
		bool thingy_bf_value : 1;
		bool thingy_obj_value : 1;
		bool thingy_pooled_v : 1;
		bool thingy_big_array : 1;
		bool thingy_big_array_bf : 1;
		bool car : 1;
		bool car_wheels : 1;
		bool car_resale_value : 1;
		bool person : 1;
		bool person__index : 1;
		bool person_age : 1;
		bool car_ownership : 1;
		bool car_ownership_owner : 1;
		bool car_ownership_owned_car : 1;
		bool car_ownership_ownership_date : 1;
		load_record() {
			thingy = false;
			thingy_some_value = false;
			thingy_bf_value = false;
			thingy_obj_value = false;
			thingy_pooled_v = false;
			thingy_big_array = false;
			thingy_big_array_bf = false;
			car = false;
			car_wheels = false;
			car_resale_value = false;
			person = false;
			person__index = false;
			person_age = false;
			car_ownership = false;
			car_ownership_owner = false;
			car_ownership_owned_car = false;
			car_ownership_ownership_date = false;
		}
	};
	//
	// definition of strongly typed index for thingy_id
	//
	class thingy_id {
		public:
		using value_base_t = uint16_t;
		using zero_is_null_t = std::true_type;
		
		uint16_t value = 0;
		
		constexpr thingy_id() noexcept = default;
		explicit constexpr thingy_id(uint16_t v) noexcept : value(v + 1) {}
		constexpr thingy_id(thingy_id const& v) noexcept = default;
		constexpr thingy_id(thingy_id&& v) noexcept = default;
		
		thingy_id& operator=(thingy_id const& v) noexcept = default;
		thingy_id& operator=(thingy_id&& v) noexcept = default;
		constexpr bool operator==(thingy_id v) const noexcept { return value == v.value; }
		constexpr bool operator!=(thingy_id v) const noexcept { return value != v.value; }
		explicit constexpr operator bool() const noexcept { return value != uint16_t(0); }
		constexpr DCON_RELEASE_INLINE int32_t index() const noexcept {
			return int32_t(value) - 1;
		}
	};
	
	class thingy_id_pair {
		public:
		thingy_id left;
		thingy_id right;
	};
	
	DCON_RELEASE_INLINE bool is_valid_index(thingy_id id) { return bool(id); }
	
	//
	// definition of strongly typed index for car_id
	//
	class car_id {
		public:
		using value_base_t = uint16_t;
		using zero_is_null_t = std::true_type;
		
		uint16_t value = 0;
		
		constexpr car_id() noexcept = default;
		explicit constexpr car_id(uint16_t v) noexcept : value(v + 1) {}
		constexpr car_id(car_id const& v) noexcept = default;
		constexpr car_id(car_id&& v) noexcept = default;
		
		car_id& operator=(car_id const& v) noexcept = default;
		car_id& operator=(car_id&& v) noexcept = default;
		constexpr bool operator==(car_id v) const noexcept { return value == v.value; }
		constexpr bool operator!=(car_id v) const noexcept { return value != v.value; }
		explicit constexpr operator bool() const noexcept { return value != uint16_t(0); }
		constexpr DCON_RELEASE_INLINE int32_t index() const noexcept {
			return int32_t(value) - 1;
		}
	};
	
	class car_id_pair {
		public:
		car_id left;
		car_id right;
	};
	
	DCON_RELEASE_INLINE bool is_valid_index(car_id id) { return bool(id); }
	
	//
	// definition of strongly typed index for person_id
	//
	class person_id {
		public:
		using value_base_t = uint8_t;
		using zero_is_null_t = std::true_type;
		
		uint8_t value = 0;
		
		constexpr person_id() noexcept = default;
		explicit constexpr person_id(uint8_t v) noexcept : value(v + 1) {}
		constexpr person_id(person_id const& v) noexcept = default;
		constexpr person_id(person_id&& v) noexcept = default;
		
		person_id& operator=(person_id const& v) noexcept = default;
		person_id& operator=(person_id&& v) noexcept = default;
		constexpr bool operator==(person_id v) const noexcept { return value == v.value; }
		constexpr bool operator!=(person_id v) const noexcept { return value != v.value; }
		explicit constexpr operator bool() const noexcept { return value != uint8_t(0); }
		constexpr DCON_RELEASE_INLINE int32_t index() const noexcept {
			return int32_t(value) - 1;
		}
	};
	
	class person_id_pair {
		public:
		person_id left;
		person_id right;
	};
	
	DCON_RELEASE_INLINE bool is_valid_index(person_id id) { return bool(id); }
	
	//
	// definition of strongly typed index for car_ownership_id
	//
	class car_ownership_id {
		public:
		using value_base_t = uint16_t;
		using zero_is_null_t = std::true_type;
		
		uint16_t value = 0;
		
		constexpr car_ownership_id() noexcept = default;
		explicit constexpr car_ownership_id(uint16_t v) noexcept : value(v + 1) {}
		constexpr car_ownership_id(car_ownership_id const& v) noexcept = default;
		constexpr car_ownership_id(car_ownership_id&& v) noexcept = default;
		
		car_ownership_id& operator=(car_ownership_id const& v) noexcept = default;
		car_ownership_id& operator=(car_ownership_id&& v) noexcept = default;
		constexpr bool operator==(car_ownership_id v) const noexcept { return value == v.value; }
		constexpr bool operator!=(car_ownership_id v) const noexcept { return value != v.value; }
		explicit constexpr operator bool() const noexcept { return value != uint16_t(0); }
		constexpr DCON_RELEASE_INLINE int32_t index() const noexcept {
			return int32_t(value) - 1;
		}
	};
	
	class car_ownership_id_pair {
		public:
		car_ownership_id left;
		car_ownership_id right;
	};
	
	DCON_RELEASE_INLINE bool is_valid_index(car_ownership_id id) { return bool(id); }
	
}

#ifndef DCON_NO_VE
namespace ve {
	template<>
	struct value_to_vector_type_s<dcon::thingy_id> {
		using type = ::ve::tagged_vector<dcon::thingy_id>;
	};
	
	template<>
	struct value_to_vector_type_s<dcon::car_id> {
		using type = ::ve::tagged_vector<dcon::car_id>;
	};
	
	template<>
	struct value_to_vector_type_s<dcon::person_id> {
		using type = ::ve::tagged_vector<dcon::person_id>;
	};
	
	template<>
	struct value_to_vector_type_s<dcon::car_ownership_id> {
		using type = ::ve::tagged_vector<dcon::car_ownership_id>;
	};
	
}

#endif
namespace dcon {
	namespace detail {
	}

	class data_container;

	namespace internal {
		class const_object_iterator_thingy;
		class object_iterator_thingy;

		class alignas(64) thingy_class {
			friend const_object_iterator_thingy;
			friend object_iterator_thingy;
			friend std::string fif::container_interface();
			private:
			//
			// storage space for some_value of type int32_t
			//
			struct alignas(64) dtype_some_value {
				uint8_t padding[(63 + sizeof(int32_t)) & ~uint64_t(63)];
				int32_t values[(sizeof(int32_t) <= 64 ? (uint32_t(1200) + (uint32_t(64) / uint32_t(sizeof(int32_t))) - uint32_t(1)) & ~(uint32_t(64) / uint32_t(sizeof(int32_t)) - uint32_t(1)) : uint32_t(1200))];
				DCON_RELEASE_INLINE auto vptr() const { return values; }
				DCON_RELEASE_INLINE auto vptr() { return values; }
				dtype_some_value() { std::uninitialized_value_construct_n(values - 1, 1 + (sizeof(int32_t) <= 64 ? (uint32_t(1200) + (uint32_t(64) / uint32_t(sizeof(int32_t))) - uint32_t(1)) & ~(uint32_t(64) / uint32_t(sizeof(int32_t)) - uint32_t(1)) : uint32_t(1200))); }
			}
			m_some_value;
			
			//
			// storage space for bf_value of type dcon::bitfield_type
			//
			struct alignas(64) dtype_bf_value {
				uint8_t padding[(63 + sizeof(dcon::bitfield_type)) & ~uint64_t(63)];
				dcon::bitfield_type values[((uint32_t(1200 + 7)) / uint32_t(8) + uint32_t(63)) & ~uint32_t(63)];
				DCON_RELEASE_INLINE auto vptr() const { return values; }
				DCON_RELEASE_INLINE auto vptr() { return values; }
				dtype_bf_value() { std::uninitialized_value_construct_n(values - 1, 1 + ((uint32_t(1200 + 7)) / uint32_t(8) + uint32_t(63)) & ~uint32_t(63)); }
			}
			m_bf_value;
			
			//
			// storage space for obj_value of type std::vector<float>
			//
			struct dtype_obj_value {
				std::vector<float> values[1200];
				DCON_RELEASE_INLINE auto vptr() const { return values; }
				DCON_RELEASE_INLINE auto vptr() { return values; }
				dtype_obj_value() { std::uninitialized_value_construct_n(values, 1200); }
			}
			m_obj_value;
			
			//
			// storage space for pooled_v of type dcon::stable_mk_2_tag
			//
			struct alignas(64) dtype_pooled_v {
				uint8_t padding[(63 + sizeof(dcon::stable_mk_2_tag)) & ~uint64_t(63)];
				dcon::stable_mk_2_tag values[1200];
				DCON_RELEASE_INLINE auto vptr() const { return values; }
				DCON_RELEASE_INLINE auto vptr() { return values; }
				dtype_pooled_v() { std::uninitialized_fill_n(values - 1, 1 + 1200, std::numeric_limits<dcon::stable_mk_2_tag>::max()); }
			}
			m_pooled_v;
			
			dcon::stable_variable_vector_storage_mk_2<int16_t, 16, 1000 > pooled_v_storage;
			//
			// storage space for big_array of type array of float
			//
			struct dtype_big_array {
				std::byte* values = nullptr;
				uint32_t size = 0;
				DCON_RELEASE_INLINE auto vptr(int32_t i) const {
					return reinterpret_cast<float const*>(values  + sizeof(float) + 64 - (sizeof(float) & 63)+ (i + 1) * (sizeof(float) + 64 - (sizeof(float) & 63) + sizeof(float) * 1200 + 64 - ((1200 * sizeof(float)) & 63)));
				}
				DCON_RELEASE_INLINE auto vptr(int32_t i) {
					return reinterpret_cast<float*>(values + sizeof(float) + 64 - (sizeof(float) & 63) + (i + 1) * (sizeof(float) + 64 - (sizeof(float) & 63) + sizeof(float) * 1200 + 64 - ((1200 * sizeof(float)) & 63)));
				}
				DCON_RELEASE_INLINE void resize(uint32_t sz, uint32_t) {
					std::byte* temp = sz > 0 ? (std::byte*)(::operator new((sz + 1) * (sizeof(float) + 64 - (sizeof(float) & 63) + sizeof(float) * 1200 + 64 - ((1200 * sizeof(float)) & 63)), std::align_val_t{ 64 })) : nullptr;
					if(sz > size) {
						if(values) {
							std::memcpy(temp, values, (size + 1) * (sizeof(float) + 64 - (sizeof(float) & 63) + sizeof(float) * 1200 + 64 - ((1200 * sizeof(float)) & 63)));
							std::memset(temp + (size + 1) * (sizeof(float) + 64 - (sizeof(float) & 63) + sizeof(float) * 1200 + 64 - ((1200 * sizeof(float)) & 63)), 0, (sz - size) * (sizeof(float) + 64 - (sizeof(float) & 63) + sizeof(float) * 1200 + 64 - ((1200 * sizeof(float)) & 63)));
						} else {
							std::memset(temp, 0, (sz + 1) * (sizeof(float) + 64 - (sizeof(float) & 63) + sizeof(float) * 1200 + 64 - ((1200 * sizeof(float)) & 63)));
						}
					} else if(sz > 0) {
						std::memcpy(temp, values, (sz + 1) * (sizeof(float) + 64 - (sizeof(float) & 63) + sizeof(float) * 1200 + 64 - ((1200 * sizeof(float)) & 63)));
					}
					::operator delete(values, std::align_val_t{ 64 });
					values = temp;
					size = sz;
				}
				~dtype_big_array() { ::operator delete(values, std::align_val_t{ 64 }); }
				DCON_RELEASE_INLINE void copy_value(int32_t dest, int32_t source) {
					for(int32_t bi = 0; bi < int32_t(size); ++bi) {
						vptr(bi)[dest] = vptr(bi)[source];
					}
				}
				DCON_RELEASE_INLINE void zero_at(int32_t dest) {
					for(int32_t ci = 0; ci < int32_t(size); ++ci) {
						vptr(ci)[dest] = float{};
					}
				}
			}
			m_big_array;
			
			//
			// storage space for big_array_bf of type array of dcon::bitfield_type
			//
			struct dtype_big_array_bf {
				std::byte* values = nullptr;
				uint32_t size = 0;
				DCON_RELEASE_INLINE auto vptr(int32_t i) const {
					return reinterpret_cast<dcon::bitfield_type const*>(values  + 64+ (i + 1) * (64 + (1200 + 7) / 8 + 64 - (( (1200 + 7) / 8) & 63)));
				}
				DCON_RELEASE_INLINE auto vptr(int32_t i) {
					return reinterpret_cast<dcon::bitfield_type*>(values + 64 + (i + 1) * (64 + (1200 + 7) / 8 + 64 - (( (1200 + 7) / 8) & 63)));
				}
				DCON_RELEASE_INLINE void resize(uint32_t sz, uint32_t) {
					std::byte* temp = sz > 0 ? (std::byte*)(::operator new((sz + 1) * (64 + (1200 + 7) / 8 + 64 - (( (1200 + 7) / 8) & 63)), std::align_val_t{ 64 })) : nullptr;
					if(sz > size) {
						if(values) {
							std::memcpy(temp, values, (size + 1) * (64 + (1200 + 7) / 8 + 64 - (( (1200 + 7) / 8) & 63)));
							std::memset(temp + (size + 1) * (64 + (1200 + 7) / 8 + 64 - (( (1200 + 7) / 8) & 63)), 0, (sz - size) * (64 + (1200 + 7) / 8 + 64 - (( (1200 + 7) / 8) & 63)));
						} else {
							std::memset(temp, 0, (sz + 1) * (64 + (1200 + 7) / 8 + 64 - (( (1200 + 7) / 8) & 63)));
						}
					} else if(sz > 0) {
						std::memcpy(temp, values, (sz + 1) * (64 + (1200 + 7) / 8 + 64 - (( (1200 + 7) / 8) & 63)));
					}
					::operator delete(values, std::align_val_t{ 64 });
					values = temp;
					size = sz;
				}
				~dtype_big_array_bf() { ::operator delete(values, std::align_val_t{ 64 }); }
				DCON_RELEASE_INLINE void copy_value(int32_t dest, int32_t source) {
					for(int32_t bi = 0; bi < int32_t(size); ++bi) {
						dcon::bit_vector_set(vptr(bi), dest, dcon::bit_vector_test(vptr(bi), source));
					}
				}
				DCON_RELEASE_INLINE void zero_at(int32_t dest) {
					for(int32_t ci = 0; ci < int32_t(size); ++ci) {
						dcon::bit_vector_set(vptr(ci), dest, false);
					}
				}
			}
			m_big_array_bf;
			
			uint32_t size_used = 0;


			public:
			thingy_class() {
			}
			friend data_container;
		};

		class const_object_iterator_car;
		class object_iterator_car;

		class alignas(64) car_class {
			friend const_object_iterator_car;
			friend object_iterator_car;
			friend std::string fif::container_interface();
			private:
			//
			// storage space for wheels of type int32_t
			//
			struct alignas(64) dtype_wheels {
				uint8_t padding[(63 + sizeof(int32_t)) & ~uint64_t(63)];
				int32_t values[(sizeof(int32_t) <= 64 ? (uint32_t(1200) + (uint32_t(64) / uint32_t(sizeof(int32_t))) - uint32_t(1)) & ~(uint32_t(64) / uint32_t(sizeof(int32_t)) - uint32_t(1)) : uint32_t(1200))];
				DCON_RELEASE_INLINE auto vptr() const { return values; }
				DCON_RELEASE_INLINE auto vptr() { return values; }
				dtype_wheels() { std::uninitialized_value_construct_n(values - 1, 1 + (sizeof(int32_t) <= 64 ? (uint32_t(1200) + (uint32_t(64) / uint32_t(sizeof(int32_t))) - uint32_t(1)) & ~(uint32_t(64) / uint32_t(sizeof(int32_t)) - uint32_t(1)) : uint32_t(1200))); }
			}
			m_wheels;
			
			//
			// storage space for resale_value of type float
			//
			struct alignas(64) dtype_resale_value {
				uint8_t padding[(63 + sizeof(float)) & ~uint64_t(63)];
				float values[(sizeof(float) <= 64 ? (uint32_t(1200) + (uint32_t(64) / uint32_t(sizeof(float))) - uint32_t(1)) & ~(uint32_t(64) / uint32_t(sizeof(float)) - uint32_t(1)) : uint32_t(1200))];
				DCON_RELEASE_INLINE auto vptr() const { return values; }
				DCON_RELEASE_INLINE auto vptr() { return values; }
				dtype_resale_value() { std::uninitialized_value_construct_n(values - 1, 1 + (sizeof(float) <= 64 ? (uint32_t(1200) + (uint32_t(64) / uint32_t(sizeof(float))) - uint32_t(1)) & ~(uint32_t(64) / uint32_t(sizeof(float)) - uint32_t(1)) : uint32_t(1200))); }
			}
			m_resale_value;
			
			uint32_t size_used = 0;


			public:
			car_class() {
			}
			friend data_container;
		};

		class const_object_iterator_person;
		class object_iterator_person;

		class alignas(64) person_class {
			friend const_object_iterator_person;
			friend object_iterator_person;
			friend std::string fif::container_interface();
			private:
			//
			// storage space for _index of type person_id
			//
			struct dtype__index {
				person_id values[100];
				DCON_RELEASE_INLINE auto vptr() const { return values; }
				DCON_RELEASE_INLINE auto vptr() { return values; }
				dtype__index() { std::uninitialized_value_construct_n(values, 100); }
			}
			m__index;
			
			//
			// storage space for age of type int32_t
			//
			struct alignas(64) dtype_age {
				uint8_t padding[(63 + sizeof(int32_t)) & ~uint64_t(63)];
				int32_t values[(sizeof(int32_t) <= 64 ? (uint32_t(100) + (uint32_t(64) / uint32_t(sizeof(int32_t))) - uint32_t(1)) & ~(uint32_t(64) / uint32_t(sizeof(int32_t)) - uint32_t(1)) : uint32_t(100))];
				DCON_RELEASE_INLINE auto vptr() const { return values; }
				DCON_RELEASE_INLINE auto vptr() { return values; }
				dtype_age() { std::uninitialized_value_construct_n(values - 1, 1 + (sizeof(int32_t) <= 64 ? (uint32_t(100) + (uint32_t(64) / uint32_t(sizeof(int32_t))) - uint32_t(1)) & ~(uint32_t(64) / uint32_t(sizeof(int32_t)) - uint32_t(1)) : uint32_t(100))); }
			}
			m_age;
			
			person_id first_free = person_id();
			uint32_t size_used = 0;


			public:
			person_class() {
				for(int32_t i = 100 - 1; i >= 0; --i) {
					m__index.vptr()[i] = first_free;
					first_free = person_id(uint8_t(i));
				}
			}
			friend data_container;
		};

		class const_object_iterator_car_ownership;
		class object_iterator_car_ownership;
		class const_iterator_person_foreach_car_ownership_as_owner;
		class iterator_person_foreach_car_ownership_as_owner;
		struct const_iterator_person_foreach_car_ownership_as_owner_generator;
		struct iterator_person_foreach_car_ownership_as_owner_generator;

		class alignas(64) car_ownership_class {
			friend const_object_iterator_car_ownership;
			friend object_iterator_car_ownership;
			friend std::string fif::container_interface();
			friend const_iterator_person_foreach_car_ownership_as_owner;
			friend iterator_person_foreach_car_ownership_as_owner;
			private:
			//
			// storage space for ownership_date of type int32_t
			//
			struct alignas(64) dtype_ownership_date {
				uint8_t padding[(63 + sizeof(int32_t)) & ~uint64_t(63)];
				int32_t values[(sizeof(int32_t) <= 64 ? (uint32_t(1200) + (uint32_t(64) / uint32_t(sizeof(int32_t))) - uint32_t(1)) & ~(uint32_t(64) / uint32_t(sizeof(int32_t)) - uint32_t(1)) : uint32_t(1200))];
				DCON_RELEASE_INLINE auto vptr() const { return values; }
				DCON_RELEASE_INLINE auto vptr() { return values; }
				dtype_ownership_date() { std::uninitialized_value_construct_n(values - 1, 1 + (sizeof(int32_t) <= 64 ? (uint32_t(1200) + (uint32_t(64) / uint32_t(sizeof(int32_t))) - uint32_t(1)) & ~(uint32_t(64) / uint32_t(sizeof(int32_t)) - uint32_t(1)) : uint32_t(1200))); }
			}
			m_ownership_date;
			
			//
			// storage space for owner of type person_id
			//
			struct alignas(64) dtype_owner {
				uint8_t padding[(63 + sizeof(person_id)) & ~uint64_t(63)];
				person_id values[(sizeof(person_id) <= 64 ? (uint32_t(1200) + (uint32_t(64) / uint32_t(sizeof(person_id))) - uint32_t(1)) & ~(uint32_t(64) / uint32_t(sizeof(person_id)) - uint32_t(1)) : uint32_t(1200))];
				DCON_RELEASE_INLINE auto vptr() const { return values; }
				DCON_RELEASE_INLINE auto vptr() { return values; }
				dtype_owner() { std::uninitialized_value_construct_n(values - 1, 1 + (sizeof(person_id) <= 64 ? (uint32_t(1200) + (uint32_t(64) / uint32_t(sizeof(person_id))) - uint32_t(1)) & ~(uint32_t(64) / uint32_t(sizeof(person_id)) - uint32_t(1)) : uint32_t(1200))); }
			}
			m_owner;
			
			//
			// storage space for array_owner of type dcon::stable_mk_2_tag
			//
			struct alignas(64) dtype_array_owner {
				uint8_t padding[(63 + sizeof(dcon::stable_mk_2_tag)) & ~uint64_t(63)];
				dcon::stable_mk_2_tag values[100];
				DCON_RELEASE_INLINE auto vptr() const { return values; }
				DCON_RELEASE_INLINE auto vptr() { return values; }
				dtype_array_owner() { std::uninitialized_fill_n(values - 1, 1 + 100, std::numeric_limits<dcon::stable_mk_2_tag>::max()); }
			}
			m_array_owner;
			
			dcon::stable_variable_vector_storage_mk_2<car_ownership_id, 4, 9600 > owner_storage;

			public:
			car_ownership_class() {
			}
			friend data_container;
		};

	}

	class thingy_const_fat_id;
	class thingy_fat_id;
	class car_const_fat_id;
	class car_fat_id;
	class person_const_fat_id;
	class person_fat_id;
	class car_ownership_const_fat_id;
	class car_ownership_fat_id;
	class thingy_fat_id {
		friend data_container;
		public:
		data_container& container;
		thingy_id id;
		thingy_fat_id(data_container& c, thingy_id i) noexcept : container(c), id(i) {}
		thingy_fat_id(thingy_fat_id const& o) noexcept : container(o.container), id(o.id) {}
		DCON_RELEASE_INLINE operator thingy_id() const noexcept { return id; }
		DCON_RELEASE_INLINE thingy_fat_id& operator=(thingy_fat_id const& other) noexcept {
			assert(&container == &other.container);
			id = other.id;
			return *this;
		}
		DCON_RELEASE_INLINE thingy_fat_id& operator=(thingy_id other) noexcept {
			id = other;
			return *this;
		}
		DCON_RELEASE_INLINE bool operator==(thingy_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id == other.id;
		}
		DCON_RELEASE_INLINE bool operator==(thingy_id other) const noexcept {
			return id == other;
		}
		DCON_RELEASE_INLINE bool operator!=(thingy_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id != other.id;
		}
		DCON_RELEASE_INLINE bool operator!=(thingy_id other) const noexcept {
			return id != other;
		}
		explicit operator bool() const noexcept { return bool(id); }
		DCON_RELEASE_INLINE int32_t& get_some_value() const noexcept;
		DCON_RELEASE_INLINE void set_some_value(int32_t v) const noexcept;
		DCON_RELEASE_INLINE bool get_bf_value() const noexcept;
		DCON_RELEASE_INLINE void set_bf_value(bool v) const noexcept;
		DCON_RELEASE_INLINE std::vector<float>& get_obj_value() const noexcept;
		DCON_RELEASE_INLINE void set_obj_value(std::vector<float> const& v) const noexcept;
		DCON_RELEASE_INLINE dcon::dcon_vv_fat_id<int16_t> get_pooled_v() const noexcept;
		DCON_RELEASE_INLINE float& get_big_array(int32_t i) const noexcept;
		DCON_RELEASE_INLINE uint32_t get_big_array_size() const noexcept;
		DCON_RELEASE_INLINE void set_big_array(int32_t i, float v) const noexcept;
		DCON_RELEASE_INLINE void resize_big_array(uint32_t sz) const noexcept;
		DCON_RELEASE_INLINE bool get_big_array_bf(int32_t i) const noexcept;
		DCON_RELEASE_INLINE uint32_t get_big_array_bf_size() const noexcept;
		DCON_RELEASE_INLINE void set_big_array_bf(int32_t i, bool v) const noexcept;
		DCON_RELEASE_INLINE void resize_big_array_bf(uint32_t sz) const noexcept;
		DCON_RELEASE_INLINE bool is_valid() const noexcept;
		
	};
	DCON_RELEASE_INLINE thingy_fat_id fatten(data_container& c, thingy_id id) noexcept {
		return thingy_fat_id(c, id);
	}
	
	class thingy_const_fat_id {
		friend data_container;
		public:
		data_container const& container;
		thingy_id id;
		thingy_const_fat_id(data_container const& c, thingy_id i) noexcept : container(c), id(i) {}
		thingy_const_fat_id(thingy_const_fat_id const& o) noexcept : container(o.container), id(o.id) {}
		thingy_const_fat_id(thingy_fat_id const& o) noexcept : container(o.container), id(o.id) {}
		DCON_RELEASE_INLINE operator thingy_id() const noexcept { return id; }
		DCON_RELEASE_INLINE thingy_const_fat_id& operator=(thingy_const_fat_id const& other) noexcept {
			assert(&container == &other.container);
			id = other.id;
			return *this;
		}
		DCON_RELEASE_INLINE thingy_const_fat_id& operator=(thingy_fat_id const& other) noexcept {
			assert(&container == &other.container);
			id = other.id;
			return *this;
		}
		DCON_RELEASE_INLINE thingy_const_fat_id& operator=(thingy_id other) noexcept {
			id = other;
			return *this;
		}
		DCON_RELEASE_INLINE bool operator==(thingy_const_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id == other.id;
		}
		DCON_RELEASE_INLINE bool operator==(thingy_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id == other.id;
		}
		DCON_RELEASE_INLINE bool operator==(thingy_id other) const noexcept {
			return id == other;
		}
		DCON_RELEASE_INLINE bool operator!=(thingy_const_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id != other.id;
		}
		DCON_RELEASE_INLINE bool operator!=(thingy_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id != other.id;
		}
		DCON_RELEASE_INLINE bool operator!=(thingy_id other) const noexcept {
			return id != other;
		}
		DCON_RELEASE_INLINE explicit operator bool() const noexcept { return bool(id); }
		DCON_RELEASE_INLINE int32_t get_some_value() const noexcept;
		DCON_RELEASE_INLINE bool get_bf_value() const noexcept;
		DCON_RELEASE_INLINE std::vector<float> const& get_obj_value() const noexcept;
		DCON_RELEASE_INLINE dcon::dcon_vv_const_fat_id<int16_t> get_pooled_v() const noexcept;
		DCON_RELEASE_INLINE float get_big_array(int32_t i) const noexcept;
		DCON_RELEASE_INLINE uint32_t get_big_array_size() const noexcept;
		DCON_RELEASE_INLINE bool get_big_array_bf(int32_t i) const noexcept;
		DCON_RELEASE_INLINE uint32_t get_big_array_bf_size() const noexcept;
		DCON_RELEASE_INLINE bool is_valid() const noexcept;
		
	};
	DCON_RELEASE_INLINE bool operator==(thingy_fat_id const& l, thingy_const_fat_id const& other) noexcept {
		assert(&l.container == &other.container);
		return l.id == other.id;
	}
	DCON_RELEASE_INLINE bool operator!=(thingy_fat_id const& l, thingy_const_fat_id const& other) noexcept {
		assert(&l.container == &other.container);
		return l.id != other.id;
	}
	DCON_RELEASE_INLINE thingy_const_fat_id fatten(data_container const& c, thingy_id id) noexcept {
		return thingy_const_fat_id(c, id);
	}
	
	class car_fat_id {
		friend data_container;
		public:
		data_container& container;
		car_id id;
		car_fat_id(data_container& c, car_id i) noexcept : container(c), id(i) {}
		car_fat_id(car_fat_id const& o) noexcept : container(o.container), id(o.id) {}
		DCON_RELEASE_INLINE operator car_id() const noexcept { return id; }
		DCON_RELEASE_INLINE car_fat_id& operator=(car_fat_id const& other) noexcept {
			assert(&container == &other.container);
			id = other.id;
			return *this;
		}
		DCON_RELEASE_INLINE car_fat_id& operator=(car_id other) noexcept {
			id = other;
			return *this;
		}
		DCON_RELEASE_INLINE bool operator==(car_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id == other.id;
		}
		DCON_RELEASE_INLINE bool operator==(car_id other) const noexcept {
			return id == other;
		}
		DCON_RELEASE_INLINE bool operator!=(car_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id != other.id;
		}
		DCON_RELEASE_INLINE bool operator!=(car_id other) const noexcept {
			return id != other;
		}
		explicit operator bool() const noexcept { return bool(id); }
		DCON_RELEASE_INLINE int32_t& get_wheels() const noexcept;
		DCON_RELEASE_INLINE void set_wheels(int32_t v) const noexcept;
		DCON_RELEASE_INLINE float& get_resale_value() const noexcept;
		DCON_RELEASE_INLINE void set_resale_value(float v) const noexcept;
		DCON_RELEASE_INLINE car_ownership_fat_id get_car_ownership_as_owned_car() const noexcept;
		DCON_RELEASE_INLINE void remove_car_ownership_as_owned_car() const noexcept;
		DCON_RELEASE_INLINE car_ownership_fat_id get_car_ownership() const noexcept;
		DCON_RELEASE_INLINE void remove_car_ownership() const noexcept;
		DCON_RELEASE_INLINE person_fat_id get_owner_from_car_ownership() const noexcept;
		DCON_RELEASE_INLINE void set_owner_from_car_ownership(person_id v) const noexcept;
		DCON_RELEASE_INLINE void set_ownership_date_from_car_ownership(int32_t v) const noexcept;
		DCON_RELEASE_INLINE int32_t get_ownership_date_from_car_ownership() const noexcept;
		DCON_RELEASE_INLINE bool is_valid() const noexcept;
		
	};
	DCON_RELEASE_INLINE car_fat_id fatten(data_container& c, car_id id) noexcept {
		return car_fat_id(c, id);
	}
	
	class car_const_fat_id {
		friend data_container;
		public:
		data_container const& container;
		car_id id;
		car_const_fat_id(data_container const& c, car_id i) noexcept : container(c), id(i) {}
		car_const_fat_id(car_const_fat_id const& o) noexcept : container(o.container), id(o.id) {}
		car_const_fat_id(car_fat_id const& o) noexcept : container(o.container), id(o.id) {}
		DCON_RELEASE_INLINE operator car_id() const noexcept { return id; }
		DCON_RELEASE_INLINE car_const_fat_id& operator=(car_const_fat_id const& other) noexcept {
			assert(&container == &other.container);
			id = other.id;
			return *this;
		}
		DCON_RELEASE_INLINE car_const_fat_id& operator=(car_fat_id const& other) noexcept {
			assert(&container == &other.container);
			id = other.id;
			return *this;
		}
		DCON_RELEASE_INLINE car_const_fat_id& operator=(car_id other) noexcept {
			id = other;
			return *this;
		}
		DCON_RELEASE_INLINE bool operator==(car_const_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id == other.id;
		}
		DCON_RELEASE_INLINE bool operator==(car_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id == other.id;
		}
		DCON_RELEASE_INLINE bool operator==(car_id other) const noexcept {
			return id == other;
		}
		DCON_RELEASE_INLINE bool operator!=(car_const_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id != other.id;
		}
		DCON_RELEASE_INLINE bool operator!=(car_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id != other.id;
		}
		DCON_RELEASE_INLINE bool operator!=(car_id other) const noexcept {
			return id != other;
		}
		DCON_RELEASE_INLINE explicit operator bool() const noexcept { return bool(id); }
		DCON_RELEASE_INLINE int32_t get_wheels() const noexcept;
		DCON_RELEASE_INLINE float get_resale_value() const noexcept;
		DCON_RELEASE_INLINE car_ownership_const_fat_id get_car_ownership_as_owned_car() const noexcept;
		DCON_RELEASE_INLINE car_ownership_const_fat_id get_car_ownership() const noexcept;
		DCON_RELEASE_INLINE person_const_fat_id get_owner_from_car_ownership() const noexcept;
		DCON_RELEASE_INLINE int32_t get_ownership_date_from_car_ownership() const noexcept;
		DCON_RELEASE_INLINE bool is_valid() const noexcept;
		
	};
	DCON_RELEASE_INLINE bool operator==(car_fat_id const& l, car_const_fat_id const& other) noexcept {
		assert(&l.container == &other.container);
		return l.id == other.id;
	}
	DCON_RELEASE_INLINE bool operator!=(car_fat_id const& l, car_const_fat_id const& other) noexcept {
		assert(&l.container == &other.container);
		return l.id != other.id;
	}
	DCON_RELEASE_INLINE car_const_fat_id fatten(data_container const& c, car_id id) noexcept {
		return car_const_fat_id(c, id);
	}
	
	class person_fat_id {
		friend data_container;
		public:
		data_container& container;
		person_id id;
		person_fat_id(data_container& c, person_id i) noexcept : container(c), id(i) {}
		person_fat_id(person_fat_id const& o) noexcept : container(o.container), id(o.id) {}
		DCON_RELEASE_INLINE operator person_id() const noexcept { return id; }
		DCON_RELEASE_INLINE person_fat_id& operator=(person_fat_id const& other) noexcept {
			assert(&container == &other.container);
			id = other.id;
			return *this;
		}
		DCON_RELEASE_INLINE person_fat_id& operator=(person_id other) noexcept {
			id = other;
			return *this;
		}
		DCON_RELEASE_INLINE bool operator==(person_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id == other.id;
		}
		DCON_RELEASE_INLINE bool operator==(person_id other) const noexcept {
			return id == other;
		}
		DCON_RELEASE_INLINE bool operator!=(person_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id != other.id;
		}
		DCON_RELEASE_INLINE bool operator!=(person_id other) const noexcept {
			return id != other;
		}
		explicit operator bool() const noexcept { return bool(id); }
		DCON_RELEASE_INLINE int32_t& get_age() const noexcept;
		DCON_RELEASE_INLINE void set_age(int32_t v) const noexcept;
		template<typename T>
		DCON_RELEASE_INLINE void for_each_car_ownership_as_owner(T&& func) const;
		DCON_RELEASE_INLINE std::pair<car_ownership_id const*, car_ownership_id const*> range_of_car_ownership_as_owner() const;
		DCON_RELEASE_INLINE void remove_all_car_ownership_as_owner() const noexcept;
		DCON_RELEASE_INLINE internal::iterator_person_foreach_car_ownership_as_owner_generator get_car_ownership_as_owner() const;
		template<typename T>
		DCON_RELEASE_INLINE void for_each_car_ownership(T&& func) const;
		DCON_RELEASE_INLINE std::pair<car_ownership_id const*, car_ownership_id const*> range_of_car_ownership() const;
		DCON_RELEASE_INLINE void remove_all_car_ownership() const noexcept;
		DCON_RELEASE_INLINE internal::iterator_person_foreach_car_ownership_as_owner_generator get_car_ownership() const;
		DCON_RELEASE_INLINE bool is_valid() const noexcept;
		
	};
	DCON_RELEASE_INLINE person_fat_id fatten(data_container& c, person_id id) noexcept {
		return person_fat_id(c, id);
	}
	
	class person_const_fat_id {
		friend data_container;
		public:
		data_container const& container;
		person_id id;
		person_const_fat_id(data_container const& c, person_id i) noexcept : container(c), id(i) {}
		person_const_fat_id(person_const_fat_id const& o) noexcept : container(o.container), id(o.id) {}
		person_const_fat_id(person_fat_id const& o) noexcept : container(o.container), id(o.id) {}
		DCON_RELEASE_INLINE operator person_id() const noexcept { return id; }
		DCON_RELEASE_INLINE person_const_fat_id& operator=(person_const_fat_id const& other) noexcept {
			assert(&container == &other.container);
			id = other.id;
			return *this;
		}
		DCON_RELEASE_INLINE person_const_fat_id& operator=(person_fat_id const& other) noexcept {
			assert(&container == &other.container);
			id = other.id;
			return *this;
		}
		DCON_RELEASE_INLINE person_const_fat_id& operator=(person_id other) noexcept {
			id = other;
			return *this;
		}
		DCON_RELEASE_INLINE bool operator==(person_const_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id == other.id;
		}
		DCON_RELEASE_INLINE bool operator==(person_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id == other.id;
		}
		DCON_RELEASE_INLINE bool operator==(person_id other) const noexcept {
			return id == other;
		}
		DCON_RELEASE_INLINE bool operator!=(person_const_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id != other.id;
		}
		DCON_RELEASE_INLINE bool operator!=(person_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id != other.id;
		}
		DCON_RELEASE_INLINE bool operator!=(person_id other) const noexcept {
			return id != other;
		}
		DCON_RELEASE_INLINE explicit operator bool() const noexcept { return bool(id); }
		DCON_RELEASE_INLINE int32_t get_age() const noexcept;
		template<typename T>
		DCON_RELEASE_INLINE void for_each_car_ownership_as_owner(T&& func) const;
		DCON_RELEASE_INLINE std::pair<car_ownership_id const*, car_ownership_id const*> range_of_car_ownership_as_owner() const;
		DCON_RELEASE_INLINE internal::const_iterator_person_foreach_car_ownership_as_owner_generator get_car_ownership_as_owner() const;
		template<typename T>
		DCON_RELEASE_INLINE void for_each_car_ownership(T&& func) const;
		DCON_RELEASE_INLINE std::pair<car_ownership_id const*, car_ownership_id const*> range_of_car_ownership() const;
		DCON_RELEASE_INLINE internal::const_iterator_person_foreach_car_ownership_as_owner_generator get_car_ownership() const;
		DCON_RELEASE_INLINE bool is_valid() const noexcept;
		
	};
	DCON_RELEASE_INLINE bool operator==(person_fat_id const& l, person_const_fat_id const& other) noexcept {
		assert(&l.container == &other.container);
		return l.id == other.id;
	}
	DCON_RELEASE_INLINE bool operator!=(person_fat_id const& l, person_const_fat_id const& other) noexcept {
		assert(&l.container == &other.container);
		return l.id != other.id;
	}
	DCON_RELEASE_INLINE person_const_fat_id fatten(data_container const& c, person_id id) noexcept {
		return person_const_fat_id(c, id);
	}
	
	class car_ownership_fat_id {
		friend data_container;
		public:
		data_container& container;
		car_ownership_id id;
		car_ownership_fat_id(data_container& c, car_ownership_id i) noexcept : container(c), id(i) {}
		car_ownership_fat_id(car_ownership_fat_id const& o) noexcept : container(o.container), id(o.id) {}
		DCON_RELEASE_INLINE operator car_ownership_id() const noexcept { return id; }
		DCON_RELEASE_INLINE car_ownership_fat_id& operator=(car_ownership_fat_id const& other) noexcept {
			assert(&container == &other.container);
			id = other.id;
			return *this;
		}
		DCON_RELEASE_INLINE car_ownership_fat_id& operator=(car_ownership_id other) noexcept {
			id = other;
			return *this;
		}
		DCON_RELEASE_INLINE bool operator==(car_ownership_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id == other.id;
		}
		DCON_RELEASE_INLINE bool operator==(car_ownership_id other) const noexcept {
			return id == other;
		}
		DCON_RELEASE_INLINE bool operator!=(car_ownership_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id != other.id;
		}
		DCON_RELEASE_INLINE bool operator!=(car_ownership_id other) const noexcept {
			return id != other;
		}
		explicit operator bool() const noexcept { return bool(id); }
		DCON_RELEASE_INLINE int32_t& get_ownership_date() const noexcept;
		DCON_RELEASE_INLINE void set_ownership_date(int32_t v) const noexcept;
		DCON_RELEASE_INLINE person_fat_id get_owner() const noexcept;
		DCON_RELEASE_INLINE void set_owner(person_id val) const noexcept;
		DCON_RELEASE_INLINE bool try_set_owner(person_id val) const noexcept;
		DCON_RELEASE_INLINE car_fat_id get_owned_car() const noexcept;
		DCON_RELEASE_INLINE void set_owned_car(car_id val) const noexcept;
		DCON_RELEASE_INLINE bool try_set_owned_car(car_id val) const noexcept;
		DCON_RELEASE_INLINE bool is_valid() const noexcept;
		
	};
	DCON_RELEASE_INLINE car_ownership_fat_id fatten(data_container& c, car_ownership_id id) noexcept {
		return car_ownership_fat_id(c, id);
	}
	
	class car_ownership_const_fat_id {
		friend data_container;
		public:
		data_container const& container;
		car_ownership_id id;
		car_ownership_const_fat_id(data_container const& c, car_ownership_id i) noexcept : container(c), id(i) {}
		car_ownership_const_fat_id(car_ownership_const_fat_id const& o) noexcept : container(o.container), id(o.id) {}
		car_ownership_const_fat_id(car_ownership_fat_id const& o) noexcept : container(o.container), id(o.id) {}
		DCON_RELEASE_INLINE operator car_ownership_id() const noexcept { return id; }
		DCON_RELEASE_INLINE car_ownership_const_fat_id& operator=(car_ownership_const_fat_id const& other) noexcept {
			assert(&container == &other.container);
			id = other.id;
			return *this;
		}
		DCON_RELEASE_INLINE car_ownership_const_fat_id& operator=(car_ownership_fat_id const& other) noexcept {
			assert(&container == &other.container);
			id = other.id;
			return *this;
		}
		DCON_RELEASE_INLINE car_ownership_const_fat_id& operator=(car_ownership_id other) noexcept {
			id = other;
			return *this;
		}
		DCON_RELEASE_INLINE bool operator==(car_ownership_const_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id == other.id;
		}
		DCON_RELEASE_INLINE bool operator==(car_ownership_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id == other.id;
		}
		DCON_RELEASE_INLINE bool operator==(car_ownership_id other) const noexcept {
			return id == other;
		}
		DCON_RELEASE_INLINE bool operator!=(car_ownership_const_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id != other.id;
		}
		DCON_RELEASE_INLINE bool operator!=(car_ownership_fat_id const& other) const noexcept {
			assert(&container == &other.container);
			return id != other.id;
		}
		DCON_RELEASE_INLINE bool operator!=(car_ownership_id other) const noexcept {
			return id != other;
		}
		DCON_RELEASE_INLINE explicit operator bool() const noexcept { return bool(id); }
		DCON_RELEASE_INLINE int32_t get_ownership_date() const noexcept;
		DCON_RELEASE_INLINE person_const_fat_id get_owner() const noexcept;
		DCON_RELEASE_INLINE car_const_fat_id get_owned_car() const noexcept;
		DCON_RELEASE_INLINE bool is_valid() const noexcept;
		
	};
	DCON_RELEASE_INLINE bool operator==(car_ownership_fat_id const& l, car_ownership_const_fat_id const& other) noexcept {
		assert(&l.container == &other.container);
		return l.id == other.id;
	}
	DCON_RELEASE_INLINE bool operator!=(car_ownership_fat_id const& l, car_ownership_const_fat_id const& other) noexcept {
		assert(&l.container == &other.container);
		return l.id != other.id;
	}
	DCON_RELEASE_INLINE car_ownership_const_fat_id fatten(data_container const& c, car_ownership_id id) noexcept {
		return car_ownership_const_fat_id(c, id);
	}
	
	namespace internal {
		class object_iterator_thingy {
			private:
			data_container& container;
			uint32_t index = 0;
			public:
			object_iterator_thingy(data_container& c, uint32_t i) noexcept;
			DCON_RELEASE_INLINE object_iterator_thingy& operator++() noexcept;
			DCON_RELEASE_INLINE object_iterator_thingy& operator--() noexcept;
			DCON_RELEASE_INLINE bool operator==(object_iterator_thingy const& o) const noexcept {
				return &container == &o.container && index == o.index;
			}
			DCON_RELEASE_INLINE bool operator!=(object_iterator_thingy const& o) const noexcept {
				return !(*this == o);
			}
			DCON_RELEASE_INLINE thingy_fat_id operator*() const noexcept {
				return thingy_fat_id(container, thingy_id(thingy_id::value_base_t(index)));
			}
			DCON_RELEASE_INLINE object_iterator_thingy& operator+=(int32_t n) noexcept {
				index = uint32_t(int32_t(index) + n);
				return *this;
			}
			DCON_RELEASE_INLINE object_iterator_thingy& operator-=(int32_t n) noexcept {
				index = uint32_t(int32_t(index) - n);
				return *this;
			}
			DCON_RELEASE_INLINE object_iterator_thingy operator+(int32_t n) const noexcept {
				return object_iterator_thingy(container, uint32_t(int32_t(index) + n));
			}
			DCON_RELEASE_INLINE object_iterator_thingy operator-(int32_t n) const noexcept {
				return object_iterator_thingy(container, uint32_t(int32_t(index) - n));
			}
			DCON_RELEASE_INLINE int32_t operator-(object_iterator_thingy const& o) const noexcept {
				return int32_t(index) - int32_t(o.index);
			}
			DCON_RELEASE_INLINE bool operator>(object_iterator_thingy const& o) const noexcept {
				return index > o.index;
			}
			DCON_RELEASE_INLINE bool operator>=(object_iterator_thingy const& o) const noexcept {
				return index >= o.index;
			}
			DCON_RELEASE_INLINE bool operator<(object_iterator_thingy const& o) const noexcept {
				return index < o.index;
			}
			DCON_RELEASE_INLINE bool operator<=(object_iterator_thingy const& o) const noexcept {
				return index <= o.index;
			}
			DCON_RELEASE_INLINE thingy_fat_id operator[](int32_t n) const noexcept {
				return thingy_fat_id(container, thingy_id(thingy_id::value_base_t(int32_t(index) + n)));
			}
		};
		class const_object_iterator_thingy {
			private:
			data_container const& container;
			uint32_t index = 0;
			public:
			const_object_iterator_thingy(data_container const& c, uint32_t i) noexcept;
			DCON_RELEASE_INLINE const_object_iterator_thingy& operator++() noexcept;
			DCON_RELEASE_INLINE const_object_iterator_thingy& operator--() noexcept;
			DCON_RELEASE_INLINE bool operator==(const_object_iterator_thingy const& o) const noexcept {
				return &container == &o.container && index == o.index;
			}
			DCON_RELEASE_INLINE bool operator!=(const_object_iterator_thingy const& o) const noexcept {
				return !(*this == o);
			}
			DCON_RELEASE_INLINE thingy_const_fat_id operator*() const noexcept {
				return thingy_const_fat_id(container, thingy_id(thingy_id::value_base_t(index)));
			}
			DCON_RELEASE_INLINE const_object_iterator_thingy& operator+=(int32_t n) noexcept {
				index = uint32_t(int32_t(index) + n);
				return *this;
			}
			DCON_RELEASE_INLINE const_object_iterator_thingy& operator-=(int32_t n) noexcept {
				index = uint32_t(int32_t(index) - n);
				return *this;
			}
			DCON_RELEASE_INLINE const_object_iterator_thingy operator+(int32_t n) const noexcept {
				return const_object_iterator_thingy(container, uint32_t(int32_t(index) + n));
			}
			DCON_RELEASE_INLINE const_object_iterator_thingy operator-(int32_t n) const noexcept {
				return const_object_iterator_thingy(container, uint32_t(int32_t(index) - n));
			}
			DCON_RELEASE_INLINE int32_t operator-(const_object_iterator_thingy const& o) const noexcept {
				return int32_t(index) - int32_t(o.index);
			}
			DCON_RELEASE_INLINE bool operator>(const_object_iterator_thingy const& o) const noexcept {
				return index > o.index;
			}
			DCON_RELEASE_INLINE bool operator>=(const_object_iterator_thingy const& o) const noexcept {
				return index >= o.index;
			}
			DCON_RELEASE_INLINE bool operator<(const_object_iterator_thingy const& o) const noexcept {
				return index < o.index;
			}
			DCON_RELEASE_INLINE bool operator<=(const_object_iterator_thingy const& o) const noexcept {
				return index <= o.index;
			}
			DCON_RELEASE_INLINE thingy_const_fat_id operator[](int32_t n) const noexcept {
				return thingy_const_fat_id(container, thingy_id(thingy_id::value_base_t(int32_t(index) + n)));
			}
		};
		
		class object_iterator_car {
			private:
			data_container& container;
			uint32_t index = 0;
			public:
			object_iterator_car(data_container& c, uint32_t i) noexcept;
			DCON_RELEASE_INLINE object_iterator_car& operator++() noexcept;
			DCON_RELEASE_INLINE object_iterator_car& operator--() noexcept;
			DCON_RELEASE_INLINE bool operator==(object_iterator_car const& o) const noexcept {
				return &container == &o.container && index == o.index;
			}
			DCON_RELEASE_INLINE bool operator!=(object_iterator_car const& o) const noexcept {
				return !(*this == o);
			}
			DCON_RELEASE_INLINE car_fat_id operator*() const noexcept {
				return car_fat_id(container, car_id(car_id::value_base_t(index)));
			}
			DCON_RELEASE_INLINE object_iterator_car& operator+=(int32_t n) noexcept {
				index = uint32_t(int32_t(index) + n);
				return *this;
			}
			DCON_RELEASE_INLINE object_iterator_car& operator-=(int32_t n) noexcept {
				index = uint32_t(int32_t(index) - n);
				return *this;
			}
			DCON_RELEASE_INLINE object_iterator_car operator+(int32_t n) const noexcept {
				return object_iterator_car(container, uint32_t(int32_t(index) + n));
			}
			DCON_RELEASE_INLINE object_iterator_car operator-(int32_t n) const noexcept {
				return object_iterator_car(container, uint32_t(int32_t(index) - n));
			}
			DCON_RELEASE_INLINE int32_t operator-(object_iterator_car const& o) const noexcept {
				return int32_t(index) - int32_t(o.index);
			}
			DCON_RELEASE_INLINE bool operator>(object_iterator_car const& o) const noexcept {
				return index > o.index;
			}
			DCON_RELEASE_INLINE bool operator>=(object_iterator_car const& o) const noexcept {
				return index >= o.index;
			}
			DCON_RELEASE_INLINE bool operator<(object_iterator_car const& o) const noexcept {
				return index < o.index;
			}
			DCON_RELEASE_INLINE bool operator<=(object_iterator_car const& o) const noexcept {
				return index <= o.index;
			}
			DCON_RELEASE_INLINE car_fat_id operator[](int32_t n) const noexcept {
				return car_fat_id(container, car_id(car_id::value_base_t(int32_t(index) + n)));
			}
		};
		class const_object_iterator_car {
			private:
			data_container const& container;
			uint32_t index = 0;
			public:
			const_object_iterator_car(data_container const& c, uint32_t i) noexcept;
			DCON_RELEASE_INLINE const_object_iterator_car& operator++() noexcept;
			DCON_RELEASE_INLINE const_object_iterator_car& operator--() noexcept;
			DCON_RELEASE_INLINE bool operator==(const_object_iterator_car const& o) const noexcept {
				return &container == &o.container && index == o.index;
			}
			DCON_RELEASE_INLINE bool operator!=(const_object_iterator_car const& o) const noexcept {
				return !(*this == o);
			}
			DCON_RELEASE_INLINE car_const_fat_id operator*() const noexcept {
				return car_const_fat_id(container, car_id(car_id::value_base_t(index)));
			}
			DCON_RELEASE_INLINE const_object_iterator_car& operator+=(int32_t n) noexcept {
				index = uint32_t(int32_t(index) + n);
				return *this;
			}
			DCON_RELEASE_INLINE const_object_iterator_car& operator-=(int32_t n) noexcept {
				index = uint32_t(int32_t(index) - n);
				return *this;
			}
			DCON_RELEASE_INLINE const_object_iterator_car operator+(int32_t n) const noexcept {
				return const_object_iterator_car(container, uint32_t(int32_t(index) + n));
			}
			DCON_RELEASE_INLINE const_object_iterator_car operator-(int32_t n) const noexcept {
				return const_object_iterator_car(container, uint32_t(int32_t(index) - n));
			}
			DCON_RELEASE_INLINE int32_t operator-(const_object_iterator_car const& o) const noexcept {
				return int32_t(index) - int32_t(o.index);
			}
			DCON_RELEASE_INLINE bool operator>(const_object_iterator_car const& o) const noexcept {
				return index > o.index;
			}
			DCON_RELEASE_INLINE bool operator>=(const_object_iterator_car const& o) const noexcept {
				return index >= o.index;
			}
			DCON_RELEASE_INLINE bool operator<(const_object_iterator_car const& o) const noexcept {
				return index < o.index;
			}
			DCON_RELEASE_INLINE bool operator<=(const_object_iterator_car const& o) const noexcept {
				return index <= o.index;
			}
			DCON_RELEASE_INLINE car_const_fat_id operator[](int32_t n) const noexcept {
				return car_const_fat_id(container, car_id(car_id::value_base_t(int32_t(index) + n)));
			}
		};
		
		class object_iterator_person {
			private:
			data_container& container;
			uint32_t index = 0;
			public:
			object_iterator_person(data_container& c, uint32_t i) noexcept;
			DCON_RELEASE_INLINE object_iterator_person& operator++() noexcept;
			DCON_RELEASE_INLINE object_iterator_person& operator--() noexcept;
			DCON_RELEASE_INLINE bool operator==(object_iterator_person const& o) const noexcept {
				return &container == &o.container && index == o.index;
			}
			DCON_RELEASE_INLINE bool operator!=(object_iterator_person const& o) const noexcept {
				return !(*this == o);
			}
			DCON_RELEASE_INLINE person_fat_id operator*() const noexcept {
				return person_fat_id(container, person_id(person_id::value_base_t(index)));
			}
		};
		class const_object_iterator_person {
			private:
			data_container const& container;
			uint32_t index = 0;
			public:
			const_object_iterator_person(data_container const& c, uint32_t i) noexcept;
			DCON_RELEASE_INLINE const_object_iterator_person& operator++() noexcept;
			DCON_RELEASE_INLINE const_object_iterator_person& operator--() noexcept;
			DCON_RELEASE_INLINE bool operator==(const_object_iterator_person const& o) const noexcept {
				return &container == &o.container && index == o.index;
			}
			DCON_RELEASE_INLINE bool operator!=(const_object_iterator_person const& o) const noexcept {
				return !(*this == o);
			}
			DCON_RELEASE_INLINE person_const_fat_id operator*() const noexcept {
				return person_const_fat_id(container, person_id(person_id::value_base_t(index)));
			}
		};
		
		class iterator_person_foreach_car_ownership_as_owner {
			private:
			data_container& container;
			car_ownership_id const* ptr = nullptr;
			public:
			iterator_person_foreach_car_ownership_as_owner(data_container& c, person_id fr) noexcept;
			iterator_person_foreach_car_ownership_as_owner(data_container& c, car_ownership_id const* r) noexcept : container(c), ptr(r) {}
			iterator_person_foreach_car_ownership_as_owner(data_container& c, person_id fr, int) noexcept;
			DCON_RELEASE_INLINE iterator_person_foreach_car_ownership_as_owner& operator++() noexcept;
			DCON_RELEASE_INLINE iterator_person_foreach_car_ownership_as_owner& operator--() noexcept;
			DCON_RELEASE_INLINE bool operator==(iterator_person_foreach_car_ownership_as_owner const& o) const noexcept {
				return ptr == o.ptr;
			}
			DCON_RELEASE_INLINE bool operator!=(iterator_person_foreach_car_ownership_as_owner const& o) const noexcept {
				return !(*this == o);
			}
			DCON_RELEASE_INLINE car_ownership_fat_id operator*() const noexcept {
				return car_ownership_fat_id(container, *ptr);
			}
			DCON_RELEASE_INLINE iterator_person_foreach_car_ownership_as_owner& operator+=(ptrdiff_t n) noexcept {
				ptr += n;
				return *this;
			}
			DCON_RELEASE_INLINE iterator_person_foreach_car_ownership_as_owner& operator-=(ptrdiff_t n) noexcept {
				ptr -= n;
				return *this;
			}
			DCON_RELEASE_INLINE iterator_person_foreach_car_ownership_as_owner operator+(ptrdiff_t n) const noexcept {
				return iterator_person_foreach_car_ownership_as_owner(container, ptr + n);
			}
			DCON_RELEASE_INLINE iterator_person_foreach_car_ownership_as_owner operator-(ptrdiff_t n) const noexcept {
				return iterator_person_foreach_car_ownership_as_owner(container, ptr - n);
			}
			DCON_RELEASE_INLINE ptrdiff_t operator-(iterator_person_foreach_car_ownership_as_owner const& o) const noexcept {
				return ptr - o.ptr;
			}
			DCON_RELEASE_INLINE bool operator>(iterator_person_foreach_car_ownership_as_owner const& o) const noexcept {
				return ptr > o.ptr;
			}
			DCON_RELEASE_INLINE bool operator>=(iterator_person_foreach_car_ownership_as_owner const& o) const noexcept {
				return ptr >= o.ptr;
			}
			DCON_RELEASE_INLINE bool operator<(iterator_person_foreach_car_ownership_as_owner const& o) const noexcept {
				return ptr < o.ptr;
			}
			DCON_RELEASE_INLINE bool operator<=(iterator_person_foreach_car_ownership_as_owner const& o) const noexcept {
				return ptr <= o.ptr;
			}
			DCON_RELEASE_INLINE car_ownership_fat_id operator[](ptrdiff_t n) const noexcept {
				return car_ownership_fat_id(container, *(ptr + n));
			}
		};
		class const_iterator_person_foreach_car_ownership_as_owner {
			private:
			data_container const& container;
			car_ownership_id const* ptr = nullptr;
			public:
			const_iterator_person_foreach_car_ownership_as_owner(data_container const& c, person_id fr) noexcept;
			const_iterator_person_foreach_car_ownership_as_owner(data_container const& c, car_ownership_id const* r) noexcept : container(c), ptr(r) {}
			const_iterator_person_foreach_car_ownership_as_owner(data_container const& c, person_id fr, int) noexcept;
			DCON_RELEASE_INLINE const_iterator_person_foreach_car_ownership_as_owner& operator++() noexcept;
			DCON_RELEASE_INLINE const_iterator_person_foreach_car_ownership_as_owner& operator--() noexcept;
			DCON_RELEASE_INLINE bool operator==(const_iterator_person_foreach_car_ownership_as_owner const& o) const noexcept {
				return ptr == o.ptr;
			}
			DCON_RELEASE_INLINE bool operator!=(const_iterator_person_foreach_car_ownership_as_owner const& o) const noexcept {
				return !(*this == o);
			}
			DCON_RELEASE_INLINE car_ownership_const_fat_id operator*() const noexcept {
				return car_ownership_const_fat_id(container, *ptr);
			}
			DCON_RELEASE_INLINE const_iterator_person_foreach_car_ownership_as_owner& operator+=(ptrdiff_t n) noexcept {
				ptr += n;
				return *this;
			}
			DCON_RELEASE_INLINE const_iterator_person_foreach_car_ownership_as_owner& operator-=(ptrdiff_t n) noexcept {
				ptr -= n;
				return *this;
			}
			DCON_RELEASE_INLINE const_iterator_person_foreach_car_ownership_as_owner operator+(ptrdiff_t n) const noexcept {
				return const_iterator_person_foreach_car_ownership_as_owner(container, ptr + n);
			}
			DCON_RELEASE_INLINE const_iterator_person_foreach_car_ownership_as_owner operator-(ptrdiff_t n) const noexcept {
				return const_iterator_person_foreach_car_ownership_as_owner(container, ptr - n);
			}
			DCON_RELEASE_INLINE ptrdiff_t operator-(const_iterator_person_foreach_car_ownership_as_owner const& o) const noexcept {
				return ptr - o.ptr;
			}
			DCON_RELEASE_INLINE bool operator>(const_iterator_person_foreach_car_ownership_as_owner const& o) const noexcept {
				return ptr > o.ptr;
			}
			DCON_RELEASE_INLINE bool operator>=(const_iterator_person_foreach_car_ownership_as_owner const& o) const noexcept {
				return ptr >= o.ptr;
			}
			DCON_RELEASE_INLINE bool operator<(const_iterator_person_foreach_car_ownership_as_owner const& o) const noexcept {
				return ptr < o.ptr;
			}
			DCON_RELEASE_INLINE bool operator<=(const_iterator_person_foreach_car_ownership_as_owner const& o) const noexcept {
				return ptr <= o.ptr;
			}
			DCON_RELEASE_INLINE car_ownership_const_fat_id operator[](ptrdiff_t n) const noexcept {
				return car_ownership_const_fat_id(container, *(ptr + n));
			}
		};
		
		struct iterator_person_foreach_car_ownership_as_owner_generator {
			data_container& container;
			person_id ob;
			iterator_person_foreach_car_ownership_as_owner_generator(data_container& c, person_id o) : container(c), ob(o) {}
			DCON_RELEASE_INLINE iterator_person_foreach_car_ownership_as_owner begin() const noexcept {
				return iterator_person_foreach_car_ownership_as_owner(container, ob);
			}
			DCON_RELEASE_INLINE iterator_person_foreach_car_ownership_as_owner end() const noexcept {
				return iterator_person_foreach_car_ownership_as_owner(container, ob, 0);
			}
		};
		struct const_iterator_person_foreach_car_ownership_as_owner_generator {
			data_container const& container;
			person_id ob;
			const_iterator_person_foreach_car_ownership_as_owner_generator(data_container const& c, person_id o) : container(c), ob(o) {}
			DCON_RELEASE_INLINE const_iterator_person_foreach_car_ownership_as_owner begin() const noexcept {
				return const_iterator_person_foreach_car_ownership_as_owner(container, ob);
			}
			DCON_RELEASE_INLINE const_iterator_person_foreach_car_ownership_as_owner end() const noexcept {
				return const_iterator_person_foreach_car_ownership_as_owner(container, ob, 0);
			}
		};
		
		class object_iterator_car_ownership {
			private:
			data_container& container;
			uint32_t index = 0;
			public:
			object_iterator_car_ownership(data_container& c, uint32_t i) noexcept;
			DCON_RELEASE_INLINE object_iterator_car_ownership& operator++() noexcept;
			DCON_RELEASE_INLINE object_iterator_car_ownership& operator--() noexcept;
			DCON_RELEASE_INLINE bool operator==(object_iterator_car_ownership const& o) const noexcept {
				return &container == &o.container && index == o.index;
			}
			DCON_RELEASE_INLINE bool operator!=(object_iterator_car_ownership const& o) const noexcept {
				return !(*this == o);
			}
			DCON_RELEASE_INLINE car_ownership_fat_id operator*() const noexcept {
				return car_ownership_fat_id(container, car_ownership_id(car_ownership_id::value_base_t(index)));
			}
			DCON_RELEASE_INLINE object_iterator_car_ownership& operator+=(int32_t n) noexcept {
				index = uint32_t(int32_t(index) + n);
				return *this;
			}
			DCON_RELEASE_INLINE object_iterator_car_ownership& operator-=(int32_t n) noexcept {
				index = uint32_t(int32_t(index) - n);
				return *this;
			}
			DCON_RELEASE_INLINE object_iterator_car_ownership operator+(int32_t n) const noexcept {
				return object_iterator_car_ownership(container, uint32_t(int32_t(index) + n));
			}
			DCON_RELEASE_INLINE object_iterator_car_ownership operator-(int32_t n) const noexcept {
				return object_iterator_car_ownership(container, uint32_t(int32_t(index) - n));
			}
			DCON_RELEASE_INLINE int32_t operator-(object_iterator_car_ownership const& o) const noexcept {
				return int32_t(index) - int32_t(o.index);
			}
			DCON_RELEASE_INLINE bool operator>(object_iterator_car_ownership const& o) const noexcept {
				return index > o.index;
			}
			DCON_RELEASE_INLINE bool operator>=(object_iterator_car_ownership const& o) const noexcept {
				return index >= o.index;
			}
			DCON_RELEASE_INLINE bool operator<(object_iterator_car_ownership const& o) const noexcept {
				return index < o.index;
			}
			DCON_RELEASE_INLINE bool operator<=(object_iterator_car_ownership const& o) const noexcept {
				return index <= o.index;
			}
			DCON_RELEASE_INLINE car_ownership_fat_id operator[](int32_t n) const noexcept {
				return car_ownership_fat_id(container, car_ownership_id(car_ownership_id::value_base_t(int32_t(index) + n)));
			}
		};
		class const_object_iterator_car_ownership {
			private:
			data_container const& container;
			uint32_t index = 0;
			public:
			const_object_iterator_car_ownership(data_container const& c, uint32_t i) noexcept;
			DCON_RELEASE_INLINE const_object_iterator_car_ownership& operator++() noexcept;
			DCON_RELEASE_INLINE const_object_iterator_car_ownership& operator--() noexcept;
			DCON_RELEASE_INLINE bool operator==(const_object_iterator_car_ownership const& o) const noexcept {
				return &container == &o.container && index == o.index;
			}
			DCON_RELEASE_INLINE bool operator!=(const_object_iterator_car_ownership const& o) const noexcept {
				return !(*this == o);
			}
			DCON_RELEASE_INLINE car_ownership_const_fat_id operator*() const noexcept {
				return car_ownership_const_fat_id(container, car_ownership_id(car_ownership_id::value_base_t(index)));
			}
			DCON_RELEASE_INLINE const_object_iterator_car_ownership& operator+=(int32_t n) noexcept {
				index = uint32_t(int32_t(index) + n);
				return *this;
			}
			DCON_RELEASE_INLINE const_object_iterator_car_ownership& operator-=(int32_t n) noexcept {
				index = uint32_t(int32_t(index) - n);
				return *this;
			}
			DCON_RELEASE_INLINE const_object_iterator_car_ownership operator+(int32_t n) const noexcept {
				return const_object_iterator_car_ownership(container, uint32_t(int32_t(index) + n));
			}
			DCON_RELEASE_INLINE const_object_iterator_car_ownership operator-(int32_t n) const noexcept {
				return const_object_iterator_car_ownership(container, uint32_t(int32_t(index) - n));
			}
			DCON_RELEASE_INLINE int32_t operator-(const_object_iterator_car_ownership const& o) const noexcept {
				return int32_t(index) - int32_t(o.index);
			}
			DCON_RELEASE_INLINE bool operator>(const_object_iterator_car_ownership const& o) const noexcept {
				return index > o.index;
			}
			DCON_RELEASE_INLINE bool operator>=(const_object_iterator_car_ownership const& o) const noexcept {
				return index >= o.index;
			}
			DCON_RELEASE_INLINE bool operator<(const_object_iterator_car_ownership const& o) const noexcept {
				return index < o.index;
			}
			DCON_RELEASE_INLINE bool operator<=(const_object_iterator_car_ownership const& o) const noexcept {
				return index <= o.index;
			}
			DCON_RELEASE_INLINE car_ownership_const_fat_id operator[](int32_t n) const noexcept {
				return car_ownership_const_fat_id(container, car_ownership_id(car_ownership_id::value_base_t(int32_t(index) + n)));
			}
		};
		
	}

	class alignas(64) data_container {
		public:
		internal::thingy_class thingy;
		internal::car_class car;
		internal::person_class person;
		internal::car_ownership_class car_ownership;

		//
		// Functions for thingy:
		//
		//
		// accessors for thingy: some_value
		//
		DCON_RELEASE_INLINE int32_t const& thingy_get_some_value(thingy_id id) const noexcept {
			return thingy.m_some_value.vptr()[id.index()];
		}
		DCON_RELEASE_INLINE int32_t& thingy_get_some_value(thingy_id id) noexcept {
			return thingy.m_some_value.vptr()[id.index()];
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE ve::value_to_vector_type<int32_t> thingy_get_some_value(ve::contiguous_tags<thingy_id> id) const noexcept {
			return ve::load(id, thingy.m_some_value.vptr());
		}
		DCON_RELEASE_INLINE ve::value_to_vector_type<int32_t> thingy_get_some_value(ve::partial_contiguous_tags<thingy_id> id) const noexcept {
			return ve::load(id, thingy.m_some_value.vptr());
		}
		DCON_RELEASE_INLINE ve::value_to_vector_type<int32_t> thingy_get_some_value(ve::tagged_vector<thingy_id> id) const noexcept {
			return ve::load(id, thingy.m_some_value.vptr());
		}
		#endif
		DCON_RELEASE_INLINE void thingy_set_some_value(thingy_id id, int32_t value) noexcept {
			#ifdef DCON_TRAP_INVALID_STORE
			assert(id.index() >= 0);
			#endif
			thingy.m_some_value.vptr()[id.index()] = value;
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE void thingy_set_some_value(ve::contiguous_tags<thingy_id> id, ve::value_to_vector_type<int32_t> values) noexcept {
			ve::store(id, thingy.m_some_value.vptr(), values);
		}
		DCON_RELEASE_INLINE void thingy_set_some_value(ve::partial_contiguous_tags<thingy_id> id, ve::value_to_vector_type<int32_t> values) noexcept {
			ve::store(id, thingy.m_some_value.vptr(), values);
		}
		DCON_RELEASE_INLINE void thingy_set_some_value(ve::tagged_vector<thingy_id> id, ve::value_to_vector_type<int32_t> values) noexcept {
			ve::store(id, thingy.m_some_value.vptr(), values);
		}
		#endif
		//
		// accessors for thingy: bf_value
		//
		DCON_RELEASE_INLINE bool thingy_get_bf_value(thingy_id id) const noexcept {
			return dcon::bit_vector_test(thingy.m_bf_value.vptr(), id.index());
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE ve::vbitfield_type thingy_get_bf_value(ve::contiguous_tags<thingy_id> id) const noexcept {
			return ve::load(id, thingy.m_bf_value.vptr());
		}
		DCON_RELEASE_INLINE ve::vbitfield_type thingy_get_bf_value(ve::partial_contiguous_tags<thingy_id> id) const noexcept {
			return ve::load(id, thingy.m_bf_value.vptr());
		}
		DCON_RELEASE_INLINE ve::vbitfield_type thingy_get_bf_value(ve::tagged_vector<thingy_id> id) const noexcept {
			return ve::load(id, thingy.m_bf_value.vptr());
		}
		#endif
		DCON_RELEASE_INLINE void thingy_set_bf_value(thingy_id id, bool value) noexcept {
			#ifdef DCON_TRAP_INVALID_STORE
			assert(id.index() >= 0);
			#endif
			dcon::bit_vector_set(thingy.m_bf_value.vptr(), id.index(), value);
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE void thingy_set_bf_value(ve::contiguous_tags<thingy_id> id, ve::vbitfield_type values) noexcept {
			ve::store(id, thingy.m_bf_value.vptr(), values);
		}
		DCON_RELEASE_INLINE void thingy_set_bf_value(ve::partial_contiguous_tags<thingy_id> id, ve::vbitfield_type values) noexcept {
			ve::store(id, thingy.m_bf_value.vptr(), values);
		}
		DCON_RELEASE_INLINE void thingy_set_bf_value(ve::tagged_vector<thingy_id> id, ve::vbitfield_type values) noexcept {
			ve::store(id, thingy.m_bf_value.vptr(), values);
		}
		#endif
		//
		// accessors for thingy: obj_value
		//
		DCON_RELEASE_INLINE std::vector<float> const& thingy_get_obj_value(thingy_id id) const noexcept {
			return thingy.m_obj_value.vptr()[id.index()];
		}
		DCON_RELEASE_INLINE std::vector<float>& thingy_get_obj_value(thingy_id id) noexcept {
			return thingy.m_obj_value.vptr()[id.index()];
		}
		DCON_RELEASE_INLINE void thingy_set_obj_value(thingy_id id, std::vector<float> const& value) noexcept {
			#ifdef DCON_TRAP_INVALID_STORE
			assert(id.index() >= 0);
			#endif
			thingy.m_obj_value.vptr()[id.index()] = value;
		}
		//
		// accessors for thingy: pooled_v
		//
		DCON_RELEASE_INLINE dcon::dcon_vv_const_fat_id<int16_t> thingy_get_pooled_v(thingy_id id) const noexcept {
			return dcon::dcon_vv_const_fat_id<int16_t>(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[id.index()]);
		}
		DCON_RELEASE_INLINE dcon::dcon_vv_fat_id<int16_t> thingy_get_pooled_v(thingy_id id) noexcept {
			return dcon::dcon_vv_fat_id<int16_t>(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[id.index()]);
		}
		//
		// accessors for thingy: big_array
		//
		DCON_RELEASE_INLINE float const& thingy_get_big_array(thingy_id id, int32_t n) const noexcept {
			return thingy.m_big_array.vptr(dcon::get_index(n))[id.index()];
		}
		DCON_RELEASE_INLINE float& thingy_get_big_array(thingy_id id, int32_t n) noexcept {
			return thingy.m_big_array.vptr(dcon::get_index(n))[id.index()];
		}
		DCON_RELEASE_INLINE uint32_t thingy_get_big_array_size() const noexcept {
			return thingy.m_big_array.size;
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE ve::value_to_vector_type<float> thingy_get_big_array(ve::contiguous_tags<thingy_id> id, int32_t n) const noexcept {
			return ve::load(id, thingy.m_big_array.vptr(dcon::get_index(n)));
		}
		DCON_RELEASE_INLINE ve::value_to_vector_type<float> thingy_get_big_array(ve::partial_contiguous_tags<thingy_id> id, int32_t n) const noexcept {
			return ve::load(id, thingy.m_big_array.vptr(dcon::get_index(n)));
		}
		DCON_RELEASE_INLINE ve::value_to_vector_type<float> thingy_get_big_array(ve::tagged_vector<thingy_id> id, int32_t n) const noexcept {
			return ve::load(id, thingy.m_big_array.vptr(dcon::get_index(n)));
		}
		#endif
		DCON_RELEASE_INLINE void thingy_set_big_array(thingy_id id, int32_t n, float value) noexcept {
			#ifdef DCON_TRAP_INVALID_STORE
			assert(id.index() >= 0);
			assert(dcon::get_index(n) >= 0);
			#endif
			thingy.m_big_array.vptr(dcon::get_index(n))[id.index()] = value;
		}
		DCON_RELEASE_INLINE void thingy_resize_big_array(uint32_t size) noexcept {
			return thingy.m_big_array.resize(size, thingy.size_used);
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE void thingy_set_big_array(ve::contiguous_tags<thingy_id> id, int32_t n, ve::value_to_vector_type<float> values) noexcept {
			ve::store(id, thingy.m_big_array.vptr(dcon::get_index(n)), values);
		}
		DCON_RELEASE_INLINE void thingy_set_big_array(ve::partial_contiguous_tags<thingy_id> id, int32_t n, ve::value_to_vector_type<float> values) noexcept {
			ve::store(id, thingy.m_big_array.vptr(dcon::get_index(n)), values);
		}
		DCON_RELEASE_INLINE void thingy_set_big_array(ve::tagged_vector<thingy_id> id, int32_t n, ve::value_to_vector_type<float> values) noexcept {
			ve::store(id, thingy.m_big_array.vptr(dcon::get_index(n)), values);
		}
		#endif
		//
		// accessors for thingy: big_array_bf
		//
		DCON_RELEASE_INLINE bool thingy_get_big_array_bf(thingy_id id, int32_t n) const noexcept {
			return dcon::bit_vector_test(thingy.m_big_array_bf.vptr(dcon::get_index(n)), id.index());
		}
		DCON_RELEASE_INLINE uint32_t thingy_get_big_array_bf_size() const noexcept {
			return thingy.m_big_array_bf.size;
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE ve::vbitfield_type thingy_get_big_array_bf(ve::contiguous_tags<thingy_id> id, int32_t n) const noexcept {
			return ve::load(id, thingy.m_big_array_bf.vptr(dcon::get_index(n)));
		}
		DCON_RELEASE_INLINE ve::vbitfield_type thingy_get_big_array_bf(ve::partial_contiguous_tags<thingy_id> id, int32_t n) const noexcept {
			return ve::load(id, thingy.m_big_array_bf.vptr(dcon::get_index(n)));
		}
		DCON_RELEASE_INLINE ve::vbitfield_type thingy_get_big_array_bf(ve::tagged_vector<thingy_id> id, int32_t n) const noexcept {
			return ve::load(id, thingy.m_big_array_bf.vptr(dcon::get_index(n)));
		}
		#endif
		DCON_RELEASE_INLINE void thingy_set_big_array_bf(thingy_id id, int32_t n, bool value) noexcept {
			#ifdef DCON_TRAP_INVALID_STORE
			assert(id.index() >= 0);
			assert(dcon::get_index(n) >= 0);
			#endif
			dcon::bit_vector_set(thingy.m_big_array_bf.vptr(dcon::get_index(n)), id.index(), value);
		}
		DCON_RELEASE_INLINE void thingy_resize_big_array_bf(uint32_t size) noexcept {
			thingy.m_big_array_bf.resize(size, thingy.size_used);
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE void thingy_set_big_array_bf(ve::contiguous_tags<thingy_id> id, int32_t n, ve::vbitfield_type values) noexcept {
			ve::store(id, thingy.m_big_array_bf.vptr(dcon::get_index(n)), values);
		}
		DCON_RELEASE_INLINE void thingy_set_big_array_bf(ve::partial_contiguous_tags<thingy_id> id, int32_t n, ve::vbitfield_type values) noexcept {
			ve::store(id, thingy.m_big_array_bf.vptr(dcon::get_index(n)), values);
		}
		DCON_RELEASE_INLINE void thingy_set_big_array_bf(ve::tagged_vector<thingy_id> id, int32_t n, ve::vbitfield_type values) noexcept {
			ve::store(id, thingy.m_big_array_bf.vptr(dcon::get_index(n)), values);
		}
		#endif
		DCON_RELEASE_INLINE bool thingy_is_valid(thingy_id id) const noexcept {
			return bool(id) && uint32_t(id.index()) < thingy.size_used;
		}
		
		uint32_t thingy_size() const noexcept { return thingy.size_used; }

		//
		// Functions for car:
		//
		//
		// accessors for car: wheels
		//
		DCON_RELEASE_INLINE int32_t const& car_get_wheels(car_id id) const noexcept {
			return car.m_wheels.vptr()[id.index()];
		}
		DCON_RELEASE_INLINE int32_t& car_get_wheels(car_id id) noexcept {
			return car.m_wheels.vptr()[id.index()];
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE ve::value_to_vector_type<int32_t> car_get_wheels(ve::contiguous_tags<car_id> id) const noexcept {
			return ve::load(id, car.m_wheels.vptr());
		}
		DCON_RELEASE_INLINE ve::value_to_vector_type<int32_t> car_get_wheels(ve::partial_contiguous_tags<car_id> id) const noexcept {
			return ve::load(id, car.m_wheels.vptr());
		}
		DCON_RELEASE_INLINE ve::value_to_vector_type<int32_t> car_get_wheels(ve::tagged_vector<car_id> id) const noexcept {
			return ve::load(id, car.m_wheels.vptr());
		}
		#endif
		DCON_RELEASE_INLINE void car_set_wheels(car_id id, int32_t value) noexcept {
			#ifdef DCON_TRAP_INVALID_STORE
			assert(id.index() >= 0);
			#endif
			car.m_wheels.vptr()[id.index()] = value;
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE void car_set_wheels(ve::contiguous_tags<car_id> id, ve::value_to_vector_type<int32_t> values) noexcept {
			ve::store(id, car.m_wheels.vptr(), values);
		}
		DCON_RELEASE_INLINE void car_set_wheels(ve::partial_contiguous_tags<car_id> id, ve::value_to_vector_type<int32_t> values) noexcept {
			ve::store(id, car.m_wheels.vptr(), values);
		}
		DCON_RELEASE_INLINE void car_set_wheels(ve::tagged_vector<car_id> id, ve::value_to_vector_type<int32_t> values) noexcept {
			ve::store(id, car.m_wheels.vptr(), values);
		}
		#endif
		//
		// accessors for car: resale_value
		//
		DCON_RELEASE_INLINE float const& car_get_resale_value(car_id id) const noexcept {
			return car.m_resale_value.vptr()[id.index()];
		}
		DCON_RELEASE_INLINE float& car_get_resale_value(car_id id) noexcept {
			return car.m_resale_value.vptr()[id.index()];
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE ve::value_to_vector_type<float> car_get_resale_value(ve::contiguous_tags<car_id> id) const noexcept {
			return ve::load(id, car.m_resale_value.vptr());
		}
		DCON_RELEASE_INLINE ve::value_to_vector_type<float> car_get_resale_value(ve::partial_contiguous_tags<car_id> id) const noexcept {
			return ve::load(id, car.m_resale_value.vptr());
		}
		DCON_RELEASE_INLINE ve::value_to_vector_type<float> car_get_resale_value(ve::tagged_vector<car_id> id) const noexcept {
			return ve::load(id, car.m_resale_value.vptr());
		}
		#endif
		DCON_RELEASE_INLINE void car_set_resale_value(car_id id, float value) noexcept {
			#ifdef DCON_TRAP_INVALID_STORE
			assert(id.index() >= 0);
			#endif
			car.m_resale_value.vptr()[id.index()] = value;
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE void car_set_resale_value(ve::contiguous_tags<car_id> id, ve::value_to_vector_type<float> values) noexcept {
			ve::store(id, car.m_resale_value.vptr(), values);
		}
		DCON_RELEASE_INLINE void car_set_resale_value(ve::partial_contiguous_tags<car_id> id, ve::value_to_vector_type<float> values) noexcept {
			ve::store(id, car.m_resale_value.vptr(), values);
		}
		DCON_RELEASE_INLINE void car_set_resale_value(ve::tagged_vector<car_id> id, ve::value_to_vector_type<float> values) noexcept {
			ve::store(id, car.m_resale_value.vptr(), values);
		}
		#endif
		DCON_RELEASE_INLINE car_ownership_id car_get_car_ownership_as_owned_car(car_id id) const noexcept {
			return (id.value <= car.size_used) ? car_ownership_id(car_ownership_id::value_base_t(id.index())) : car_ownership_id();
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE ve::contiguous_tags<car_ownership_id> car_get_car_ownership_as_owned_car(ve::contiguous_tags<car_id> id) const noexcept {
			return ve::contiguous_tags<car_ownership_id>(id.value);
		}
		DCON_RELEASE_INLINE ve::partial_contiguous_tags<car_ownership_id> car_get_car_ownership_as_owned_car(ve::partial_contiguous_tags<car_id> id) const noexcept {
			return ve::partial_contiguous_tags<car_ownership_id>(id.value, id.subcount);
		}
		DCON_RELEASE_INLINE ve::tagged_vector<car_ownership_id> car_get_car_ownership_as_owned_car(ve::tagged_vector<car_id> id) const noexcept {
			return ve::tagged_vector<car_ownership_id>(id, std::true_type{});
		}
		#endif
		DCON_RELEASE_INLINE void car_remove_car_ownership_as_owned_car(car_id id) noexcept {
			if(car_ownership_is_valid(car_ownership_id(car_ownership_id::value_base_t(id.index())))) {
				car_ownership_set_owned_car(car_ownership_id(car_ownership_id::value_base_t(id.index())), car_id());
			}
		}
		DCON_RELEASE_INLINE car_ownership_id car_get_car_ownership(car_id id) const noexcept {
			return (id.value <= car.size_used) ? car_ownership_id(car_ownership_id::value_base_t(id.index())) : car_ownership_id();
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE ve::contiguous_tags<car_ownership_id> car_get_car_ownership(ve::contiguous_tags<car_id> id) const noexcept {
			return ve::contiguous_tags<car_ownership_id>(id.value);
		}
		DCON_RELEASE_INLINE ve::partial_contiguous_tags<car_ownership_id> car_get_car_ownership(ve::partial_contiguous_tags<car_id> id) const noexcept {
			return ve::partial_contiguous_tags<car_ownership_id>(id.value, id.subcount);
		}
		DCON_RELEASE_INLINE ve::tagged_vector<car_ownership_id> car_get_car_ownership(ve::tagged_vector<car_id> id) const noexcept {
			return ve::tagged_vector<car_ownership_id>(id, std::true_type{});
		}
		#endif
		DCON_RELEASE_INLINE void car_remove_car_ownership(car_id id) noexcept {
			if(car_ownership_is_valid(car_ownership_id(car_ownership_id::value_base_t(id.index())))) {
				car_ownership_set_owned_car(car_ownership_id(car_ownership_id::value_base_t(id.index())), car_id());
			}
		}
		DCON_RELEASE_INLINE person_id car_get_owner_from_car_ownership(car_id ref_id) const {
			return car_ownership_get_owner(car_ownership_id(car_ownership_id::value_base_t(ref_id.index())));
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE ve::value_to_vector_type<person_id> car_get_owner_from_car_ownership(ve::contiguous_tags<car_id> ref_id) const {
			return car_ownership_get_owner(ve::contiguous_tags<car_ownership_id>(ref_id.value));
		}
		DCON_RELEASE_INLINE ve::value_to_vector_type<person_id> car_get_owner_from_car_ownership(ve::partial_contiguous_tags<car_id> ref_id) const {
			return car_ownership_get_owner(ve::partial_contiguous_tags<car_ownership_id>(ref_id.value, ref_id.subcount));
		}
		DCON_RELEASE_INLINE ve::value_to_vector_type<person_id> car_get_owner_from_car_ownership(ve::tagged_vector<car_id> ref_id) const {
			return car_ownership_get_owner(ve::tagged_vector<car_ownership_id>(ref_id, std::true_type{}));
		}
		#endif
		void car_set_owner_from_car_ownership(car_id ref_id, person_id val) {
			car_ownership_set_owner(car_ownership_id(car_ownership_id::value_base_t(ref_id.index())), val);
		}
		void car_set_ownership_date_from_car_ownership(car_id ref_id, int32_t val) {
			car_ownership_set_ownership_date(car_ownership_id(car_ownership_id::value_base_t(ref_id.index())), val);
		}
		int32_t car_get_ownership_date_from_car_ownership(car_id ref_id) const {
			return car_ownership_get_ownership_date(car_ownership_id(car_ownership_id::value_base_t(ref_id.index())));
		}
		#ifndef DCON_NO_VE
		ve::value_to_vector_type<int32_t> car_get_ownership_date_from_car_ownership(ve::contiguous_tags<car_id> ref_id) const {
			return car_ownership_get_ownership_date(ve::contiguous_tags<car_ownership_id>(ref_id.value));
		}
		ve::value_to_vector_type<int32_t> car_get_ownership_date_from_car_ownership(ve::partial_contiguous_tags<car_id> ref_id) const {
			return car_ownership_get_ownership_date(ve::partial_contiguous_tags<car_ownership_id>(ref_id.value, ref_id.subcount));
		}
		ve::value_to_vector_type<int32_t> car_get_ownership_date_from_car_ownership(ve::tagged_vector<car_id> ref_id) const {
			return car_ownership_get_ownership_date(ve::tagged_vector<car_ownership_id>(ref_id, std::true_type{}));
		}
		#endif
		DCON_RELEASE_INLINE bool car_is_valid(car_id id) const noexcept {
			return bool(id) && uint32_t(id.index()) < car.size_used;
		}
		
		uint32_t car_size() const noexcept { return car.size_used; }

		//
		// Functions for person:
		//
		//
		// accessors for person: age
		//
		DCON_RELEASE_INLINE int32_t const& person_get_age(person_id id) const noexcept {
			return person.m_age.vptr()[id.index()];
		}
		DCON_RELEASE_INLINE int32_t& person_get_age(person_id id) noexcept {
			return person.m_age.vptr()[id.index()];
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE ve::value_to_vector_type<int32_t> person_get_age(ve::contiguous_tags<person_id> id) const noexcept {
			return ve::load(id, person.m_age.vptr());
		}
		DCON_RELEASE_INLINE ve::value_to_vector_type<int32_t> person_get_age(ve::partial_contiguous_tags<person_id> id) const noexcept {
			return ve::load(id, person.m_age.vptr());
		}
		DCON_RELEASE_INLINE ve::value_to_vector_type<int32_t> person_get_age(ve::tagged_vector<person_id> id) const noexcept {
			return ve::load(id, person.m_age.vptr());
		}
		#endif
		DCON_RELEASE_INLINE void person_set_age(person_id id, int32_t value) noexcept {
			#ifdef DCON_TRAP_INVALID_STORE
			assert(id.index() >= 0);
			#endif
			person.m_age.vptr()[id.index()] = value;
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE void person_set_age(ve::contiguous_tags<person_id> id, ve::value_to_vector_type<int32_t> values) noexcept {
			ve::store(id, person.m_age.vptr(), values);
		}
		DCON_RELEASE_INLINE void person_set_age(ve::partial_contiguous_tags<person_id> id, ve::value_to_vector_type<int32_t> values) noexcept {
			ve::store(id, person.m_age.vptr(), values);
		}
		DCON_RELEASE_INLINE void person_set_age(ve::tagged_vector<person_id> id, ve::value_to_vector_type<int32_t> values) noexcept {
			ve::store(id, person.m_age.vptr(), values);
		}
		#endif
		DCON_RELEASE_INLINE internal::const_iterator_person_foreach_car_ownership_as_owner_generator person_get_car_ownership_as_owner(person_id id) const {
			return internal::const_iterator_person_foreach_car_ownership_as_owner_generator(*this, id);
		}
		DCON_RELEASE_INLINE internal::iterator_person_foreach_car_ownership_as_owner_generator person_get_car_ownership_as_owner(person_id id) {
			return internal::iterator_person_foreach_car_ownership_as_owner_generator(*this, id);
		}
		template<typename T>
		DCON_RELEASE_INLINE void person_for_each_car_ownership_as_owner(person_id id, T&& func) const {
			if(bool(id)) {
				auto vrange = dcon::get_range(car_ownership.owner_storage, car_ownership.m_array_owner.vptr()[id.index()]);
				std::for_each(vrange.first, vrange.second, func);
			}
		}
		DCON_RELEASE_INLINE std::pair<car_ownership_id const*, car_ownership_id const*> person_range_of_car_ownership_as_owner(person_id id) const {
			if(bool(id)) {
				auto vrange = dcon::get_range(car_ownership.owner_storage, car_ownership.m_array_owner.vptr()[id.index()]);
				return std::pair<car_ownership_id const*, car_ownership_id const*>(vrange.first, vrange.second);
			} else {
				return std::pair<car_ownership_id const*, car_ownership_id const*>(nullptr, nullptr);
			}
		}
		void person_remove_all_car_ownership_as_owner(person_id id) noexcept {
			auto rng = person_range_of_car_ownership_as_owner(id);
			dcon::local_vector<car_ownership_id> temp(rng.first, rng.second);
			std::for_each(temp.begin(), temp.end(), [t = this](car_ownership_id i) { t->car_ownership_set_owner(i, person_id()); });
		}
		DCON_RELEASE_INLINE internal::const_iterator_person_foreach_car_ownership_as_owner_generator person_get_car_ownership(person_id id) const {
			return internal::const_iterator_person_foreach_car_ownership_as_owner_generator(*this, id);
		}
		DCON_RELEASE_INLINE internal::iterator_person_foreach_car_ownership_as_owner_generator person_get_car_ownership(person_id id) {
			return internal::iterator_person_foreach_car_ownership_as_owner_generator(*this, id);
		}
		template<typename T>
		DCON_RELEASE_INLINE void person_for_each_car_ownership(person_id id, T&& func) const {
			if(bool(id)) {
				auto vrange = dcon::get_range(car_ownership.owner_storage, car_ownership.m_array_owner.vptr()[id.index()]);
				std::for_each(vrange.first, vrange.second, func);
			}
		}
		DCON_RELEASE_INLINE std::pair<car_ownership_id const*, car_ownership_id const*> person_range_of_car_ownership(person_id id) const {
			if(bool(id)) {
				auto vrange = dcon::get_range(car_ownership.owner_storage, car_ownership.m_array_owner.vptr()[id.index()]);
				return std::pair<car_ownership_id const*, car_ownership_id const*>(vrange.first, vrange.second);
			} else {
				return std::pair<car_ownership_id const*, car_ownership_id const*>(nullptr, nullptr);
			}
		}
		void person_remove_all_car_ownership(person_id id) noexcept {
			auto rng = person_range_of_car_ownership_as_owner(id);
			dcon::local_vector<car_ownership_id> temp(rng.first, rng.second);
			std::for_each(temp.begin(), temp.end(), [t = this](car_ownership_id i) { t->car_ownership_set_owner(i, person_id()); });
		}
		DCON_RELEASE_INLINE bool person_is_valid(person_id id) const noexcept {
			return bool(id) && uint32_t(id.index()) < person.size_used && person.m__index.vptr()[id.index()] == id;
		}
		
		uint32_t person_size() const noexcept { return person.size_used; }

		//
		// Functions for car_ownership:
		//
		//
		// accessors for car_ownership: ownership_date
		//
		DCON_RELEASE_INLINE int32_t const& car_ownership_get_ownership_date(car_ownership_id id) const noexcept {
			return car_ownership.m_ownership_date.vptr()[id.index()];
		}
		DCON_RELEASE_INLINE int32_t& car_ownership_get_ownership_date(car_ownership_id id) noexcept {
			return car_ownership.m_ownership_date.vptr()[id.index()];
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE ve::value_to_vector_type<int32_t> car_ownership_get_ownership_date(ve::contiguous_tags<car_ownership_id> id) const noexcept {
			return ve::load(id, car_ownership.m_ownership_date.vptr());
		}
		DCON_RELEASE_INLINE ve::value_to_vector_type<int32_t> car_ownership_get_ownership_date(ve::partial_contiguous_tags<car_ownership_id> id) const noexcept {
			return ve::load(id, car_ownership.m_ownership_date.vptr());
		}
		DCON_RELEASE_INLINE ve::value_to_vector_type<int32_t> car_ownership_get_ownership_date(ve::tagged_vector<car_ownership_id> id) const noexcept {
			return ve::load(id, car_ownership.m_ownership_date.vptr());
		}
		#endif
		DCON_RELEASE_INLINE void car_ownership_set_ownership_date(car_ownership_id id, int32_t value) noexcept {
			#ifdef DCON_TRAP_INVALID_STORE
			assert(id.index() >= 0);
			#endif
			car_ownership.m_ownership_date.vptr()[id.index()] = value;
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE void car_ownership_set_ownership_date(ve::contiguous_tags<car_ownership_id> id, ve::value_to_vector_type<int32_t> values) noexcept {
			ve::store(id, car_ownership.m_ownership_date.vptr(), values);
		}
		DCON_RELEASE_INLINE void car_ownership_set_ownership_date(ve::partial_contiguous_tags<car_ownership_id> id, ve::value_to_vector_type<int32_t> values) noexcept {
			ve::store(id, car_ownership.m_ownership_date.vptr(), values);
		}
		DCON_RELEASE_INLINE void car_ownership_set_ownership_date(ve::tagged_vector<car_ownership_id> id, ve::value_to_vector_type<int32_t> values) noexcept {
			ve::store(id, car_ownership.m_ownership_date.vptr(), values);
		}
		#endif
		DCON_RELEASE_INLINE person_id car_ownership_get_owner(car_ownership_id id) const noexcept {
			return car_ownership.m_owner.vptr()[id.index()];
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE ve::value_to_vector_type<person_id> car_ownership_get_owner(ve::contiguous_tags<car_ownership_id> id) const noexcept {
			return ve::load(id, car_ownership.m_owner.vptr());
		}
		DCON_RELEASE_INLINE ve::value_to_vector_type<person_id> car_ownership_get_owner(ve::partial_contiguous_tags<car_ownership_id> id) const noexcept {
			return ve::load(id, car_ownership.m_owner.vptr());
		}
		DCON_RELEASE_INLINE ve::value_to_vector_type<person_id> car_ownership_get_owner(ve::tagged_vector<car_ownership_id> id) const noexcept {
			return ve::load(id, car_ownership.m_owner.vptr());
		}
		#endif
		private:
		void internal_car_ownership_set_owner(car_ownership_id id, person_id value) noexcept {
			if(auto old_value = car_ownership.m_owner.vptr()[id.index()]; bool(old_value)) {
				auto& vref = car_ownership.m_array_owner.vptr()[old_value.index()];
				dcon::remove_unique_item(car_ownership.owner_storage, vref, id);
			}
			if(bool(value)) {
				dcon::push_back(car_ownership.owner_storage, car_ownership.m_array_owner.vptr()[value.index()], id);
			}
			car_ownership.m_owner.vptr()[id.index()] = value;
		}
		public:
		void car_ownership_set_owner(car_ownership_id id, person_id value) noexcept {
			if(!bool(value)) {
				delete_car_ownership(id);
				return;
			}
			internal_car_ownership_set_owner(id, value);
		}
		bool car_ownership_try_set_owner(car_ownership_id id, person_id value) noexcept {
			if(!bool(value)) {
				return false;
			}
			internal_car_ownership_set_owner(id, value);
			return true;
		}
		DCON_RELEASE_INLINE car_id car_ownership_get_owned_car(car_ownership_id id) const noexcept {
			return car_id(car_id::value_base_t(id.index()));
		}
		#ifndef DCON_NO_VE
		DCON_RELEASE_INLINE ve::contiguous_tags<car_id> car_ownership_get_owned_car(ve::contiguous_tags<car_ownership_id> id) const noexcept {
			return ve::contiguous_tags<car_id>(id.value);
		}
		DCON_RELEASE_INLINE ve::partial_contiguous_tags<car_id> car_ownership_get_owned_car(ve::partial_contiguous_tags<car_ownership_id> id) const noexcept {
			return ve::partial_contiguous_tags<car_id>(id.value, id.subcount);
		}
		DCON_RELEASE_INLINE ve::tagged_vector<car_id> car_ownership_get_owned_car(ve::tagged_vector<car_ownership_id> id) const noexcept {
			return ve::tagged_vector<car_id>(id, std::true_type{});
		}
		#endif
		private:
		void internal_car_ownership_set_owned_car(car_ownership_id id, car_id value) noexcept {
			if(bool(value)) {
				delete_car_ownership( car_ownership_id(car_ownership_id::value_base_t(value.index())) );
				internal_move_relationship_car_ownership(id, car_ownership_id(car_ownership_id::value_base_t(value.index())) );
			}
		}
		public:
		void car_ownership_set_owned_car(car_ownership_id id, car_id value) noexcept {
			if(bool(value)) {
				delete_car_ownership( car_ownership_id(car_ownership_id::value_base_t(value.index())) );
				internal_move_relationship_car_ownership(id, car_ownership_id(car_ownership_id::value_base_t(value.index())) );
			} else {
				delete_car_ownership(id);
			}
		}
		bool car_ownership_try_set_owned_car(car_ownership_id id, car_id value) noexcept {
			if(bool(value)) {
				if(car_ownership_is_valid( car_ownership_id(car_ownership_id::value_base_t(value.index())) )) return false;
				internal_move_relationship_car_ownership(id, car_ownership_id(car_ownership_id::value_base_t(value.index())) );
				return true;
			} else {
				return false;
			}
		}
		DCON_RELEASE_INLINE bool car_ownership_is_valid(car_ownership_id id) const noexcept {
			return bool(id) && uint32_t(id.index()) < car.size_used && car_is_valid(car_id(car_id::value_base_t(id.index()))) && (bool(car_ownership.m_owner.vptr()[id.index()]) || false);
		}
		
		uint32_t car_ownership_size() const noexcept { return car.size_used; }


		//
		// container pop_back for thingy
		//
		void pop_back_thingy() {
			if(thingy.size_used == 0) return;
			thingy_id id_removed(thingy_id::value_base_t(thingy.size_used - 1));
			thingy.m_some_value.vptr()[id_removed.index()] = int32_t{};
			dcon::bit_vector_set(thingy.m_bf_value.vptr(), id_removed.index(), false);
			thingy.m_obj_value.vptr()[id_removed.index()] = std::vector<float>{};
			thingy.pooled_v_storage.release(thingy.m_pooled_v.vptr()[id_removed.index()]);
			thingy.m_big_array.zero_at(id_removed.index());
			thingy.m_big_array_bf.zero_at(id_removed.index());
			--thingy.size_used;
		}
		
		//
		// container resize for thingy
		//
		void thingy_resize(uint32_t new_size) {
			#ifndef DCON_USE_EXCEPTIONS
			if(new_size > 1200) std::abort();
			#else
			if(new_size > 1200) throw dcon::out_of_space{};
			#endif
			const uint32_t old_size = thingy.size_used;
			if(new_size < old_size) {
				std::fill_n(thingy.m_some_value.vptr() + new_size, old_size - new_size, int32_t{});
				for(uint32_t s = new_size; s < 8 * (((new_size + 7) / 8)); ++s) {
					dcon::bit_vector_set(thingy.m_bf_value.vptr(), s, false);
				}
				std::fill_n(thingy.m_bf_value.vptr() + (new_size + 7) / 8, (new_size + old_size - new_size + 7) / 8 - (new_size + 7) / 8, dcon::bitfield_type{0});
				std::destroy_n(thingy.m_obj_value.vptr() + new_size, old_size - new_size);
				std::uninitialized_default_construct_n(thingy.m_obj_value.vptr() + new_size, old_size - new_size);
				std::for_each(thingy.m_pooled_v.vptr() + new_size, thingy.m_pooled_v.vptr() + new_size + old_size - new_size, [t = this](dcon::stable_mk_2_tag& i){ t->thingy.pooled_v_storage.release(i); });
				for(int32_t s = 0; s < int32_t(thingy.m_big_array.size); ++s) {
					std::fill_n(thingy.m_big_array.vptr(s) + new_size, old_size - new_size, float{});
				}
				for(int32_t s = 0; s < int32_t(thingy.m_big_array_bf.size); ++s) {
					for(uint32_t t = new_size; t < 8 * (((new_size + 7) / 8)); ++t) {
						dcon::bit_vector_set(thingy.m_big_array_bf.vptr(s), t, false);
					}
					std::fill_n(thingy.m_big_array_bf.vptr(s) + (new_size + 7) / 8, (new_size + old_size - new_size + 7) / 8 - (new_size + 7) / 8, dcon::bitfield_type{0});
				}
			} else if(new_size > old_size) {
			}
			thingy.size_used = new_size;
		}
		
		//
		// container create for thingy
		//
		thingy_id create_thingy() {
			thingy_id new_id(thingy_id::value_base_t(thingy.size_used));
			#ifndef DCON_USE_EXCEPTIONS
			if(thingy.size_used >= 1200) std::abort();
			#else
			if(thingy.size_used >= 1200) throw dcon::out_of_space{};
			#endif
			++thingy.size_used;
			return new_id;
		}
		
		//
		// container compactable delete for thingy
		//
		void delete_thingy(thingy_id id) {
			thingy_id id_removed = id;
			#ifndef NDEBUG
			assert(id.index() >= 0);
			assert(uint32_t(id.index()) < thingy.size_used );
			assert(thingy.size_used != 0);
			#endif
			thingy_id last_id(thingy_id::value_base_t(thingy.size_used - 1));
			if(id_removed == last_id) { pop_back_thingy(); return; }
			thingy.m_some_value.vptr()[id_removed.index()] = std::move(thingy.m_some_value.vptr()[last_id.index()]);
			thingy.m_some_value.vptr()[last_id.index()] = int32_t{};
			dcon::bit_vector_set(thingy.m_bf_value.vptr(), id_removed.index(), dcon::bit_vector_test(thingy.m_bf_value.vptr(), last_id.index()));
			dcon::bit_vector_set(thingy.m_bf_value.vptr(), last_id.index(), false);
			thingy.m_obj_value.vptr()[id_removed.index()] = std::move(thingy.m_obj_value.vptr()[last_id.index()]);
			thingy.m_obj_value.vptr()[last_id.index()] = std::vector<float>{};
			thingy.pooled_v_storage.release(thingy.m_pooled_v.vptr()[id_removed.index()]);
			thingy.m_pooled_v.vptr()[id_removed.index()] = std::move(thingy.m_pooled_v.vptr()[last_id.index()]);
			thingy.m_pooled_v.vptr()[last_id.index()] = std::numeric_limits<dcon::stable_mk_2_tag>::max();
			thingy.m_big_array.copy_value(id_removed.index(), last_id.index());
			thingy.m_big_array.zero_at(last_id.index());
			thingy.m_big_array_bf.copy_value(id_removed.index(), last_id.index());
			thingy.m_big_array_bf.zero_at(last_id.index());
			--thingy.size_used;
		}
		
		//
		// container pop_back for car
		//
		void pop_back_car() {
			if(car.size_used == 0) return;
			car_id id_removed(car_id::value_base_t(car.size_used - 1));
			delete_car_ownership(car_ownership_id(car_ownership_id::value_base_t(id_removed.index())));
			car.m_wheels.vptr()[id_removed.index()] = int32_t{};
			car.m_resale_value.vptr()[id_removed.index()] = float{};
			--car.size_used;
		}
		
		//
		// container resize for car
		//
		void car_resize(uint32_t new_size) {
			#ifndef DCON_USE_EXCEPTIONS
			if(new_size > 1200) std::abort();
			#else
			if(new_size > 1200) throw dcon::out_of_space{};
			#endif
			const uint32_t old_size = car.size_used;
			if(new_size < old_size) {
				std::fill_n(car.m_wheels.vptr() + new_size, old_size - new_size, int32_t{});
				std::fill_n(car.m_resale_value.vptr() + new_size, old_size - new_size, float{});
				car_ownership_resize(std::min(new_size, car.size_used));
			} else if(new_size > old_size) {
			}
			car.size_used = new_size;
		}
		
		//
		// container create for car
		//
		car_id create_car() {
			car_id new_id(car_id::value_base_t(car.size_used));
			#ifndef DCON_USE_EXCEPTIONS
			if(car.size_used >= 1200) std::abort();
			#else
			if(car.size_used >= 1200) throw dcon::out_of_space{};
			#endif
			++car.size_used;
			return new_id;
		}
		
		//
		// container compactable delete for car
		//
		void delete_car(car_id id) {
			car_id id_removed = id;
			#ifndef NDEBUG
			assert(id.index() >= 0);
			assert(uint32_t(id.index()) < car.size_used );
			assert(car.size_used != 0);
			#endif
			car_id last_id(car_id::value_base_t(car.size_used - 1));
			if(id_removed == last_id) { pop_back_car(); return; }
			delete_car_ownership(car_ownership_id(car_ownership_id::value_base_t(id_removed.index())));
			internal_move_relationship_car_ownership(car_ownership_id(car_ownership_id::value_base_t(last_id.index())), car_ownership_id(car_ownership_id::value_base_t(id_removed.index())));
			car.m_wheels.vptr()[id_removed.index()] = std::move(car.m_wheels.vptr()[last_id.index()]);
			car.m_wheels.vptr()[last_id.index()] = int32_t{};
			car.m_resale_value.vptr()[id_removed.index()] = std::move(car.m_resale_value.vptr()[last_id.index()]);
			car.m_resale_value.vptr()[last_id.index()] = float{};
			--car.size_used;
		}
		
		//
		// container delete for person
		//
		void delete_person(person_id id_removed) {
			#ifndef NDEBUG
			assert(id_removed.index() >= 0);
			assert(person.m__index.vptr()[id_removed.index()] == id_removed);
			#endif
			person.m__index.vptr()[id_removed.index()] = person.first_free;
			person.first_free = id_removed;
			if(int32_t(person.size_used) - 1 == id_removed.index()) {
				for( ; person.size_used > 0 && person.m__index.vptr()[person.size_used - 1] != person_id(person_id::value_base_t(person.size_used - 1));  --person.size_used) ;
			}
			person_remove_all_car_ownership_as_owner(id_removed);
			person.m_age.vptr()[id_removed.index()] = int32_t{};
		}
		
		//
		// container create for person
		//
		person_id create_person() {
			#ifndef DCON_USE_EXCEPTIONS
			if(!bool(person.first_free)) std::abort();
			#else
			if(!bool(person.first_free)) throw dcon::out_of_space{};
			#endif
			person_id new_id = person.first_free;
			person.first_free = person.m__index.vptr()[person.first_free.index()];
			person.m__index.vptr()[new_id.index()] = new_id;
			person.size_used = std::max(person.size_used, uint32_t(new_id.index() + 1));
			return new_id;
		}
		
		//
		// container resize for person
		//
		void person_resize(uint32_t new_size) {
			#ifndef DCON_USE_EXCEPTIONS
			if(new_size > 100) std::abort();
			#else
			if(new_size > 100) throw dcon::out_of_space{};
			#endif
			const uint32_t old_size = person.size_used;
			if(new_size < old_size) {
				person.first_free = person_id();
				int32_t i = int32_t(100 - 1);
				for(; i >= int32_t(new_size); --i) {
					person.m__index.vptr()[i] = person.first_free;
					person.first_free = person_id(person_id::value_base_t(i));
				}
				for(; i >= 0; --i) {
					if(person.m__index.vptr()[i] != person_id(person_id::value_base_t(i))) {
						person.m__index.vptr()[i] = person.first_free;
						person.first_free = person_id(person_id::value_base_t(i));
					}
				}
				std::fill_n(person.m_age.vptr() + new_size, old_size - new_size, int32_t{});
				car_ownership_resize(0);
			} else if(new_size > old_size) {
				person.first_free = person_id();
				int32_t i = int32_t(100 - 1);
				for(; i >= int32_t(old_size); --i) {
					person.m__index.vptr()[i] = person.first_free;
					person.first_free = person_id(person_id::value_base_t(i));
				}
				for(; i >= 0; --i) {
					if(person.m__index.vptr()[i] != person_id(person_id::value_base_t(i))) {
						person.m__index.vptr()[i] = person.first_free;
						person.first_free = person_id(person_id::value_base_t(i));
					}
				}
			}
			person.size_used = new_size;
		}
		
		//
		// container resize for car_ownership
		//
		void car_ownership_resize(uint32_t new_size) {
			#ifndef DCON_USE_EXCEPTIONS
			if(new_size > 1200) std::abort();
			#else
			if(new_size > 1200) throw dcon::out_of_space{};
			#endif
			const uint32_t old_size = car.size_used;
			if(new_size < old_size) {
				std::fill_n(car_ownership.m_owner.vptr() + 0, old_size, person_id{});
				std::for_each(car_ownership.m_array_owner.vptr() + 0, car_ownership.m_array_owner.vptr() + 0 + person.size_used, [t = this](dcon::stable_mk_2_tag& i){ t->car_ownership.owner_storage.release(i); });
				std::fill_n(car_ownership.m_ownership_date.vptr() + new_size, old_size - new_size, int32_t{});
			} else if(new_size > old_size) {
			}
		}
		
		//
		// container delete for car_ownership
		//
		void delete_car_ownership(car_ownership_id id_removed) {
			#ifndef NDEBUG
			assert(id_removed.index() >= 0);
			#endif
			internal_car_ownership_set_owner(id_removed, person_id());
			car_ownership.m_ownership_date.vptr()[id_removed.index()] = int32_t{};
		}
		
		//
		// container pop_back for car_ownership
		//
		void pop_back_car_ownership() {
			if(car.size_used == 0) return;
			car_ownership_id id_removed(car_ownership_id::value_base_t(car.size_used - 1));
			internal_car_ownership_set_owner(id_removed, person_id());
			car_ownership.m_ownership_date.vptr()[id_removed.index()] = int32_t{};
		}
		
		private:
		//
		// container move relationship for car_ownership
		//
		void internal_move_relationship_car_ownership(car_ownership_id last_id, car_ownership_id id_removed) {
			internal_car_ownership_set_owner(id_removed, person_id());
			if(auto tmp = car_ownership.m_owner.vptr()[last_id.index()]; bool(tmp)) {
				dcon::replace_unique_item(car_ownership.owner_storage, car_ownership.m_array_owner.vptr()[tmp.index()], last_id, id_removed);
			}
			car_ownership.m_owner.vptr()[id_removed.index()] = std::move(car_ownership.m_owner.vptr()[last_id.index()]);
			car_ownership.m_owner.vptr()[last_id.index()] = person_id();
			car_ownership.m_ownership_date.vptr()[id_removed.index()] = std::move(car_ownership.m_ownership_date.vptr()[last_id.index()]);
			car_ownership.m_ownership_date.vptr()[last_id.index()] = int32_t{};
		}
		
		public:
		//
		// container try create relationship for car_ownership
		//
		car_ownership_id try_create_car_ownership(person_id owner_p, car_id owned_car_p) {
			if(!bool(owner_p)) return car_ownership_id();
			if(!bool(owned_car_p)) return car_ownership_id();
			if(car_ownership_is_valid(car_ownership_id(car_ownership_id::value_base_t(owned_car_p.index())))) return car_ownership_id();
			car_ownership_id new_id(car_ownership_id::value_base_t(owned_car_p.index()));
			if(car.size_used < uint32_t(owned_car_p.value)) car_resize(uint32_t(owned_car_p.value));
			internal_car_ownership_set_owner(new_id, owner_p);
			return new_id;
		}
		
		//
		// container force create relationship for car_ownership
		//
		car_ownership_id force_create_car_ownership(person_id owner_p, car_id owned_car_p) {
			car_ownership_id new_id(car_ownership_id::value_base_t(owned_car_p.index()));
			if(car.size_used < uint32_t(owned_car_p.value)) car_resize(uint32_t(owned_car_p.value));
			internal_car_ownership_set_owner(new_id, owner_p);
			return new_id;
		}
		
		template <typename T>
		DCON_RELEASE_INLINE void for_each_thingy(T&& func) {
			for(uint32_t i = 0; i < thingy.size_used; ++i) {
				thingy_id tmp = thingy_id(thingy_id::value_base_t(i));
				func(tmp);
			}
		}
		friend internal::const_object_iterator_thingy;
		friend internal::object_iterator_thingy;
		struct {
			internal::object_iterator_thingy begin() {
				data_container* container = reinterpret_cast<data_container*>(reinterpret_cast<std::byte*>(this) - offsetof(data_container, in_thingy));
				return internal::object_iterator_thingy(*container, uint32_t(0));
			}
			internal::object_iterator_thingy end() {
				data_container* container = reinterpret_cast<data_container*>(reinterpret_cast<std::byte*>(this) - offsetof(data_container, in_thingy));
				return internal::object_iterator_thingy(*container, container->thingy_size());
			}
			internal::const_object_iterator_thingy begin() const {
				data_container const* container = reinterpret_cast<data_container const*>(reinterpret_cast<std::byte const*>(this) - offsetof(data_container, in_thingy));
				return internal::const_object_iterator_thingy(*container, uint32_t(0));
			}
			internal::const_object_iterator_thingy end() const {
				data_container const* container = reinterpret_cast<data_container const*>(reinterpret_cast<std::byte const*>(this) - offsetof(data_container, in_thingy));
				return internal::const_object_iterator_thingy(*container, container->thingy_size());
			}
		}  in_thingy ;
		
		template <typename T>
		DCON_RELEASE_INLINE void for_each_car(T&& func) {
			for(uint32_t i = 0; i < car.size_used; ++i) {
				car_id tmp = car_id(car_id::value_base_t(i));
				func(tmp);
			}
		}
		friend internal::const_object_iterator_car;
		friend internal::object_iterator_car;
		struct {
			internal::object_iterator_car begin() {
				data_container* container = reinterpret_cast<data_container*>(reinterpret_cast<std::byte*>(this) - offsetof(data_container, in_car));
				return internal::object_iterator_car(*container, uint32_t(0));
			}
			internal::object_iterator_car end() {
				data_container* container = reinterpret_cast<data_container*>(reinterpret_cast<std::byte*>(this) - offsetof(data_container, in_car));
				return internal::object_iterator_car(*container, container->car_size());
			}
			internal::const_object_iterator_car begin() const {
				data_container const* container = reinterpret_cast<data_container const*>(reinterpret_cast<std::byte const*>(this) - offsetof(data_container, in_car));
				return internal::const_object_iterator_car(*container, uint32_t(0));
			}
			internal::const_object_iterator_car end() const {
				data_container const* container = reinterpret_cast<data_container const*>(reinterpret_cast<std::byte const*>(this) - offsetof(data_container, in_car));
				return internal::const_object_iterator_car(*container, container->car_size());
			}
		}  in_car ;
		
		template <typename T>
		DCON_RELEASE_INLINE void for_each_person(T&& func) {
			for(uint32_t i = 0; i < person.size_used; ++i) {
				person_id tmp = person_id(person_id::value_base_t(i));
				if(person.m__index.vptr()[tmp.index()] == tmp) func(tmp);
			}
		}
		friend internal::const_object_iterator_person;
		friend internal::object_iterator_person;
		struct {
			internal::object_iterator_person begin() {
				data_container* container = reinterpret_cast<data_container*>(reinterpret_cast<std::byte*>(this) - offsetof(data_container, in_person));
				return internal::object_iterator_person(*container, uint32_t(0));
			}
			internal::object_iterator_person end() {
				data_container* container = reinterpret_cast<data_container*>(reinterpret_cast<std::byte*>(this) - offsetof(data_container, in_person));
				return internal::object_iterator_person(*container, container->person_size());
			}
			internal::const_object_iterator_person begin() const {
				data_container const* container = reinterpret_cast<data_container const*>(reinterpret_cast<std::byte const*>(this) - offsetof(data_container, in_person));
				return internal::const_object_iterator_person(*container, uint32_t(0));
			}
			internal::const_object_iterator_person end() const {
				data_container const* container = reinterpret_cast<data_container const*>(reinterpret_cast<std::byte const*>(this) - offsetof(data_container, in_person));
				return internal::const_object_iterator_person(*container, container->person_size());
			}
		}  in_person ;
		
		template <typename T>
		DCON_RELEASE_INLINE void for_each_car_ownership(T&& func) {
			for(uint32_t i = 0; i < car.size_used; ++i) {
				car_ownership_id tmp = car_ownership_id(car_ownership_id::value_base_t(i));
				func(tmp);
			}
		}
		friend internal::const_object_iterator_car_ownership;
		friend internal::object_iterator_car_ownership;
		struct {
			internal::object_iterator_car_ownership begin() {
				data_container* container = reinterpret_cast<data_container*>(reinterpret_cast<std::byte*>(this) - offsetof(data_container, in_car_ownership));
				return internal::object_iterator_car_ownership(*container, uint32_t(0));
			}
			internal::object_iterator_car_ownership end() {
				data_container* container = reinterpret_cast<data_container*>(reinterpret_cast<std::byte*>(this) - offsetof(data_container, in_car_ownership));
				return internal::object_iterator_car_ownership(*container, container->car_ownership_size());
			}
			internal::const_object_iterator_car_ownership begin() const {
				data_container const* container = reinterpret_cast<data_container const*>(reinterpret_cast<std::byte const*>(this) - offsetof(data_container, in_car_ownership));
				return internal::const_object_iterator_car_ownership(*container, uint32_t(0));
			}
			internal::const_object_iterator_car_ownership end() const {
				data_container const* container = reinterpret_cast<data_container const*>(reinterpret_cast<std::byte const*>(this) - offsetof(data_container, in_car_ownership));
				return internal::const_object_iterator_car_ownership(*container, container->car_ownership_size());
			}
		}  in_car_ownership ;
		


		uint64_t serialize_size(std::vector<float> const& obj) const;
		void serialize(std::byte*& output_buffer, std::vector<float> const& obj) const;
		void deserialize(std::byte const*& input_buffer, std::vector<float> & obj, std::byte const* end) const;

		void reset() {
			car_ownership_resize(0);
			thingy_resize(0);
			car_resize(0);
			person_resize(0);
		}

		#ifndef DCON_NO_VE
		ve::vectorizable_buffer<float, thingy_id> thingy_make_vectorizable_float_buffer() const noexcept {
			return ve::vectorizable_buffer<float, thingy_id>(thingy.size_used);
		}
		ve::vectorizable_buffer<int32_t, thingy_id> thingy_make_vectorizable_int_buffer() const noexcept {
			return ve::vectorizable_buffer<int32_t, thingy_id>(thingy.size_used);
		}
		template<typename F>
		DCON_RELEASE_INLINE void execute_serial_over_thingy(F&& functor) {
			ve::execute_serial<thingy_id>(thingy.size_used, functor);
		}
#ifndef VE_NO_TBB
		template<typename F>
		DCON_RELEASE_INLINE void execute_parallel_over_thingy(F&& functor) {
			ve::execute_parallel_exact<thingy_id>(thingy.size_used, functor);
		}
#endif
		ve::vectorizable_buffer<float, car_id> car_make_vectorizable_float_buffer() const noexcept {
			return ve::vectorizable_buffer<float, car_id>(car.size_used);
		}
		ve::vectorizable_buffer<int32_t, car_id> car_make_vectorizable_int_buffer() const noexcept {
			return ve::vectorizable_buffer<int32_t, car_id>(car.size_used);
		}
		template<typename F>
		DCON_RELEASE_INLINE void execute_serial_over_car(F&& functor) {
			ve::execute_serial<car_id>(car.size_used, functor);
		}
#ifndef VE_NO_TBB
		template<typename F>
		DCON_RELEASE_INLINE void execute_parallel_over_car(F&& functor) {
			ve::execute_parallel_exact<car_id>(car.size_used, functor);
		}
#endif
		ve::vectorizable_buffer<float, person_id> person_make_vectorizable_float_buffer() const noexcept {
			return ve::vectorizable_buffer<float, person_id>(person.size_used);
		}
		ve::vectorizable_buffer<int32_t, person_id> person_make_vectorizable_int_buffer() const noexcept {
			return ve::vectorizable_buffer<int32_t, person_id>(person.size_used);
		}
		template<typename F>
		DCON_RELEASE_INLINE void execute_serial_over_person(F&& functor) {
			ve::execute_serial<person_id>(person.size_used, functor);
		}
#ifndef VE_NO_TBB
		template<typename F>
		DCON_RELEASE_INLINE void execute_parallel_over_person(F&& functor) {
			ve::execute_parallel_exact<person_id>(person.size_used, functor);
		}
#endif
		ve::vectorizable_buffer<float, car_ownership_id> car_ownership_make_vectorizable_float_buffer() const noexcept {
			return ve::vectorizable_buffer<float, car_ownership_id>(car.size_used);
		}
		ve::vectorizable_buffer<int32_t, car_ownership_id> car_ownership_make_vectorizable_int_buffer() const noexcept {
			return ve::vectorizable_buffer<int32_t, car_ownership_id>(car.size_used);
		}
		template<typename F>
		DCON_RELEASE_INLINE void execute_serial_over_car_ownership(F&& functor) {
			ve::execute_serial<car_ownership_id>(car.size_used, functor);
		}
#ifndef VE_NO_TBB
		template<typename F>
		DCON_RELEASE_INLINE void execute_parallel_over_car_ownership(F&& functor) {
			ve::execute_parallel_exact<car_ownership_id>(car.size_used, functor);
		}
#endif
		#endif

		load_record serialize_entire_container_record() const noexcept {
			load_record result;
			result.thingy = true;
			result.thingy_some_value = true;
			result.thingy_bf_value = true;
			result.thingy_obj_value = true;
			result.thingy_pooled_v = true;
			result.thingy_big_array = true;
			result.thingy_big_array_bf = true;
			result.car = true;
			result.car_wheels = true;
			result.car_resale_value = true;
			result.person = true;
			result.person__index = true;
			result.person_age = true;
			result.car_ownership = true;
			result.car_ownership_owner = true;
			result.car_ownership_owned_car = true;
			result.car_ownership_ownership_date = true;
			return result;
		}
		
		//
		// calculate size (in bytes) required to serialize the desired objects, relationships, and properties
		//
		uint64_t serialize_size(load_record const& serialize_selection) const {
			uint64_t total_size = 0;
			if(serialize_selection.thingy) {
				dcon::record_header header(0, "uint32_t", "thingy", "$size");
				total_size += header.serialize_size();
				total_size += sizeof(uint32_t);
			}
			if(serialize_selection.thingy_some_value) {
				dcon::record_header iheader(0, "int32_t", "thingy", "some_value");
				total_size += iheader.serialize_size();
				total_size += sizeof(int32_t) * thingy.size_used;
			}
			if(serialize_selection.thingy_bf_value) {
				dcon::record_header iheader(0, "bitfield", "thingy", "bf_value");
				total_size += iheader.serialize_size();
				total_size += (thingy.size_used + 7) / 8;
			}
			if(serialize_selection.thingy_obj_value) {
				std::for_each(thingy.m_obj_value.vptr(), thingy.m_obj_value.vptr() + thingy.size_used, [t = this, &total_size](std::vector<float> const& obj){ total_size += t->serialize_size(obj); });
				dcon::record_header iheader(0, "std::vector<float>", "thingy", "obj_value");
				total_size += iheader.serialize_size();
			}
			if(serialize_selection.thingy_pooled_v) {
				std::for_each(thingy.m_pooled_v.vptr(), thingy.m_pooled_v.vptr() + thingy.size_used, [t = this, &total_size](dcon::stable_mk_2_tag obj) {
					auto rng = dcon::get_range(t->thingy.pooled_v_storage, obj);
					total_size += sizeof(uint16_t);
					total_size += sizeof(int16_t) * (rng.second - rng.first);
				} );
				 {
					total_size += 8;
					dcon::record_header iheader(0, "stable_mk_2_tag", "thingy", "pooled_v");
					total_size += iheader.serialize_size();
				}
			}
			if(serialize_selection.thingy_big_array) {
				total_size += 6;
				total_size += sizeof(uint16_t);
				total_size += thingy.m_big_array.size * sizeof(float) * thingy.size_used;
				dcon::record_header iheader(0, "$array", "thingy", "big_array");
				total_size += iheader.serialize_size();
			}
			if(serialize_selection.thingy_big_array_bf) {
				total_size += 9;
				total_size += sizeof(uint16_t);
				total_size += thingy.m_big_array_bf.size * ((thingy.size_used + 7) / 8);
				dcon::record_header iheader(0, "$array", "thingy", "big_array_bf");
				total_size += iheader.serialize_size();
			}
			if(serialize_selection.car) {
				dcon::record_header header(0, "uint32_t", "car", "$size");
				total_size += header.serialize_size();
				total_size += sizeof(uint32_t);
			}
			if(serialize_selection.car_wheels) {
				dcon::record_header iheader(0, "int32_t", "car", "wheels");
				total_size += iheader.serialize_size();
				total_size += sizeof(int32_t) * car.size_used;
			}
			if(serialize_selection.car_resale_value) {
				dcon::record_header iheader(0, "float", "car", "resale_value");
				total_size += iheader.serialize_size();
				total_size += sizeof(float) * car.size_used;
			}
			if(serialize_selection.person) {
				dcon::record_header header(0, "uint32_t", "person", "$size");
				total_size += header.serialize_size();
				total_size += sizeof(uint32_t);
			}
			if(serialize_selection.person__index) {
				dcon::record_header iheader(0, "uint8_t", "person", "_index");
				total_size += iheader.serialize_size();
				total_size += sizeof(person_id) * person.size_used;
			}
			if(serialize_selection.person_age) {
				dcon::record_header iheader(0, "int32_t", "person", "age");
				total_size += iheader.serialize_size();
				total_size += sizeof(int32_t) * person.size_used;
			}
			if(serialize_selection.car_ownership) {
				dcon::record_header header(0, "uint32_t", "car_ownership", "$size");
				total_size += header.serialize_size();
				total_size += sizeof(uint32_t);
				if(serialize_selection.car_ownership_owner) {
					dcon::record_header iheader(0, "uint8_t", "car_ownership", "owner");
					total_size += iheader.serialize_size();
					total_size += sizeof(person_id) * car.size_used;
				}
				dcon::record_header headerb(0, "$", "car_ownership", "$index_end");
				total_size += headerb.serialize_size();
			}
			if(serialize_selection.car_ownership_ownership_date) {
				dcon::record_header iheader(0, "int32_t", "car_ownership", "ownership_date");
				total_size += iheader.serialize_size();
				total_size += sizeof(int32_t) * car.size_used;
			}
			return total_size;
		}
		
		//
		// serialize the desired objects, relationships, and properties
		//
		void serialize(std::byte*& output_buffer, load_record const& serialize_selection) const {
			if(serialize_selection.thingy) {
				dcon::record_header header(sizeof(uint32_t), "uint32_t", "thingy", "$size");
				header.serialize(output_buffer);
				*(reinterpret_cast<uint32_t*>(output_buffer)) = thingy.size_used;
				output_buffer += sizeof(uint32_t);
			}
			if(serialize_selection.thingy_some_value) {
				dcon::record_header header(sizeof(int32_t) * thingy.size_used, "int32_t", "thingy", "some_value");
				header.serialize(output_buffer);
				std::memcpy(reinterpret_cast<int32_t*>(output_buffer), thingy.m_some_value.vptr(), sizeof(int32_t) * thingy.size_used);
				output_buffer += sizeof(int32_t) * thingy.size_used;
			}
			if(serialize_selection.thingy_bf_value) {
				dcon::record_header header((thingy.size_used + 7) / 8, "bitfield", "thingy", "bf_value");
				header.serialize(output_buffer);
				std::memcpy(reinterpret_cast<dcon::bitfield_type*>(output_buffer), thingy.m_bf_value.vptr(), (thingy.size_used + 7) / 8);
				output_buffer += (thingy.size_used + 7) / 8;
			}
			if(serialize_selection.thingy_obj_value) {
				size_t total_size = 0;
				std::for_each(thingy.m_obj_value.vptr(), thingy.m_obj_value.vptr() + thingy.size_used, [t = this, &total_size](std::vector<float> const& obj) {
					total_size += t->serialize_size(obj);
				} );
				dcon::record_header header(total_size, "std::vector<float>", "thingy", "obj_value");
				header.serialize(output_buffer);
				std::for_each(thingy.m_obj_value.vptr(), thingy.m_obj_value.vptr() + thingy.size_used, [t = this, &output_buffer](std::vector<float> const& obj){ t->serialize(output_buffer, obj); });
			}
			if(serialize_selection.thingy_pooled_v) {
				size_t total_size = 0;
				std::for_each(thingy.m_pooled_v.vptr(), thingy.m_pooled_v.vptr() + thingy.size_used, [t = this, &total_size](dcon::stable_mk_2_tag obj) {
					auto rng = dcon::get_range(t->thingy.pooled_v_storage, obj);
					total_size += sizeof(uint16_t) + sizeof(int16_t) * (rng.second - rng.first);
				} );
				total_size += 8;
				dcon::record_header header(total_size, "stable_mk_2_tag", "thingy", "pooled_v");
				header.serialize(output_buffer);
				std::memcpy(reinterpret_cast<char*>(output_buffer), "int16_t", 8);
				output_buffer += 8;
				std::for_each(thingy.m_pooled_v.vptr(), thingy.m_pooled_v.vptr() + thingy.size_used, [t = this, &output_buffer](dcon::stable_mk_2_tag obj) {
					auto rng = dcon::get_range(t->thingy.pooled_v_storage, obj);
					*(reinterpret_cast<uint16_t*>(output_buffer)) = uint16_t(rng.second - rng.first);
					output_buffer += sizeof(uint16_t);
					std::memcpy(reinterpret_cast<int16_t*>(output_buffer), rng.first, sizeof(int16_t) * (rng.second - rng.first));
					output_buffer += sizeof(int16_t) * (rng.second - rng.first);
				} );
			}
			if(serialize_selection.thingy_big_array) {
				dcon::record_header header(6 + sizeof(uint16_t) + sizeof(float) * thingy.m_big_array.size * thingy.size_used, "$array", "thingy", "big_array");
				header.serialize(output_buffer);
				std::memcpy(reinterpret_cast<char*>(output_buffer), "float", 6);
				output_buffer += 6;
				*(reinterpret_cast<uint16_t*>(output_buffer)) = uint16_t(thingy.m_big_array.size);
				output_buffer += sizeof(uint16_t);
				for(int32_t s = 0; s < int32_t(thingy.m_big_array.size); ++s) {
					std::memcpy(reinterpret_cast<float*>(output_buffer), thingy.m_big_array.vptr(s), sizeof(float) * thingy.size_used);
					output_buffer +=  sizeof(float) * thingy.size_used;
				}
			}
			if(serialize_selection.thingy_big_array_bf) {
				dcon::record_header header(9 + sizeof(uint16_t) + thingy.m_big_array_bf.size * ((thingy.size_used + 7) / 8), "$array", "thingy", "big_array_bf");
				header.serialize(output_buffer);
				std::memcpy(reinterpret_cast<char*>(output_buffer), "bitfield", 9);
				output_buffer += 9;
				*(reinterpret_cast<uint16_t*>(output_buffer)) = uint16_t(thingy.m_big_array_bf.size);
				output_buffer += sizeof(uint16_t);
				for(int32_t s = 0; s < int32_t(thingy.m_big_array_bf.size); ++s) {
					std::memcpy(reinterpret_cast<dcon::bitfield_type*>(output_buffer), thingy.m_big_array_bf.vptr(s), (thingy.size_used + 7) / 8);
					output_buffer += (thingy.size_used + 7) / 8;
				}
			}
			if(serialize_selection.car) {
				dcon::record_header header(sizeof(uint32_t), "uint32_t", "car", "$size");
				header.serialize(output_buffer);
				*(reinterpret_cast<uint32_t*>(output_buffer)) = car.size_used;
				output_buffer += sizeof(uint32_t);
			}
			if(serialize_selection.car_wheels) {
				dcon::record_header header(sizeof(int32_t) * car.size_used, "int32_t", "car", "wheels");
				header.serialize(output_buffer);
				std::memcpy(reinterpret_cast<int32_t*>(output_buffer), car.m_wheels.vptr(), sizeof(int32_t) * car.size_used);
				output_buffer += sizeof(int32_t) * car.size_used;
			}
			if(serialize_selection.car_resale_value) {
				dcon::record_header header(sizeof(float) * car.size_used, "float", "car", "resale_value");
				header.serialize(output_buffer);
				std::memcpy(reinterpret_cast<float*>(output_buffer), car.m_resale_value.vptr(), sizeof(float) * car.size_used);
				output_buffer += sizeof(float) * car.size_used;
			}
			if(serialize_selection.person) {
				dcon::record_header header(sizeof(uint32_t), "uint32_t", "person", "$size");
				header.serialize(output_buffer);
				*(reinterpret_cast<uint32_t*>(output_buffer)) = person.size_used;
				output_buffer += sizeof(uint32_t);
			}
			if(serialize_selection.person__index) {
				dcon::record_header header(sizeof(person_id) * person.size_used, "uint8_t", "person", "_index");
				header.serialize(output_buffer);
				std::memcpy(reinterpret_cast<person_id*>(output_buffer), person.m__index.vptr(), sizeof(person_id) * person.size_used);
				output_buffer += sizeof(person_id) * person.size_used;
			}
			if(serialize_selection.person_age) {
				dcon::record_header header(sizeof(int32_t) * person.size_used, "int32_t", "person", "age");
				header.serialize(output_buffer);
				std::memcpy(reinterpret_cast<int32_t*>(output_buffer), person.m_age.vptr(), sizeof(int32_t) * person.size_used);
				output_buffer += sizeof(int32_t) * person.size_used;
			}
			if(serialize_selection.car_ownership) {
				dcon::record_header header(sizeof(uint32_t), "uint32_t", "car_ownership", "$size");
				header.serialize(output_buffer);
				*(reinterpret_cast<uint32_t*>(output_buffer)) = car.size_used;
				output_buffer += sizeof(uint32_t);
				 {
					dcon::record_header iheader(sizeof(person_id) * car.size_used, "uint8_t", "car_ownership", "owner");
					iheader.serialize(output_buffer);
					std::memcpy(reinterpret_cast<person_id*>(output_buffer), car_ownership.m_owner.vptr(), sizeof(person_id) * car.size_used);
					output_buffer += sizeof(person_id) *  car.size_used;
				}
				dcon::record_header headerb(0, "$", "car_ownership", "$index_end");
				headerb.serialize(output_buffer);
			}
			if(serialize_selection.car_ownership_ownership_date) {
				dcon::record_header header(sizeof(int32_t) * car.size_used, "int32_t", "car_ownership", "ownership_date");
				header.serialize(output_buffer);
				std::memcpy(reinterpret_cast<int32_t*>(output_buffer), car_ownership.m_ownership_date.vptr(), sizeof(int32_t) * car.size_used);
				output_buffer += sizeof(int32_t) * car.size_used;
			}
		}
		
		//
		// deserialize the desired objects, relationships, and properties
		//
		void deserialize(std::byte const*& input_buffer, std::byte const* end, load_record& serialize_selection) {
			while(input_buffer < end) {
				dcon::record_header header;
				header.deserialize(input_buffer, end);
				if(input_buffer + header.record_size <= end) {
					do {
						if(header.is_object("thingy")) {
							do {
								if(header.is_property("$size") && header.record_size == sizeof(uint32_t)) {
									thingy_resize(*(reinterpret_cast<uint32_t const*>(input_buffer)));
									serialize_selection.thingy = true;
									break;
								}
								if(header.is_property("some_value")) {
									if(header.is_type("int32_t")) {
										std::memcpy(thingy.m_some_value.vptr(), reinterpret_cast<int32_t const*>(input_buffer), std::min(size_t(thingy.size_used) * sizeof(int32_t), size_t(header.record_size)));
										serialize_selection.thingy_some_value = true;
									}
									else if(header.is_type("int8_t")) {
										for(uint32_t i = 0; i < std::min(thingy.size_used, uint32_t(header.record_size / sizeof(int8_t))); ++i) {
											thingy.m_some_value.vptr()[i] = int32_t(*(reinterpret_cast<int8_t const*>(input_buffer) + i));
										}
										serialize_selection.thingy_some_value = true;
									}
									else if(header.is_type("uint8_t")) {
										for(uint32_t i = 0; i < std::min(thingy.size_used, uint32_t(header.record_size / sizeof(uint8_t))); ++i) {
											thingy.m_some_value.vptr()[i] = int32_t(*(reinterpret_cast<uint8_t const*>(input_buffer) + i));
										}
										serialize_selection.thingy_some_value = true;
									}
									else if(header.is_type("int16_t")) {
										for(uint32_t i = 0; i < std::min(thingy.size_used, uint32_t(header.record_size / sizeof(int16_t))); ++i) {
											thingy.m_some_value.vptr()[i] = int32_t(*(reinterpret_cast<int16_t const*>(input_buffer) + i));
										}
										serialize_selection.thingy_some_value = true;
									}
									else if(header.is_type("uint16_t")) {
										for(uint32_t i = 0; i < std::min(thingy.size_used, uint32_t(header.record_size / sizeof(uint16_t))); ++i) {
											thingy.m_some_value.vptr()[i] = int32_t(*(reinterpret_cast<uint16_t const*>(input_buffer) + i));
										}
										serialize_selection.thingy_some_value = true;
									}
									else if(header.is_type("uint32_t")) {
										for(uint32_t i = 0; i < std::min(thingy.size_used, uint32_t(header.record_size / sizeof(uint32_t))); ++i) {
											thingy.m_some_value.vptr()[i] = int32_t(*(reinterpret_cast<uint32_t const*>(input_buffer) + i));
										}
										serialize_selection.thingy_some_value = true;
									}
									else if(header.is_type("int64_t")) {
										for(uint32_t i = 0; i < std::min(thingy.size_used, uint32_t(header.record_size / sizeof(int64_t))); ++i) {
											thingy.m_some_value.vptr()[i] = int32_t(*(reinterpret_cast<int64_t const*>(input_buffer) + i));
										}
										serialize_selection.thingy_some_value = true;
									}
									else if(header.is_type("uint64_t")) {
										for(uint32_t i = 0; i < std::min(thingy.size_used, uint32_t(header.record_size / sizeof(uint64_t))); ++i) {
											thingy.m_some_value.vptr()[i] = int32_t(*(reinterpret_cast<uint64_t const*>(input_buffer) + i));
										}
										serialize_selection.thingy_some_value = true;
									}
									else if(header.is_type("float")) {
										for(uint32_t i = 0; i < std::min(thingy.size_used, uint32_t(header.record_size / sizeof(float))); ++i) {
											thingy.m_some_value.vptr()[i] = int32_t(*(reinterpret_cast<float const*>(input_buffer) + i));
										}
										serialize_selection.thingy_some_value = true;
									}
									else if(header.is_type("double")) {
										for(uint32_t i = 0; i < std::min(thingy.size_used, uint32_t(header.record_size / sizeof(double))); ++i) {
											thingy.m_some_value.vptr()[i] = int32_t(*(reinterpret_cast<double const*>(input_buffer) + i));
										}
										serialize_selection.thingy_some_value = true;
									}
									break;
								}
								if(header.is_property("bf_value")) {
									if(header.is_type("bitfield")) {
										std::memcpy(thingy.m_bf_value.vptr(), reinterpret_cast<dcon::bitfield_type const*>(input_buffer), std::min(size_t(thingy.size_used + 7) / 8, size_t(header.record_size)));
										serialize_selection.thingy_bf_value = true;
									}
									break;
								}
								if(header.is_property("obj_value")) {
									if(header.is_type("std::vector<float>")) {
										std::byte const* icpy = input_buffer;
										for(uint32_t i = 0; icpy < input_buffer + header.record_size && i < thingy.size_used; ++i) {
											deserialize(icpy, thingy.m_obj_value.vptr()[i], input_buffer + header.record_size);
										}
										serialize_selection.thingy_obj_value = true;
									}
									break;
								}
								if(header.is_property("pooled_v")) {
									if(header.is_type("stable_mk_2_tag")) {
										uint32_t ix = 0;
										std::byte const* zero_pos = std::find(input_buffer, input_buffer + header.record_size, std::byte(0));
										if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "int16_t")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(int16_t) ));
													icpy += sizeof(uint16_t);
												}
												dcon::load_range(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], reinterpret_cast<int16_t const*>(icpy), reinterpret_cast<int16_t const*>(icpy) + sz);
												icpy += sz * sizeof(int16_t);
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "int8_t")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(int8_t) ));
													icpy += sizeof(uint16_t);
												}
												dcon::resize(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], sz);
												for(uint32_t ii = 0; ii < sz && icpy < input_buffer + header.record_size; ++ii) {
													dcon::get(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], ii) = int16_t(*(reinterpret_cast<int8_t const*>(icpy)));
													icpy += sizeof(int8_t);
												}
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "uint8_t")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(uint8_t) ));
													icpy += sizeof(uint16_t);
												}
												dcon::resize(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], sz);
												for(uint32_t ii = 0; ii < sz && icpy < input_buffer + header.record_size; ++ii) {
													dcon::get(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], ii) = int16_t(*(reinterpret_cast<uint8_t const*>(icpy)));
													icpy += sizeof(uint8_t);
												}
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "uint16_t")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(uint16_t) ));
													icpy += sizeof(uint16_t);
												}
												dcon::resize(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], sz);
												for(uint32_t ii = 0; ii < sz && icpy < input_buffer + header.record_size; ++ii) {
													dcon::get(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], ii) = int16_t(*(reinterpret_cast<uint16_t const*>(icpy)));
													icpy += sizeof(uint16_t);
												}
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "int32_t")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(int32_t) ));
													icpy += sizeof(uint16_t);
												}
												dcon::resize(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], sz);
												for(uint32_t ii = 0; ii < sz && icpy < input_buffer + header.record_size; ++ii) {
													dcon::get(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], ii) = int16_t(*(reinterpret_cast<int32_t const*>(icpy)));
													icpy += sizeof(int32_t);
												}
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "uint32_t")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(uint32_t) ));
													icpy += sizeof(uint16_t);
												}
												dcon::resize(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], sz);
												for(uint32_t ii = 0; ii < sz && icpy < input_buffer + header.record_size; ++ii) {
													dcon::get(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], ii) = int16_t(*(reinterpret_cast<uint32_t const*>(icpy)));
													icpy += sizeof(uint32_t);
												}
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "int64_t")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(int64_t) ));
													icpy += sizeof(uint16_t);
												}
												dcon::resize(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], sz);
												for(uint32_t ii = 0; ii < sz && icpy < input_buffer + header.record_size; ++ii) {
													dcon::get(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], ii) = int16_t(*(reinterpret_cast<int64_t const*>(icpy)));
													icpy += sizeof(int64_t);
												}
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "uint64_t")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(uint64_t) ));
													icpy += sizeof(uint16_t);
												}
												dcon::resize(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], sz);
												for(uint32_t ii = 0; ii < sz && icpy < input_buffer + header.record_size; ++ii) {
													dcon::get(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], ii) = int16_t(*(reinterpret_cast<uint64_t const*>(icpy)));
													icpy += sizeof(uint64_t);
												}
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "float")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(float) ));
													icpy += sizeof(uint16_t);
												}
												dcon::resize(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], sz);
												for(uint32_t ii = 0; ii < sz && icpy < input_buffer + header.record_size; ++ii) {
													dcon::get(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], ii) = int16_t(*(reinterpret_cast<float const*>(icpy)));
													icpy += sizeof(float);
												}
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "double")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(double) ));
													icpy += sizeof(uint16_t);
												}
												dcon::resize(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], sz);
												for(uint32_t ii = 0; ii < sz && icpy < input_buffer + header.record_size; ++ii) {
													dcon::get(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], ii) = int16_t(*(reinterpret_cast<double const*>(icpy)));
													icpy += sizeof(double);
												}
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
									}
									break;
								}
								if(header.is_property("big_array")) {
									if(header.is_type("$array")) {
										std::byte const* zero_pos = std::find(input_buffer, input_buffer + header.record_size, std::byte(0));
										std::byte const* icpy = zero_pos + 1;
										if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "float")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												std::memcpy(thingy.m_big_array.vptr(s), reinterpret_cast<float const*>(icpy), std::min(sizeof(float) * thingy.size_used, size_t(input_buffer + header.record_size - icpy)));
												icpy += sizeof(float) * thingy.size_used;
											}
											serialize_selection.thingy_big_array = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "int8_t")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												for(uint32_t j = 0; j < thingy.size_used && icpy < input_buffer + header.record_size; ++j) {
													thingy.m_big_array.vptr(s)[j] = float(*(reinterpret_cast<int8_t const*>(icpy)));
													icpy += sizeof(int8_t);
												}
											}
											serialize_selection.thingy_big_array = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "uint8_t")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												for(uint32_t j = 0; j < thingy.size_used && icpy < input_buffer + header.record_size; ++j) {
													thingy.m_big_array.vptr(s)[j] = float(*(reinterpret_cast<uint8_t const*>(icpy)));
													icpy += sizeof(uint8_t);
												}
											}
											serialize_selection.thingy_big_array = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "int16_t")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												for(uint32_t j = 0; j < thingy.size_used && icpy < input_buffer + header.record_size; ++j) {
													thingy.m_big_array.vptr(s)[j] = float(*(reinterpret_cast<int16_t const*>(icpy)));
													icpy += sizeof(int16_t);
												}
											}
											serialize_selection.thingy_big_array = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "uint16_t")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												for(uint32_t j = 0; j < thingy.size_used && icpy < input_buffer + header.record_size; ++j) {
													thingy.m_big_array.vptr(s)[j] = float(*(reinterpret_cast<uint16_t const*>(icpy)));
													icpy += sizeof(uint16_t);
												}
											}
											serialize_selection.thingy_big_array = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "int32_t")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												for(uint32_t j = 0; j < thingy.size_used && icpy < input_buffer + header.record_size; ++j) {
													thingy.m_big_array.vptr(s)[j] = float(*(reinterpret_cast<int32_t const*>(icpy)));
													icpy += sizeof(int32_t);
												}
											}
											serialize_selection.thingy_big_array = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "uint32_t")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												for(uint32_t j = 0; j < thingy.size_used && icpy < input_buffer + header.record_size; ++j) {
													thingy.m_big_array.vptr(s)[j] = float(*(reinterpret_cast<uint32_t const*>(icpy)));
													icpy += sizeof(uint32_t);
												}
											}
											serialize_selection.thingy_big_array = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "int64_t")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												for(uint32_t j = 0; j < thingy.size_used && icpy < input_buffer + header.record_size; ++j) {
													thingy.m_big_array.vptr(s)[j] = float(*(reinterpret_cast<int64_t const*>(icpy)));
													icpy += sizeof(int64_t);
												}
											}
											serialize_selection.thingy_big_array = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "uint64_t")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												for(uint32_t j = 0; j < thingy.size_used && icpy < input_buffer + header.record_size; ++j) {
													thingy.m_big_array.vptr(s)[j] = float(*(reinterpret_cast<uint64_t const*>(icpy)));
													icpy += sizeof(uint64_t);
												}
											}
											serialize_selection.thingy_big_array = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "double")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												for(uint32_t j = 0; j < thingy.size_used && icpy < input_buffer + header.record_size; ++j) {
													thingy.m_big_array.vptr(s)[j] = float(*(reinterpret_cast<double const*>(icpy)));
													icpy += sizeof(double);
												}
											}
											serialize_selection.thingy_big_array = true;
										}
									}
									break;
								}
								if(header.is_property("big_array_bf")) {
									if(header.is_type("$array")) {
										std::byte const* zero_pos = std::find(input_buffer, input_buffer + header.record_size, std::byte(0));
										std::byte const* icpy = zero_pos + 1;
										if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "bitfield")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array_bf.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array_bf.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array_bf.size) && icpy < input_buffer + header.record_size; ++s) {
												std::memcpy(thingy.m_big_array_bf.vptr(s), reinterpret_cast<dcon::bitfield_type const*>(icpy), std::min(size_t(thingy.size_used + 7) / 8, size_t(input_buffer + header.record_size - icpy)));
												icpy += (thingy.size_used + 7) / 8;
											}
											serialize_selection.thingy_big_array_bf = true;
										}
									}
									break;
								}
							} while(false);
							break;
						}
						if(header.is_object("car")) {
							do {
								if(header.is_property("$size") && header.record_size == sizeof(uint32_t)) {
									car_resize(*(reinterpret_cast<uint32_t const*>(input_buffer)));
									serialize_selection.car = true;
									break;
								}
								if(header.is_property("wheels")) {
									if(header.is_type("int32_t")) {
										std::memcpy(car.m_wheels.vptr(), reinterpret_cast<int32_t const*>(input_buffer), std::min(size_t(car.size_used) * sizeof(int32_t), size_t(header.record_size)));
										serialize_selection.car_wheels = true;
									}
									else if(header.is_type("int8_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int8_t))); ++i) {
											car.m_wheels.vptr()[i] = int32_t(*(reinterpret_cast<int8_t const*>(input_buffer) + i));
										}
										serialize_selection.car_wheels = true;
									}
									else if(header.is_type("uint8_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint8_t))); ++i) {
											car.m_wheels.vptr()[i] = int32_t(*(reinterpret_cast<uint8_t const*>(input_buffer) + i));
										}
										serialize_selection.car_wheels = true;
									}
									else if(header.is_type("int16_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int16_t))); ++i) {
											car.m_wheels.vptr()[i] = int32_t(*(reinterpret_cast<int16_t const*>(input_buffer) + i));
										}
										serialize_selection.car_wheels = true;
									}
									else if(header.is_type("uint16_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint16_t))); ++i) {
											car.m_wheels.vptr()[i] = int32_t(*(reinterpret_cast<uint16_t const*>(input_buffer) + i));
										}
										serialize_selection.car_wheels = true;
									}
									else if(header.is_type("uint32_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint32_t))); ++i) {
											car.m_wheels.vptr()[i] = int32_t(*(reinterpret_cast<uint32_t const*>(input_buffer) + i));
										}
										serialize_selection.car_wheels = true;
									}
									else if(header.is_type("int64_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int64_t))); ++i) {
											car.m_wheels.vptr()[i] = int32_t(*(reinterpret_cast<int64_t const*>(input_buffer) + i));
										}
										serialize_selection.car_wheels = true;
									}
									else if(header.is_type("uint64_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint64_t))); ++i) {
											car.m_wheels.vptr()[i] = int32_t(*(reinterpret_cast<uint64_t const*>(input_buffer) + i));
										}
										serialize_selection.car_wheels = true;
									}
									else if(header.is_type("float")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(float))); ++i) {
											car.m_wheels.vptr()[i] = int32_t(*(reinterpret_cast<float const*>(input_buffer) + i));
										}
										serialize_selection.car_wheels = true;
									}
									else if(header.is_type("double")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(double))); ++i) {
											car.m_wheels.vptr()[i] = int32_t(*(reinterpret_cast<double const*>(input_buffer) + i));
										}
										serialize_selection.car_wheels = true;
									}
									break;
								}
								if(header.is_property("resale_value")) {
									if(header.is_type("float")) {
										std::memcpy(car.m_resale_value.vptr(), reinterpret_cast<float const*>(input_buffer), std::min(size_t(car.size_used) * sizeof(float), size_t(header.record_size)));
										serialize_selection.car_resale_value = true;
									}
									else if(header.is_type("int8_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int8_t))); ++i) {
											car.m_resale_value.vptr()[i] = float(*(reinterpret_cast<int8_t const*>(input_buffer) + i));
										}
										serialize_selection.car_resale_value = true;
									}
									else if(header.is_type("uint8_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint8_t))); ++i) {
											car.m_resale_value.vptr()[i] = float(*(reinterpret_cast<uint8_t const*>(input_buffer) + i));
										}
										serialize_selection.car_resale_value = true;
									}
									else if(header.is_type("int16_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int16_t))); ++i) {
											car.m_resale_value.vptr()[i] = float(*(reinterpret_cast<int16_t const*>(input_buffer) + i));
										}
										serialize_selection.car_resale_value = true;
									}
									else if(header.is_type("uint16_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint16_t))); ++i) {
											car.m_resale_value.vptr()[i] = float(*(reinterpret_cast<uint16_t const*>(input_buffer) + i));
										}
										serialize_selection.car_resale_value = true;
									}
									else if(header.is_type("int32_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int32_t))); ++i) {
											car.m_resale_value.vptr()[i] = float(*(reinterpret_cast<int32_t const*>(input_buffer) + i));
										}
										serialize_selection.car_resale_value = true;
									}
									else if(header.is_type("uint32_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint32_t))); ++i) {
											car.m_resale_value.vptr()[i] = float(*(reinterpret_cast<uint32_t const*>(input_buffer) + i));
										}
										serialize_selection.car_resale_value = true;
									}
									else if(header.is_type("int64_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int64_t))); ++i) {
											car.m_resale_value.vptr()[i] = float(*(reinterpret_cast<int64_t const*>(input_buffer) + i));
										}
										serialize_selection.car_resale_value = true;
									}
									else if(header.is_type("uint64_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint64_t))); ++i) {
											car.m_resale_value.vptr()[i] = float(*(reinterpret_cast<uint64_t const*>(input_buffer) + i));
										}
										serialize_selection.car_resale_value = true;
									}
									else if(header.is_type("double")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(double))); ++i) {
											car.m_resale_value.vptr()[i] = float(*(reinterpret_cast<double const*>(input_buffer) + i));
										}
										serialize_selection.car_resale_value = true;
									}
									break;
								}
							} while(false);
							break;
						}
						if(header.is_object("person")) {
							do {
								if(header.is_property("$size") && header.record_size == sizeof(uint32_t)) {
									person_resize(*(reinterpret_cast<uint32_t const*>(input_buffer)));
									serialize_selection.person = true;
									break;
								}
								if(header.is_property("_index")) {
									if(header.is_type("uint8_t")) {
										std::memcpy(person.m__index.vptr(), reinterpret_cast<uint8_t const*>(input_buffer), std::min(size_t(person.size_used) * sizeof(uint8_t), size_t(header.record_size)));
										serialize_selection.person__index = true;
									}
									else if(header.is_type("uint16_t")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(uint16_t))); ++i) {
											person.m__index.vptr()[i].value = uint8_t(*(reinterpret_cast<uint16_t const*>(input_buffer) + i));
										}
										serialize_selection.person__index = true;
									}
									else if(header.is_type("uint32_t")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(uint32_t))); ++i) {
											person.m__index.vptr()[i].value = uint8_t(*(reinterpret_cast<uint32_t const*>(input_buffer) + i));
										}
										serialize_selection.person__index = true;
									}
									if(serialize_selection.person__index == true) {
										person.first_free = person_id();
										for(int32_t j = 100 - 1; j >= 0; --j) {
											if(person.m__index.vptr()[j] != person_id(uint8_t(j))) {
												person.m__index.vptr()[j] = person.first_free;
												person.first_free = person_id(uint8_t(j));
											} else {
											}
										}
									}
									break;
								}
								if(header.is_property("age")) {
									if(header.is_type("int32_t")) {
										std::memcpy(person.m_age.vptr(), reinterpret_cast<int32_t const*>(input_buffer), std::min(size_t(person.size_used) * sizeof(int32_t), size_t(header.record_size)));
										serialize_selection.person_age = true;
									}
									else if(header.is_type("int8_t")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(int8_t))); ++i) {
											person.m_age.vptr()[i] = int32_t(*(reinterpret_cast<int8_t const*>(input_buffer) + i));
										}
										serialize_selection.person_age = true;
									}
									else if(header.is_type("uint8_t")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(uint8_t))); ++i) {
											person.m_age.vptr()[i] = int32_t(*(reinterpret_cast<uint8_t const*>(input_buffer) + i));
										}
										serialize_selection.person_age = true;
									}
									else if(header.is_type("int16_t")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(int16_t))); ++i) {
											person.m_age.vptr()[i] = int32_t(*(reinterpret_cast<int16_t const*>(input_buffer) + i));
										}
										serialize_selection.person_age = true;
									}
									else if(header.is_type("uint16_t")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(uint16_t))); ++i) {
											person.m_age.vptr()[i] = int32_t(*(reinterpret_cast<uint16_t const*>(input_buffer) + i));
										}
										serialize_selection.person_age = true;
									}
									else if(header.is_type("uint32_t")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(uint32_t))); ++i) {
											person.m_age.vptr()[i] = int32_t(*(reinterpret_cast<uint32_t const*>(input_buffer) + i));
										}
										serialize_selection.person_age = true;
									}
									else if(header.is_type("int64_t")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(int64_t))); ++i) {
											person.m_age.vptr()[i] = int32_t(*(reinterpret_cast<int64_t const*>(input_buffer) + i));
										}
										serialize_selection.person_age = true;
									}
									else if(header.is_type("uint64_t")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(uint64_t))); ++i) {
											person.m_age.vptr()[i] = int32_t(*(reinterpret_cast<uint64_t const*>(input_buffer) + i));
										}
										serialize_selection.person_age = true;
									}
									else if(header.is_type("float")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(float))); ++i) {
											person.m_age.vptr()[i] = int32_t(*(reinterpret_cast<float const*>(input_buffer) + i));
										}
										serialize_selection.person_age = true;
									}
									else if(header.is_type("double")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(double))); ++i) {
											person.m_age.vptr()[i] = int32_t(*(reinterpret_cast<double const*>(input_buffer) + i));
										}
										serialize_selection.person_age = true;
									}
									break;
								}
							} while(false);
							break;
						}
						if(header.is_object("car_ownership")) {
							do {
								if(header.is_property("$size") && header.record_size == sizeof(uint32_t)) {
									if(*(reinterpret_cast<uint32_t const*>(input_buffer)) >= car.size_used) {
										car_ownership_resize(0);
									}
									car_ownership_resize(*(reinterpret_cast<uint32_t const*>(input_buffer)));
									serialize_selection.car_ownership = true;
									break;
								}
								if(header.is_property("owner")) {
									if(header.is_type("uint8_t")) {
										std::memcpy(car_ownership.m_owner.vptr(), reinterpret_cast<uint8_t const*>(input_buffer), std::min(size_t(car.size_used) * sizeof(uint8_t), size_t(header.record_size)));
										serialize_selection.car_ownership_owner = true;
									}
									else if(header.is_type("uint16_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint16_t))); ++i) {
											car_ownership.m_owner.vptr()[i].value = uint8_t(*(reinterpret_cast<uint16_t const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_owner = true;
									}
									else if(header.is_type("uint32_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint32_t))); ++i) {
											car_ownership.m_owner.vptr()[i].value = uint8_t(*(reinterpret_cast<uint32_t const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_owner = true;
									}
									break;
								}
								if(header.is_property("$index_end")) {
									if(serialize_selection.car_ownership_owner == true) {
										for(uint32_t i = 0; i < car.size_used; ++i) {
											auto tmp = car_ownership.m_owner.vptr()[i];
											car_ownership.m_owner.vptr()[i] = person_id();
											internal_car_ownership_set_owner(car_ownership_id(car_ownership_id::value_base_t(i)), tmp);
										}
									}
									break;
								}
								if(header.is_property("ownership_date")) {
									if(header.is_type("int32_t")) {
										std::memcpy(car_ownership.m_ownership_date.vptr(), reinterpret_cast<int32_t const*>(input_buffer), std::min(size_t(car.size_used) * sizeof(int32_t), size_t(header.record_size)));
										serialize_selection.car_ownership_ownership_date = true;
									}
									else if(header.is_type("int8_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int8_t))); ++i) {
											car_ownership.m_ownership_date.vptr()[i] = int32_t(*(reinterpret_cast<int8_t const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_ownership_date = true;
									}
									else if(header.is_type("uint8_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint8_t))); ++i) {
											car_ownership.m_ownership_date.vptr()[i] = int32_t(*(reinterpret_cast<uint8_t const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_ownership_date = true;
									}
									else if(header.is_type("int16_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int16_t))); ++i) {
											car_ownership.m_ownership_date.vptr()[i] = int32_t(*(reinterpret_cast<int16_t const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_ownership_date = true;
									}
									else if(header.is_type("uint16_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint16_t))); ++i) {
											car_ownership.m_ownership_date.vptr()[i] = int32_t(*(reinterpret_cast<uint16_t const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_ownership_date = true;
									}
									else if(header.is_type("uint32_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint32_t))); ++i) {
											car_ownership.m_ownership_date.vptr()[i] = int32_t(*(reinterpret_cast<uint32_t const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_ownership_date = true;
									}
									else if(header.is_type("int64_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int64_t))); ++i) {
											car_ownership.m_ownership_date.vptr()[i] = int32_t(*(reinterpret_cast<int64_t const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_ownership_date = true;
									}
									else if(header.is_type("uint64_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint64_t))); ++i) {
											car_ownership.m_ownership_date.vptr()[i] = int32_t(*(reinterpret_cast<uint64_t const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_ownership_date = true;
									}
									else if(header.is_type("float")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(float))); ++i) {
											car_ownership.m_ownership_date.vptr()[i] = int32_t(*(reinterpret_cast<float const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_ownership_date = true;
									}
									else if(header.is_type("double")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(double))); ++i) {
											car_ownership.m_ownership_date.vptr()[i] = int32_t(*(reinterpret_cast<double const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_ownership_date = true;
									}
									break;
								}
							} while(false);
							break;
						}
					} while(false);
				}
				input_buffer += header.record_size;
			}
		}
		
		//
		// deserialize the desired objects, relationships, and properties where the mask is set
		//
		void deserialize(std::byte const*& input_buffer, std::byte const* end, load_record& serialize_selection, load_record const& mask) {
			while(input_buffer < end) {
				dcon::record_header header;
				header.deserialize(input_buffer, end);
				if(input_buffer + header.record_size <= end) {
					do {
						if(header.is_object("thingy") && mask.thingy) {
							do {
								if(header.is_property("$size") && header.record_size == sizeof(uint32_t)) {
									thingy_resize(*(reinterpret_cast<uint32_t const*>(input_buffer)));
									serialize_selection.thingy = true;
									break;
								}
								if(header.is_property("some_value") && mask.thingy_some_value) {
									if(header.is_type("int32_t")) {
										std::memcpy(thingy.m_some_value.vptr(), reinterpret_cast<int32_t const*>(input_buffer), std::min(size_t(thingy.size_used) * sizeof(int32_t), size_t(header.record_size)));
										serialize_selection.thingy_some_value = true;
									}
									else if(header.is_type("int8_t")) {
										for(uint32_t i = 0; i < std::min(thingy.size_used, uint32_t(header.record_size / sizeof(int8_t))); ++i) {
											thingy.m_some_value.vptr()[i] = int32_t(*(reinterpret_cast<int8_t const*>(input_buffer) + i));
										}
										serialize_selection.thingy_some_value = true;
									}
									else if(header.is_type("uint8_t")) {
										for(uint32_t i = 0; i < std::min(thingy.size_used, uint32_t(header.record_size / sizeof(uint8_t))); ++i) {
											thingy.m_some_value.vptr()[i] = int32_t(*(reinterpret_cast<uint8_t const*>(input_buffer) + i));
										}
										serialize_selection.thingy_some_value = true;
									}
									else if(header.is_type("int16_t")) {
										for(uint32_t i = 0; i < std::min(thingy.size_used, uint32_t(header.record_size / sizeof(int16_t))); ++i) {
											thingy.m_some_value.vptr()[i] = int32_t(*(reinterpret_cast<int16_t const*>(input_buffer) + i));
										}
										serialize_selection.thingy_some_value = true;
									}
									else if(header.is_type("uint16_t")) {
										for(uint32_t i = 0; i < std::min(thingy.size_used, uint32_t(header.record_size / sizeof(uint16_t))); ++i) {
											thingy.m_some_value.vptr()[i] = int32_t(*(reinterpret_cast<uint16_t const*>(input_buffer) + i));
										}
										serialize_selection.thingy_some_value = true;
									}
									else if(header.is_type("uint32_t")) {
										for(uint32_t i = 0; i < std::min(thingy.size_used, uint32_t(header.record_size / sizeof(uint32_t))); ++i) {
											thingy.m_some_value.vptr()[i] = int32_t(*(reinterpret_cast<uint32_t const*>(input_buffer) + i));
										}
										serialize_selection.thingy_some_value = true;
									}
									else if(header.is_type("int64_t")) {
										for(uint32_t i = 0; i < std::min(thingy.size_used, uint32_t(header.record_size / sizeof(int64_t))); ++i) {
											thingy.m_some_value.vptr()[i] = int32_t(*(reinterpret_cast<int64_t const*>(input_buffer) + i));
										}
										serialize_selection.thingy_some_value = true;
									}
									else if(header.is_type("uint64_t")) {
										for(uint32_t i = 0; i < std::min(thingy.size_used, uint32_t(header.record_size / sizeof(uint64_t))); ++i) {
											thingy.m_some_value.vptr()[i] = int32_t(*(reinterpret_cast<uint64_t const*>(input_buffer) + i));
										}
										serialize_selection.thingy_some_value = true;
									}
									else if(header.is_type("float")) {
										for(uint32_t i = 0; i < std::min(thingy.size_used, uint32_t(header.record_size / sizeof(float))); ++i) {
											thingy.m_some_value.vptr()[i] = int32_t(*(reinterpret_cast<float const*>(input_buffer) + i));
										}
										serialize_selection.thingy_some_value = true;
									}
									else if(header.is_type("double")) {
										for(uint32_t i = 0; i < std::min(thingy.size_used, uint32_t(header.record_size / sizeof(double))); ++i) {
											thingy.m_some_value.vptr()[i] = int32_t(*(reinterpret_cast<double const*>(input_buffer) + i));
										}
										serialize_selection.thingy_some_value = true;
									}
									break;
								}
								if(header.is_property("bf_value") && mask.thingy_bf_value) {
									if(header.is_type("bitfield")) {
										std::memcpy(thingy.m_bf_value.vptr(), reinterpret_cast<dcon::bitfield_type const*>(input_buffer), std::min(size_t(thingy.size_used + 7) / 8, size_t(header.record_size)));
										serialize_selection.thingy_bf_value = true;
									}
									break;
								}
								if(header.is_property("obj_value") && mask.thingy_obj_value) {
									if(header.is_type("std::vector<float>")) {
										std::byte const* icpy = input_buffer;
										for(uint32_t i = 0; icpy < input_buffer + header.record_size && i < thingy.size_used; ++i) {
											deserialize(icpy, thingy.m_obj_value.vptr()[i], input_buffer + header.record_size);
										}
										serialize_selection.thingy_obj_value = true;
									}
									break;
								}
								if(header.is_property("pooled_v") && mask.thingy_pooled_v) {
									if(header.is_type("stable_mk_2_tag")) {
										uint32_t ix = 0;
										std::byte const* zero_pos = std::find(input_buffer, input_buffer + header.record_size, std::byte(0));
										if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "int16_t")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(int16_t) ));
													icpy += sizeof(uint16_t);
												}
												dcon::load_range(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], reinterpret_cast<int16_t const*>(icpy), reinterpret_cast<int16_t const*>(icpy) + sz);
												icpy += sz * sizeof(int16_t);
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "int8_t")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(int8_t) ));
													icpy += sizeof(uint16_t);
												}
												dcon::resize(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], sz);
												for(uint32_t ii = 0; ii < sz && icpy < input_buffer + header.record_size; ++ii) {
													dcon::get(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], ii) = int16_t(*(reinterpret_cast<int8_t const*>(icpy)));
													icpy += sizeof(int8_t);
												}
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "uint8_t")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(uint8_t) ));
													icpy += sizeof(uint16_t);
												}
												dcon::resize(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], sz);
												for(uint32_t ii = 0; ii < sz && icpy < input_buffer + header.record_size; ++ii) {
													dcon::get(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], ii) = int16_t(*(reinterpret_cast<uint8_t const*>(icpy)));
													icpy += sizeof(uint8_t);
												}
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "uint16_t")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(uint16_t) ));
													icpy += sizeof(uint16_t);
												}
												dcon::resize(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], sz);
												for(uint32_t ii = 0; ii < sz && icpy < input_buffer + header.record_size; ++ii) {
													dcon::get(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], ii) = int16_t(*(reinterpret_cast<uint16_t const*>(icpy)));
													icpy += sizeof(uint16_t);
												}
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "int32_t")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(int32_t) ));
													icpy += sizeof(uint16_t);
												}
												dcon::resize(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], sz);
												for(uint32_t ii = 0; ii < sz && icpy < input_buffer + header.record_size; ++ii) {
													dcon::get(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], ii) = int16_t(*(reinterpret_cast<int32_t const*>(icpy)));
													icpy += sizeof(int32_t);
												}
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "uint32_t")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(uint32_t) ));
													icpy += sizeof(uint16_t);
												}
												dcon::resize(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], sz);
												for(uint32_t ii = 0; ii < sz && icpy < input_buffer + header.record_size; ++ii) {
													dcon::get(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], ii) = int16_t(*(reinterpret_cast<uint32_t const*>(icpy)));
													icpy += sizeof(uint32_t);
												}
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "int64_t")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(int64_t) ));
													icpy += sizeof(uint16_t);
												}
												dcon::resize(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], sz);
												for(uint32_t ii = 0; ii < sz && icpy < input_buffer + header.record_size; ++ii) {
													dcon::get(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], ii) = int16_t(*(reinterpret_cast<int64_t const*>(icpy)));
													icpy += sizeof(int64_t);
												}
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "uint64_t")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(uint64_t) ));
													icpy += sizeof(uint16_t);
												}
												dcon::resize(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], sz);
												for(uint32_t ii = 0; ii < sz && icpy < input_buffer + header.record_size; ++ii) {
													dcon::get(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], ii) = int16_t(*(reinterpret_cast<uint64_t const*>(icpy)));
													icpy += sizeof(uint64_t);
												}
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "float")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(float) ));
													icpy += sizeof(uint16_t);
												}
												dcon::resize(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], sz);
												for(uint32_t ii = 0; ii < sz && icpy < input_buffer + header.record_size; ++ii) {
													dcon::get(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], ii) = int16_t(*(reinterpret_cast<float const*>(icpy)));
													icpy += sizeof(float);
												}
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "double")) {
											for(std::byte const* icpy = zero_pos + 1; ix < thingy.size_used && icpy < input_buffer + header.record_size; ) {
												uint16_t sz = 0;
												if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
													sz = uint16_t(std::min(size_t(*(reinterpret_cast<uint16_t const*>(icpy))), (input_buffer + header.record_size - (icpy + sizeof(uint16_t))) / sizeof(double) ));
													icpy += sizeof(uint16_t);
												}
												dcon::resize(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], sz);
												for(uint32_t ii = 0; ii < sz && icpy < input_buffer + header.record_size; ++ii) {
													dcon::get(thingy.pooled_v_storage, thingy.m_pooled_v.vptr()[ix], ii) = int16_t(*(reinterpret_cast<double const*>(icpy)));
													icpy += sizeof(double);
												}
												++ix;
											}
											serialize_selection.thingy_pooled_v = true;
										}
									}
									break;
								}
								if(header.is_property("big_array") && mask.thingy_big_array) {
									if(header.is_type("$array")) {
										std::byte const* zero_pos = std::find(input_buffer, input_buffer + header.record_size, std::byte(0));
										std::byte const* icpy = zero_pos + 1;
										if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "float")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												std::memcpy(thingy.m_big_array.vptr(s), reinterpret_cast<float const*>(icpy), std::min(sizeof(float) * thingy.size_used, size_t(input_buffer + header.record_size - icpy)));
												icpy += sizeof(float) * thingy.size_used;
											}
											serialize_selection.thingy_big_array = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "int8_t")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												for(uint32_t j = 0; j < thingy.size_used && icpy < input_buffer + header.record_size; ++j) {
													thingy.m_big_array.vptr(s)[j] = float(*(reinterpret_cast<int8_t const*>(icpy)));
													icpy += sizeof(int8_t);
												}
											}
											serialize_selection.thingy_big_array = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "uint8_t")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												for(uint32_t j = 0; j < thingy.size_used && icpy < input_buffer + header.record_size; ++j) {
													thingy.m_big_array.vptr(s)[j] = float(*(reinterpret_cast<uint8_t const*>(icpy)));
													icpy += sizeof(uint8_t);
												}
											}
											serialize_selection.thingy_big_array = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "int16_t")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												for(uint32_t j = 0; j < thingy.size_used && icpy < input_buffer + header.record_size; ++j) {
													thingy.m_big_array.vptr(s)[j] = float(*(reinterpret_cast<int16_t const*>(icpy)));
													icpy += sizeof(int16_t);
												}
											}
											serialize_selection.thingy_big_array = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "uint16_t")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												for(uint32_t j = 0; j < thingy.size_used && icpy < input_buffer + header.record_size; ++j) {
													thingy.m_big_array.vptr(s)[j] = float(*(reinterpret_cast<uint16_t const*>(icpy)));
													icpy += sizeof(uint16_t);
												}
											}
											serialize_selection.thingy_big_array = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "int32_t")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												for(uint32_t j = 0; j < thingy.size_used && icpy < input_buffer + header.record_size; ++j) {
													thingy.m_big_array.vptr(s)[j] = float(*(reinterpret_cast<int32_t const*>(icpy)));
													icpy += sizeof(int32_t);
												}
											}
											serialize_selection.thingy_big_array = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "uint32_t")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												for(uint32_t j = 0; j < thingy.size_used && icpy < input_buffer + header.record_size; ++j) {
													thingy.m_big_array.vptr(s)[j] = float(*(reinterpret_cast<uint32_t const*>(icpy)));
													icpy += sizeof(uint32_t);
												}
											}
											serialize_selection.thingy_big_array = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "int64_t")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												for(uint32_t j = 0; j < thingy.size_used && icpy < input_buffer + header.record_size; ++j) {
													thingy.m_big_array.vptr(s)[j] = float(*(reinterpret_cast<int64_t const*>(icpy)));
													icpy += sizeof(int64_t);
												}
											}
											serialize_selection.thingy_big_array = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "uint64_t")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												for(uint32_t j = 0; j < thingy.size_used && icpy < input_buffer + header.record_size; ++j) {
													thingy.m_big_array.vptr(s)[j] = float(*(reinterpret_cast<uint64_t const*>(icpy)));
													icpy += sizeof(uint64_t);
												}
											}
											serialize_selection.thingy_big_array = true;
										}
										else if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "double")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array.size) && icpy < input_buffer + header.record_size; ++s) {
												for(uint32_t j = 0; j < thingy.size_used && icpy < input_buffer + header.record_size; ++j) {
													thingy.m_big_array.vptr(s)[j] = float(*(reinterpret_cast<double const*>(icpy)));
													icpy += sizeof(double);
												}
											}
											serialize_selection.thingy_big_array = true;
										}
									}
									break;
								}
								if(header.is_property("big_array_bf") && mask.thingy_big_array_bf) {
									if(header.is_type("$array")) {
										std::byte const* zero_pos = std::find(input_buffer, input_buffer + header.record_size, std::byte(0));
										std::byte const* icpy = zero_pos + 1;
										if(dcon::char_span_equals_str(reinterpret_cast<char const*>(input_buffer), reinterpret_cast<char const*>(zero_pos), "bitfield")) {
											if(icpy + sizeof(uint16_t) <= input_buffer + header.record_size) {
												thingy.m_big_array_bf.resize(*(reinterpret_cast<uint16_t const*>(icpy)), thingy.size_used);
												icpy += sizeof(uint16_t);
											} else {
												thingy.m_big_array_bf.resize(0, thingy.size_used);
											}
											for(int32_t s = 0; s < int32_t(thingy.m_big_array_bf.size) && icpy < input_buffer + header.record_size; ++s) {
												std::memcpy(thingy.m_big_array_bf.vptr(s), reinterpret_cast<dcon::bitfield_type const*>(icpy), std::min(size_t(thingy.size_used + 7) / 8, size_t(input_buffer + header.record_size - icpy)));
												icpy += (thingy.size_used + 7) / 8;
											}
											serialize_selection.thingy_big_array_bf = true;
										}
									}
									break;
								}
							} while(false);
							break;
						}
						if(header.is_object("car") && mask.car) {
							do {
								if(header.is_property("$size") && header.record_size == sizeof(uint32_t)) {
									car_resize(*(reinterpret_cast<uint32_t const*>(input_buffer)));
									serialize_selection.car = true;
									break;
								}
								if(header.is_property("wheels") && mask.car_wheels) {
									if(header.is_type("int32_t")) {
										std::memcpy(car.m_wheels.vptr(), reinterpret_cast<int32_t const*>(input_buffer), std::min(size_t(car.size_used) * sizeof(int32_t), size_t(header.record_size)));
										serialize_selection.car_wheels = true;
									}
									else if(header.is_type("int8_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int8_t))); ++i) {
											car.m_wheels.vptr()[i] = int32_t(*(reinterpret_cast<int8_t const*>(input_buffer) + i));
										}
										serialize_selection.car_wheels = true;
									}
									else if(header.is_type("uint8_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint8_t))); ++i) {
											car.m_wheels.vptr()[i] = int32_t(*(reinterpret_cast<uint8_t const*>(input_buffer) + i));
										}
										serialize_selection.car_wheels = true;
									}
									else if(header.is_type("int16_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int16_t))); ++i) {
											car.m_wheels.vptr()[i] = int32_t(*(reinterpret_cast<int16_t const*>(input_buffer) + i));
										}
										serialize_selection.car_wheels = true;
									}
									else if(header.is_type("uint16_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint16_t))); ++i) {
											car.m_wheels.vptr()[i] = int32_t(*(reinterpret_cast<uint16_t const*>(input_buffer) + i));
										}
										serialize_selection.car_wheels = true;
									}
									else if(header.is_type("uint32_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint32_t))); ++i) {
											car.m_wheels.vptr()[i] = int32_t(*(reinterpret_cast<uint32_t const*>(input_buffer) + i));
										}
										serialize_selection.car_wheels = true;
									}
									else if(header.is_type("int64_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int64_t))); ++i) {
											car.m_wheels.vptr()[i] = int32_t(*(reinterpret_cast<int64_t const*>(input_buffer) + i));
										}
										serialize_selection.car_wheels = true;
									}
									else if(header.is_type("uint64_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint64_t))); ++i) {
											car.m_wheels.vptr()[i] = int32_t(*(reinterpret_cast<uint64_t const*>(input_buffer) + i));
										}
										serialize_selection.car_wheels = true;
									}
									else if(header.is_type("float")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(float))); ++i) {
											car.m_wheels.vptr()[i] = int32_t(*(reinterpret_cast<float const*>(input_buffer) + i));
										}
										serialize_selection.car_wheels = true;
									}
									else if(header.is_type("double")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(double))); ++i) {
											car.m_wheels.vptr()[i] = int32_t(*(reinterpret_cast<double const*>(input_buffer) + i));
										}
										serialize_selection.car_wheels = true;
									}
									break;
								}
								if(header.is_property("resale_value") && mask.car_resale_value) {
									if(header.is_type("float")) {
										std::memcpy(car.m_resale_value.vptr(), reinterpret_cast<float const*>(input_buffer), std::min(size_t(car.size_used) * sizeof(float), size_t(header.record_size)));
										serialize_selection.car_resale_value = true;
									}
									else if(header.is_type("int8_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int8_t))); ++i) {
											car.m_resale_value.vptr()[i] = float(*(reinterpret_cast<int8_t const*>(input_buffer) + i));
										}
										serialize_selection.car_resale_value = true;
									}
									else if(header.is_type("uint8_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint8_t))); ++i) {
											car.m_resale_value.vptr()[i] = float(*(reinterpret_cast<uint8_t const*>(input_buffer) + i));
										}
										serialize_selection.car_resale_value = true;
									}
									else if(header.is_type("int16_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int16_t))); ++i) {
											car.m_resale_value.vptr()[i] = float(*(reinterpret_cast<int16_t const*>(input_buffer) + i));
										}
										serialize_selection.car_resale_value = true;
									}
									else if(header.is_type("uint16_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint16_t))); ++i) {
											car.m_resale_value.vptr()[i] = float(*(reinterpret_cast<uint16_t const*>(input_buffer) + i));
										}
										serialize_selection.car_resale_value = true;
									}
									else if(header.is_type("int32_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int32_t))); ++i) {
											car.m_resale_value.vptr()[i] = float(*(reinterpret_cast<int32_t const*>(input_buffer) + i));
										}
										serialize_selection.car_resale_value = true;
									}
									else if(header.is_type("uint32_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint32_t))); ++i) {
											car.m_resale_value.vptr()[i] = float(*(reinterpret_cast<uint32_t const*>(input_buffer) + i));
										}
										serialize_selection.car_resale_value = true;
									}
									else if(header.is_type("int64_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int64_t))); ++i) {
											car.m_resale_value.vptr()[i] = float(*(reinterpret_cast<int64_t const*>(input_buffer) + i));
										}
										serialize_selection.car_resale_value = true;
									}
									else if(header.is_type("uint64_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint64_t))); ++i) {
											car.m_resale_value.vptr()[i] = float(*(reinterpret_cast<uint64_t const*>(input_buffer) + i));
										}
										serialize_selection.car_resale_value = true;
									}
									else if(header.is_type("double")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(double))); ++i) {
											car.m_resale_value.vptr()[i] = float(*(reinterpret_cast<double const*>(input_buffer) + i));
										}
										serialize_selection.car_resale_value = true;
									}
									break;
								}
							} while(false);
							break;
						}
						if(header.is_object("person") && mask.person) {
							do {
								if(header.is_property("$size") && header.record_size == sizeof(uint32_t)) {
									person_resize(*(reinterpret_cast<uint32_t const*>(input_buffer)));
									serialize_selection.person = true;
									break;
								}
								if(header.is_property("_index") && mask.person__index) {
									if(header.is_type("uint8_t")) {
										std::memcpy(person.m__index.vptr(), reinterpret_cast<uint8_t const*>(input_buffer), std::min(size_t(person.size_used) * sizeof(uint8_t), size_t(header.record_size)));
										serialize_selection.person__index = true;
									}
									else if(header.is_type("uint16_t")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(uint16_t))); ++i) {
											person.m__index.vptr()[i].value = uint8_t(*(reinterpret_cast<uint16_t const*>(input_buffer) + i));
										}
										serialize_selection.person__index = true;
									}
									else if(header.is_type("uint32_t")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(uint32_t))); ++i) {
											person.m__index.vptr()[i].value = uint8_t(*(reinterpret_cast<uint32_t const*>(input_buffer) + i));
										}
										serialize_selection.person__index = true;
									}
									if(serialize_selection.person__index == true) {
										person.first_free = person_id();
										for(int32_t j = 100 - 1; j >= 0; --j) {
											if(person.m__index.vptr()[j] != person_id(uint8_t(j))) {
												person.m__index.vptr()[j] = person.first_free;
												person.first_free = person_id(uint8_t(j));
											} else {
											}
										}
									}
									break;
								}
								if(header.is_property("age") && mask.person_age) {
									if(header.is_type("int32_t")) {
										std::memcpy(person.m_age.vptr(), reinterpret_cast<int32_t const*>(input_buffer), std::min(size_t(person.size_used) * sizeof(int32_t), size_t(header.record_size)));
										serialize_selection.person_age = true;
									}
									else if(header.is_type("int8_t")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(int8_t))); ++i) {
											person.m_age.vptr()[i] = int32_t(*(reinterpret_cast<int8_t const*>(input_buffer) + i));
										}
										serialize_selection.person_age = true;
									}
									else if(header.is_type("uint8_t")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(uint8_t))); ++i) {
											person.m_age.vptr()[i] = int32_t(*(reinterpret_cast<uint8_t const*>(input_buffer) + i));
										}
										serialize_selection.person_age = true;
									}
									else if(header.is_type("int16_t")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(int16_t))); ++i) {
											person.m_age.vptr()[i] = int32_t(*(reinterpret_cast<int16_t const*>(input_buffer) + i));
										}
										serialize_selection.person_age = true;
									}
									else if(header.is_type("uint16_t")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(uint16_t))); ++i) {
											person.m_age.vptr()[i] = int32_t(*(reinterpret_cast<uint16_t const*>(input_buffer) + i));
										}
										serialize_selection.person_age = true;
									}
									else if(header.is_type("uint32_t")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(uint32_t))); ++i) {
											person.m_age.vptr()[i] = int32_t(*(reinterpret_cast<uint32_t const*>(input_buffer) + i));
										}
										serialize_selection.person_age = true;
									}
									else if(header.is_type("int64_t")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(int64_t))); ++i) {
											person.m_age.vptr()[i] = int32_t(*(reinterpret_cast<int64_t const*>(input_buffer) + i));
										}
										serialize_selection.person_age = true;
									}
									else if(header.is_type("uint64_t")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(uint64_t))); ++i) {
											person.m_age.vptr()[i] = int32_t(*(reinterpret_cast<uint64_t const*>(input_buffer) + i));
										}
										serialize_selection.person_age = true;
									}
									else if(header.is_type("float")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(float))); ++i) {
											person.m_age.vptr()[i] = int32_t(*(reinterpret_cast<float const*>(input_buffer) + i));
										}
										serialize_selection.person_age = true;
									}
									else if(header.is_type("double")) {
										for(uint32_t i = 0; i < std::min(person.size_used, uint32_t(header.record_size / sizeof(double))); ++i) {
											person.m_age.vptr()[i] = int32_t(*(reinterpret_cast<double const*>(input_buffer) + i));
										}
										serialize_selection.person_age = true;
									}
									break;
								}
							} while(false);
							break;
						}
						if(header.is_object("car_ownership") && mask.car_ownership) {
							do {
								if(header.is_property("$size") && header.record_size == sizeof(uint32_t)) {
									if(*(reinterpret_cast<uint32_t const*>(input_buffer)) >= car.size_used) {
										car_ownership_resize(0);
									}
									car_ownership_resize(*(reinterpret_cast<uint32_t const*>(input_buffer)));
									serialize_selection.car_ownership = true;
									break;
								}
								if(header.is_property("owner") && mask.car_ownership_owner) {
									if(header.is_type("uint8_t")) {
										std::memcpy(car_ownership.m_owner.vptr(), reinterpret_cast<uint8_t const*>(input_buffer), std::min(size_t(car.size_used) * sizeof(uint8_t), size_t(header.record_size)));
										serialize_selection.car_ownership_owner = true;
									}
									else if(header.is_type("uint16_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint16_t))); ++i) {
											car_ownership.m_owner.vptr()[i].value = uint8_t(*(reinterpret_cast<uint16_t const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_owner = true;
									}
									else if(header.is_type("uint32_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint32_t))); ++i) {
											car_ownership.m_owner.vptr()[i].value = uint8_t(*(reinterpret_cast<uint32_t const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_owner = true;
									}
									break;
								}
								if(header.is_property("$index_end") && mask.car_ownership) {
									if(serialize_selection.car_ownership_owner == true) {
										for(uint32_t i = 0; i < car.size_used; ++i) {
											auto tmp = car_ownership.m_owner.vptr()[i];
											car_ownership.m_owner.vptr()[i] = person_id();
											internal_car_ownership_set_owner(car_ownership_id(car_ownership_id::value_base_t(i)), tmp);
										}
									}
									break;
								}
								if(header.is_property("ownership_date") && mask.car_ownership_ownership_date) {
									if(header.is_type("int32_t")) {
										std::memcpy(car_ownership.m_ownership_date.vptr(), reinterpret_cast<int32_t const*>(input_buffer), std::min(size_t(car.size_used) * sizeof(int32_t), size_t(header.record_size)));
										serialize_selection.car_ownership_ownership_date = true;
									}
									else if(header.is_type("int8_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int8_t))); ++i) {
											car_ownership.m_ownership_date.vptr()[i] = int32_t(*(reinterpret_cast<int8_t const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_ownership_date = true;
									}
									else if(header.is_type("uint8_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint8_t))); ++i) {
											car_ownership.m_ownership_date.vptr()[i] = int32_t(*(reinterpret_cast<uint8_t const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_ownership_date = true;
									}
									else if(header.is_type("int16_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int16_t))); ++i) {
											car_ownership.m_ownership_date.vptr()[i] = int32_t(*(reinterpret_cast<int16_t const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_ownership_date = true;
									}
									else if(header.is_type("uint16_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint16_t))); ++i) {
											car_ownership.m_ownership_date.vptr()[i] = int32_t(*(reinterpret_cast<uint16_t const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_ownership_date = true;
									}
									else if(header.is_type("uint32_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint32_t))); ++i) {
											car_ownership.m_ownership_date.vptr()[i] = int32_t(*(reinterpret_cast<uint32_t const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_ownership_date = true;
									}
									else if(header.is_type("int64_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(int64_t))); ++i) {
											car_ownership.m_ownership_date.vptr()[i] = int32_t(*(reinterpret_cast<int64_t const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_ownership_date = true;
									}
									else if(header.is_type("uint64_t")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(uint64_t))); ++i) {
											car_ownership.m_ownership_date.vptr()[i] = int32_t(*(reinterpret_cast<uint64_t const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_ownership_date = true;
									}
									else if(header.is_type("float")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(float))); ++i) {
											car_ownership.m_ownership_date.vptr()[i] = int32_t(*(reinterpret_cast<float const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_ownership_date = true;
									}
									else if(header.is_type("double")) {
										for(uint32_t i = 0; i < std::min(car.size_used, uint32_t(header.record_size / sizeof(double))); ++i) {
											car_ownership.m_ownership_date.vptr()[i] = int32_t(*(reinterpret_cast<double const*>(input_buffer) + i));
										}
										serialize_selection.car_ownership_ownership_date = true;
									}
									break;
								}
							} while(false);
							break;
						}
					} while(false);
				}
				input_buffer += header.record_size;
			}
		}
		

	};

	DCON_RELEASE_INLINE int32_t& thingy_fat_id::get_some_value() const noexcept {
		return container.thingy_get_some_value(id);
	}
	DCON_RELEASE_INLINE void thingy_fat_id::set_some_value(int32_t v) const noexcept {
		container.thingy_set_some_value(id, v);
	}
	DCON_RELEASE_INLINE bool thingy_fat_id::get_bf_value() const noexcept {
		return container.thingy_get_bf_value(id);
	}
	DCON_RELEASE_INLINE void thingy_fat_id::set_bf_value(bool v) const noexcept {
		container.thingy_set_bf_value(id, v);
	}
	DCON_RELEASE_INLINE std::vector<float>& thingy_fat_id::get_obj_value() const noexcept {
		return container.thingy_get_obj_value(id);
	}
	DCON_RELEASE_INLINE void thingy_fat_id::set_obj_value(std::vector<float> const& v) const noexcept {
		container.thingy_set_obj_value(id, v);
	}
	DCON_RELEASE_INLINE dcon::dcon_vv_fat_id<int16_t> thingy_fat_id::get_pooled_v() const noexcept {
		return container.thingy_get_pooled_v(id);
	}
	DCON_RELEASE_INLINE float& thingy_fat_id::get_big_array(int32_t i) const noexcept {
		return container.thingy_get_big_array(id, i);
	}
	DCON_RELEASE_INLINE uint32_t thingy_fat_id::get_big_array_size() const noexcept {
		return container.thingy_get_big_array_size();
	}
	DCON_RELEASE_INLINE void thingy_fat_id::set_big_array(int32_t i, float v) const noexcept {
		container.thingy_set_big_array(id, i, v);
	}
	DCON_RELEASE_INLINE void thingy_fat_id::resize_big_array(uint32_t sz) const noexcept {
		container.thingy_resize_big_array(sz);
	}
	DCON_RELEASE_INLINE bool thingy_fat_id::get_big_array_bf(int32_t i) const noexcept {
		return container.thingy_get_big_array_bf(id, i);
	}
	DCON_RELEASE_INLINE uint32_t thingy_fat_id::get_big_array_bf_size() const noexcept {
		return container.thingy_get_big_array_bf_size();
	}
	DCON_RELEASE_INLINE void thingy_fat_id::set_big_array_bf(int32_t i, bool v) const noexcept {
		container.thingy_set_big_array_bf(id, i, v);
	}
	DCON_RELEASE_INLINE void thingy_fat_id::resize_big_array_bf(uint32_t sz) const noexcept {
		container.thingy_resize_big_array_bf(sz);
	}
	DCON_RELEASE_INLINE bool thingy_fat_id::is_valid() const noexcept {
		return container.thingy_is_valid(id);
	}
	
	DCON_RELEASE_INLINE int32_t thingy_const_fat_id::get_some_value() const noexcept {
		return container.thingy_get_some_value(id);
	}
	DCON_RELEASE_INLINE bool thingy_const_fat_id::get_bf_value() const noexcept {
		return container.thingy_get_bf_value(id);
	}
	DCON_RELEASE_INLINE std::vector<float> const& thingy_const_fat_id::get_obj_value() const noexcept {
		return container.thingy_get_obj_value(id);
	}
	DCON_RELEASE_INLINE dcon::dcon_vv_const_fat_id<int16_t> thingy_const_fat_id::get_pooled_v() const noexcept {
		return container.thingy_get_pooled_v(id);
	}
	DCON_RELEASE_INLINE float thingy_const_fat_id::get_big_array(int32_t i) const noexcept {
		return container.thingy_get_big_array(id, i);
	}
	DCON_RELEASE_INLINE uint32_t thingy_const_fat_id::get_big_array_size() const noexcept {
		return container.thingy_get_big_array_size();
	}
	DCON_RELEASE_INLINE bool thingy_const_fat_id::get_big_array_bf(int32_t i) const noexcept {
		return container.thingy_get_big_array_bf(id, i);
	}
	DCON_RELEASE_INLINE uint32_t thingy_const_fat_id::get_big_array_bf_size() const noexcept {
		return container.thingy_get_big_array_bf_size();
	}
	DCON_RELEASE_INLINE bool thingy_const_fat_id::is_valid() const noexcept {
		return container.thingy_is_valid(id);
	}
	
	DCON_RELEASE_INLINE int32_t& car_fat_id::get_wheels() const noexcept {
		return container.car_get_wheels(id);
	}
	DCON_RELEASE_INLINE void car_fat_id::set_wheels(int32_t v) const noexcept {
		container.car_set_wheels(id, v);
	}
	DCON_RELEASE_INLINE float& car_fat_id::get_resale_value() const noexcept {
		return container.car_get_resale_value(id);
	}
	DCON_RELEASE_INLINE void car_fat_id::set_resale_value(float v) const noexcept {
		container.car_set_resale_value(id, v);
	}
	DCON_RELEASE_INLINE car_ownership_fat_id car_fat_id::get_car_ownership_as_owned_car() const noexcept {
		return car_ownership_fat_id(container, container.car_get_car_ownership_as_owned_car(id));
	}
	DCON_RELEASE_INLINE void car_fat_id::remove_car_ownership_as_owned_car() const noexcept {
		container.car_remove_car_ownership_as_owned_car(id);
	}
	DCON_RELEASE_INLINE car_ownership_fat_id car_fat_id::get_car_ownership() const noexcept {
		return car_ownership_fat_id(container, container.car_get_car_ownership(id));
	}
	DCON_RELEASE_INLINE void car_fat_id::remove_car_ownership() const noexcept {
		container.car_remove_car_ownership(id);
	}
	DCON_RELEASE_INLINE person_fat_id car_fat_id::get_owner_from_car_ownership() const noexcept {
		return person_fat_id(container, container.car_get_owner_from_car_ownership(id));
	}
	DCON_RELEASE_INLINE void car_fat_id::set_owner_from_car_ownership(person_id v) const noexcept {
		container.car_set_owner_from_car_ownership(id, v);
	}
	DCON_RELEASE_INLINE void car_fat_id::set_ownership_date_from_car_ownership(int32_t v) const noexcept {
		container.car_set_ownership_date_from_car_ownership(id, v);
	}
	DCON_RELEASE_INLINE int32_t car_fat_id::get_ownership_date_from_car_ownership() const noexcept {
		return container.car_get_ownership_date_from_car_ownership(id);
	}
	DCON_RELEASE_INLINE bool car_fat_id::is_valid() const noexcept {
		return container.car_is_valid(id);
	}
	
	DCON_RELEASE_INLINE int32_t car_const_fat_id::get_wheels() const noexcept {
		return container.car_get_wheels(id);
	}
	DCON_RELEASE_INLINE float car_const_fat_id::get_resale_value() const noexcept {
		return container.car_get_resale_value(id);
	}
	DCON_RELEASE_INLINE car_ownership_const_fat_id car_const_fat_id::get_car_ownership_as_owned_car() const noexcept {
		return car_ownership_const_fat_id(container, container.car_get_car_ownership_as_owned_car(id));
	}
	DCON_RELEASE_INLINE car_ownership_const_fat_id car_const_fat_id::get_car_ownership() const noexcept {
		return car_ownership_const_fat_id(container, container.car_get_car_ownership(id));
	}
	DCON_RELEASE_INLINE person_const_fat_id car_const_fat_id::get_owner_from_car_ownership() const noexcept {
		return person_const_fat_id(container, container.car_get_owner_from_car_ownership(id));
	}
	DCON_RELEASE_INLINE int32_t car_const_fat_id::get_ownership_date_from_car_ownership() const noexcept {
		return container.car_get_ownership_date_from_car_ownership(id);
	}
	DCON_RELEASE_INLINE bool car_const_fat_id::is_valid() const noexcept {
		return container.car_is_valid(id);
	}
	
	DCON_RELEASE_INLINE int32_t& person_fat_id::get_age() const noexcept {
		return container.person_get_age(id);
	}
	DCON_RELEASE_INLINE void person_fat_id::set_age(int32_t v) const noexcept {
		container.person_set_age(id, v);
	}
	template<typename T>
	DCON_RELEASE_INLINE void person_fat_id::for_each_car_ownership_as_owner(T&& func) const {
		container.person_for_each_car_ownership_as_owner(id, [&, t = this](car_ownership_id i){func(fatten(t->container, i));});
	}
	DCON_RELEASE_INLINE std::pair<car_ownership_id const*, car_ownership_id const*> person_fat_id::range_of_car_ownership_as_owner() const {
		return container.person_range_of_car_ownership_as_owner(id);
	}
	DCON_RELEASE_INLINE void person_fat_id::remove_all_car_ownership_as_owner() const noexcept {
		container.person_remove_all_car_ownership_as_owner(id);
	}
	DCON_RELEASE_INLINE internal::iterator_person_foreach_car_ownership_as_owner_generator person_fat_id::get_car_ownership_as_owner() const {
		return internal::iterator_person_foreach_car_ownership_as_owner_generator(container, id);
	}
	template<typename T>
	DCON_RELEASE_INLINE void person_fat_id::for_each_car_ownership(T&& func) const {
		container.person_for_each_car_ownership(id, [&, t = this](car_ownership_id i){func(fatten(t->container, i));});
	}
	DCON_RELEASE_INLINE std::pair<car_ownership_id const*, car_ownership_id const*> person_fat_id::range_of_car_ownership() const {
		return container.person_range_of_car_ownership(id);
	}
	DCON_RELEASE_INLINE void person_fat_id::remove_all_car_ownership() const noexcept {
		container.person_remove_all_car_ownership(id);
	}
	DCON_RELEASE_INLINE internal::iterator_person_foreach_car_ownership_as_owner_generator person_fat_id::get_car_ownership() const {
		return internal::iterator_person_foreach_car_ownership_as_owner_generator(container, id);
	}
	DCON_RELEASE_INLINE bool person_fat_id::is_valid() const noexcept {
		return container.person_is_valid(id);
	}
	
	DCON_RELEASE_INLINE int32_t person_const_fat_id::get_age() const noexcept {
		return container.person_get_age(id);
	}
	template<typename T>
	DCON_RELEASE_INLINE void person_const_fat_id::for_each_car_ownership_as_owner(T&& func) const {
		container.person_for_each_car_ownership_as_owner(id, [&, t = this](car_ownership_id i){func(fatten(t->container, i));});
	}
	DCON_RELEASE_INLINE std::pair<car_ownership_id const*, car_ownership_id const*> person_const_fat_id::range_of_car_ownership_as_owner() const {
		return container.person_range_of_car_ownership_as_owner(id);
	}
	DCON_RELEASE_INLINE internal::const_iterator_person_foreach_car_ownership_as_owner_generator person_const_fat_id::get_car_ownership_as_owner() const {
		return internal::const_iterator_person_foreach_car_ownership_as_owner_generator(container, id);
	}
	template<typename T>
	DCON_RELEASE_INLINE void person_const_fat_id::for_each_car_ownership(T&& func) const {
		container.person_for_each_car_ownership(id, [&, t = this](car_ownership_id i){func(fatten(t->container, i));});
	}
	DCON_RELEASE_INLINE std::pair<car_ownership_id const*, car_ownership_id const*> person_const_fat_id::range_of_car_ownership() const {
		return container.person_range_of_car_ownership(id);
	}
	DCON_RELEASE_INLINE internal::const_iterator_person_foreach_car_ownership_as_owner_generator person_const_fat_id::get_car_ownership() const {
		return internal::const_iterator_person_foreach_car_ownership_as_owner_generator(container, id);
	}
	DCON_RELEASE_INLINE bool person_const_fat_id::is_valid() const noexcept {
		return container.person_is_valid(id);
	}
	
	DCON_RELEASE_INLINE int32_t& car_ownership_fat_id::get_ownership_date() const noexcept {
		return container.car_ownership_get_ownership_date(id);
	}
	DCON_RELEASE_INLINE void car_ownership_fat_id::set_ownership_date(int32_t v) const noexcept {
		container.car_ownership_set_ownership_date(id, v);
	}
	DCON_RELEASE_INLINE person_fat_id car_ownership_fat_id::get_owner() const noexcept {
		return person_fat_id(container, container.car_ownership_get_owner(id));
	}
	DCON_RELEASE_INLINE void car_ownership_fat_id::set_owner(person_id val) const noexcept {
		container.car_ownership_set_owner(id, val);
	}
	DCON_RELEASE_INLINE bool car_ownership_fat_id::try_set_owner(person_id val) const noexcept {
		return container.car_ownership_try_set_owner(id, val);
	}
	DCON_RELEASE_INLINE car_fat_id car_ownership_fat_id::get_owned_car() const noexcept {
		return car_fat_id(container, container.car_ownership_get_owned_car(id));
	}
	DCON_RELEASE_INLINE void car_ownership_fat_id::set_owned_car(car_id val) const noexcept {
		container.car_ownership_set_owned_car(id, val);
	}
	DCON_RELEASE_INLINE bool car_ownership_fat_id::try_set_owned_car(car_id val) const noexcept {
		return container.car_ownership_try_set_owned_car(id, val);
	}
	DCON_RELEASE_INLINE bool car_ownership_fat_id::is_valid() const noexcept {
		return container.car_ownership_is_valid(id);
	}
	
	DCON_RELEASE_INLINE int32_t car_ownership_const_fat_id::get_ownership_date() const noexcept {
		return container.car_ownership_get_ownership_date(id);
	}
	DCON_RELEASE_INLINE person_const_fat_id car_ownership_const_fat_id::get_owner() const noexcept {
		return person_const_fat_id(container, container.car_ownership_get_owner(id));
	}
	DCON_RELEASE_INLINE car_const_fat_id car_ownership_const_fat_id::get_owned_car() const noexcept {
		return car_const_fat_id(container, container.car_ownership_get_owned_car(id));
	}
	DCON_RELEASE_INLINE bool car_ownership_const_fat_id::is_valid() const noexcept {
		return container.car_ownership_is_valid(id);
	}
	

	namespace internal {
		DCON_RELEASE_INLINE object_iterator_thingy::object_iterator_thingy(data_container& c, uint32_t i) noexcept : container(c), index(i) {
		}
		DCON_RELEASE_INLINE const_object_iterator_thingy::const_object_iterator_thingy(data_container const& c, uint32_t i) noexcept : container(c), index(i) {
		}
		DCON_RELEASE_INLINE object_iterator_thingy& object_iterator_thingy::operator++() noexcept {
			++index;
			return *this;
		}
		DCON_RELEASE_INLINE const_object_iterator_thingy& const_object_iterator_thingy::operator++() noexcept {
			++index;
			return *this;
		}
		DCON_RELEASE_INLINE object_iterator_thingy& object_iterator_thingy::operator--() noexcept {
			--index;
			return *this;
		}
		DCON_RELEASE_INLINE const_object_iterator_thingy& const_object_iterator_thingy::operator--() noexcept {
			--index;
			return *this;
		}
		
		DCON_RELEASE_INLINE object_iterator_car::object_iterator_car(data_container& c, uint32_t i) noexcept : container(c), index(i) {
		}
		DCON_RELEASE_INLINE const_object_iterator_car::const_object_iterator_car(data_container const& c, uint32_t i) noexcept : container(c), index(i) {
		}
		DCON_RELEASE_INLINE object_iterator_car& object_iterator_car::operator++() noexcept {
			++index;
			return *this;
		}
		DCON_RELEASE_INLINE const_object_iterator_car& const_object_iterator_car::operator++() noexcept {
			++index;
			return *this;
		}
		DCON_RELEASE_INLINE object_iterator_car& object_iterator_car::operator--() noexcept {
			--index;
			return *this;
		}
		DCON_RELEASE_INLINE const_object_iterator_car& const_object_iterator_car::operator--() noexcept {
			--index;
			return *this;
		}
		
		DCON_RELEASE_INLINE object_iterator_person::object_iterator_person(data_container& c, uint32_t i) noexcept : container(c), index(i) {
			while(container.person.m__index.vptr()[index] != person_id(person_id::value_base_t(index)) && index < container.person.size_used) {
				++index;
			}
		}
		DCON_RELEASE_INLINE const_object_iterator_person::const_object_iterator_person(data_container const& c, uint32_t i) noexcept : container(c), index(i) {
			while(container.person.m__index.vptr()[index] != person_id(person_id::value_base_t(index)) && index < container.person.size_used) {
				++index;
			}
		}
		DCON_RELEASE_INLINE object_iterator_person& object_iterator_person::operator++() noexcept {
			++index;
			while(container.person.m__index.vptr()[index] != person_id(person_id::value_base_t(index)) && index < container.person.size_used) {
				++index;
			}
			return *this;
		}
		DCON_RELEASE_INLINE const_object_iterator_person& const_object_iterator_person::operator++() noexcept {
			++index;
			while(container.person.m__index.vptr()[index] != person_id(person_id::value_base_t(index)) && index < container.person.size_used) {
				++index;
			}
			return *this;
		}
		DCON_RELEASE_INLINE object_iterator_person& object_iterator_person::operator--() noexcept {
			--index;
			while(container.person.m__index.vptr()[index] != person_id(person_id::value_base_t(index)) && index < container.person.size_used) {
				--index;
			}
			return *this;
		}
		DCON_RELEASE_INLINE const_object_iterator_person& const_object_iterator_person::operator--() noexcept {
			--index;
			while(container.person.m__index.vptr()[index] != person_id(person_id::value_base_t(index)) && index < container.person.size_used) {
				--index;
			}
			return *this;
		}
		
		DCON_RELEASE_INLINE iterator_person_foreach_car_ownership_as_owner::iterator_person_foreach_car_ownership_as_owner(data_container& c,  person_id fr) noexcept : container(c) {
			ptr = dcon::get_range(container.car_ownership.owner_storage, container.car_ownership.m_array_owner.vptr()[fr.index()]).first;
		}
		DCON_RELEASE_INLINE iterator_person_foreach_car_ownership_as_owner::iterator_person_foreach_car_ownership_as_owner(data_container& c, person_id fr, int) noexcept : container(c) {
			ptr = dcon::get_range(container.car_ownership.owner_storage, container.car_ownership.m_array_owner.vptr()[fr.index()]).second;
		}
		DCON_RELEASE_INLINE iterator_person_foreach_car_ownership_as_owner& iterator_person_foreach_car_ownership_as_owner::operator++() noexcept {
			++ptr;
			return *this;
		}
		DCON_RELEASE_INLINE iterator_person_foreach_car_ownership_as_owner& iterator_person_foreach_car_ownership_as_owner::operator--() noexcept {
			--ptr;
			return *this;
		}
		DCON_RELEASE_INLINE const_iterator_person_foreach_car_ownership_as_owner::const_iterator_person_foreach_car_ownership_as_owner(data_container const& c,  person_id fr) noexcept : container(c) {
			ptr = dcon::get_range(container.car_ownership.owner_storage, container.car_ownership.m_array_owner.vptr()[fr.index()]).first;
		}
		DCON_RELEASE_INLINE const_iterator_person_foreach_car_ownership_as_owner::const_iterator_person_foreach_car_ownership_as_owner(data_container const& c, person_id fr, int) noexcept : container(c) {
			ptr = dcon::get_range(container.car_ownership.owner_storage, container.car_ownership.m_array_owner.vptr()[fr.index()]).second;
		}
		DCON_RELEASE_INLINE const_iterator_person_foreach_car_ownership_as_owner& const_iterator_person_foreach_car_ownership_as_owner::operator++() noexcept {
			++ptr;
			return *this;
		}
		DCON_RELEASE_INLINE const_iterator_person_foreach_car_ownership_as_owner& const_iterator_person_foreach_car_ownership_as_owner::operator--() noexcept {
			--ptr;
			return *this;
		}
		
		DCON_RELEASE_INLINE object_iterator_car_ownership::object_iterator_car_ownership(data_container& c, uint32_t i) noexcept : container(c), index(i) {
		}
		DCON_RELEASE_INLINE const_object_iterator_car_ownership::const_object_iterator_car_ownership(data_container const& c, uint32_t i) noexcept : container(c), index(i) {
		}
		DCON_RELEASE_INLINE object_iterator_car_ownership& object_iterator_car_ownership::operator++() noexcept {
			++index;
			return *this;
		}
		DCON_RELEASE_INLINE const_object_iterator_car_ownership& const_object_iterator_car_ownership::operator++() noexcept {
			++index;
			return *this;
		}
		DCON_RELEASE_INLINE object_iterator_car_ownership& object_iterator_car_ownership::operator--() noexcept {
			--index;
			return *this;
		}
		DCON_RELEASE_INLINE const_object_iterator_car_ownership& const_object_iterator_car_ownership::operator--() noexcept {
			--index;
			return *this;
		}
		
	};


}

#undef DCON_RELEASE_INLINE
#ifdef _MSC_VER
#pragma warning( pop )
#endif

