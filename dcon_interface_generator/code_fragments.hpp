#pragma once

#include <optional>
#include <cstring>
#include "source_builder.hpp"
#include "parsing.hpp"

std::string type_to_fif_type(std::string const& tin);
std::string offset_of_member_container(std::string const& object_name, std::string const& member_name);
std::string array_member_leading_padding(std::string const& member_type, size_t raw_size, bool is_bitfield);
std::string array_member_trailing_padding(std::string const& member_type, size_t raw_size, bool is_bitfield);
std::string array_member_row_size(std::string const& member_type, size_t raw_size, bool is_bitfield);
std::string offset_of_array_member_container(std::string const& object_name, std::string const& member_name);

basic_builder& make_load_record(basic_builder& o, file_def const & fd);
std::string make_id_definition(std::string const& type_name, std::string const& underlying_type);
basic_builder& make_value_to_vector_type(basic_builder& o, std::string const& qualified_name);

enum class struct_padding { none, fixed };

basic_builder& make_member_container(basic_builder& o,
	std::string const& member_name, std::string const& type_name, std::string const& size, struct_padding pad, bool is_expandable,
	std::optional<std::string> const& special_fill = std::optional<std::string>(), int32_t multiplicity = 1);
basic_builder& make_array_member_container(basic_builder& o,
	std::string const& member_name, std::string const& type_name, size_t raw_size,
	bool is_expandable, bool is_bitfield);

std::string expand_size_to_fill_cacheline_calculation(std::string const& member_type, size_t base_size);

basic_builder& make_erasable_object_constructor(basic_builder& o, relationship_object_def const& obj, std::string const& name, size_t size);
basic_builder& make_other_object_constructor(basic_builder& o, relationship_object_def const& obj, std::string const& name, size_t size);

basic_builder& make_pop_back(basic_builder& o, relationship_object_def const& obj);
basic_builder& make_object_resize(basic_builder& o, relationship_object_def const& obj);
basic_builder& make_compactable_delete(basic_builder& o, relationship_object_def const& obj);
basic_builder& make_non_erasable_create(basic_builder& o, relationship_object_def const& obj);
basic_builder& make_erasable_delete(basic_builder& o, relationship_object_def const& obj);
basic_builder& make_clearing_delete(basic_builder& o, relationship_object_def const& cob);
basic_builder& make_erasable_create(basic_builder& o, relationship_object_def const& obj);
basic_builder& make_internal_move_relationship(basic_builder& o, relationship_object_def const& obj);
basic_builder& make_relation_try_create(basic_builder& o, relationship_object_def const& cob);
basic_builder& make_relation_force_create(basic_builder& o, relationship_object_def const& cob);
basic_builder& make_iterate_over_objects(basic_builder& o, relationship_object_def const& obj);

basic_builder& make_const_fat_id(basic_builder& o, relationship_object_def const& obj, file_def const& parsed_file);
basic_builder& make_fat_id(basic_builder& o, relationship_object_def const& obj, file_def const& parsed_file);
basic_builder& make_const_fat_id_impl(basic_builder& o, relationship_object_def const& obj, file_def const& parsed_file);
basic_builder& make_fat_id_impl(basic_builder& o, relationship_object_def const& obj, file_def const& parsed_file);

basic_builder& make_composite_key_declarations(basic_builder& o, std::string const& obj_name, composite_index_def const& cc);
basic_builder& make_composite_key_getter(basic_builder& o, std::string const& obj_name, composite_index_def const& cc, relationship_object_def const& cob);

basic_builder& make_object_member_declarations(basic_builder& o, file_def const& parsed_file, relationship_object_def const& obj,
	bool add_prefix, bool declaration_mode, std::string const& namesp, bool const_mode);

std::optional<std::string> to_fat_index_type(file_def const& parsed_file, std::string const& original_name, bool is_const);

basic_builder& object_iterator_declaration(basic_builder& o, relationship_object_def const& obj);
basic_builder& object_iterator_implementation(basic_builder& o, relationship_object_def const& obj);

basic_builder& relation_iterator_foreach_as_generator(basic_builder& o, relationship_object_def const& obj, relationship_object_def const& rel, related_object const& l);
basic_builder& relation_iterator_foreach_as_declaration(basic_builder& o, relationship_object_def const& obj, relationship_object_def const& rel, related_object const& l);
basic_builder& relation_iterator_foreach_as_implementation(basic_builder& o, relationship_object_def const& obj, relationship_object_def const& rel, related_object const& l);
