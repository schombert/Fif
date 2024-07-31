#pragma once

//
// This file was automatically generated from: dcon_stuff_copy.txt
// EDIT AT YOUR OWN RISK; all changes will be lost upon regeneration
// NOT SUITABLE FOR USE IN CRITICAL SOFTWARE WHERE LIVES OR LIVELIHOODS DEPEND ON THE CORRECT OPERATION
//

#include <cstdint>
#include <cstddef>
#include <utility>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <cstring>
#include <string>


namespace fif {
std::string container_interface() {
return std::string() + "" 
" ptr(nil) global data-container "
" : set-container data-container ! ; "
" :export set_container ptr(nil) set-container ;  "
" :struct bit-proxy i32 bit ptr(i8) byte ; "
" :struct index-view ptr($0) wrapped ; "
" :s @ index-view($0) s: .wrapped @ ; "
" :s make-index-view ptr($0) s: make index-view($0) .wrapped! ; "
" :s ! bool bit-proxy s: .byte@ let byte .bit let bit 1 >i8 not bit shl byte @ and byte ! >i8 bit shl byte @ or byte ! ; "
" :s @ bit-proxy s: .byte@ let byte .bit let bit byte @ bit shr 1 >i8 and >bool ; "
" :s >index i32 s:  ; "
":struct thingy_id i16 ival ; "
":s >index thingy_id s: .ival >i32 -1 + ; "
":s >thingy_id i32 s: 1 + >i16 make thingy_id .ival! ; "
":s = thingy_id thingy_id s: .ival swap .ival = ; "
":s @ thingy_id s:  ; "
":s valid? thingy_id s: .ival 0 <> ; "
":struct car_id i16 ival ; "
":s >index car_id s: .ival >i32 -1 + ; "
":s >car_id i32 s: 1 + >i16 make car_id .ival! ; "
":s = car_id car_id s: .ival swap .ival = ; "
":s @ car_id s:  ; "
":s valid? car_id s: .ival 0 <> ; "
":struct person_id i8 ival ; "
":s >index person_id s: .ival >i32 -1 + ; "
":s >person_id i32 s: 1 + >i8 make person_id .ival! ; "
":s = person_id person_id s: .ival swap .ival = ; "
":s @ person_id s:  ; "
":s valid? person_id s: .ival 0 <> ; "
":struct car_ownership_id i16 ival ; "
":s >index car_ownership_id s: .ival >i32 -1 + ; "
":s >car_ownership_id i32 s: 1 + >i16 make car_ownership_id .ival! ; "
":s = car_ownership_id car_ownership_id s: .ival swap .ival = ; "
":s @ car_ownership_id s:  ; "
":s valid? car_ownership_id s: .ival 0 <> ; "
" :s some_value thingy_id s: >index " + std::to_string(sizeof(int32_t)) + " * " + std::to_string(offsetof(dcon::data_container, thingy) + offsetof(dcon::internal::thingy_class, m_some_value)+ offsetof(dcon::internal::thingy_class::dtype_some_value, values)) + " + data-container @ buf-add ptr-cast ptr(i32) ; "
" :s bf_value thingy_id s: >index  dup 8 / " + std::to_string(offsetof(dcon::data_container, thingy) + offsetof(dcon::internal::thingy_class, m_bf_value)+ offsetof(dcon::internal::thingy_class::dtype_bf_value, values)) + " + data-container @ buf-add ptr-cast ptr(i8) make bit-proxy .byte! swap >i32 8 mod swap .bit! ; "
" :s obj_value thingy_id s: >index " + std::to_string(sizeof(std::vector<float>)) + " * " + std::to_string(offsetof(dcon::data_container, thingy) + offsetof(dcon::internal::thingy_class, m_obj_value)+ offsetof(dcon::internal::thingy_class::dtype_obj_value, values)) + " + data-container @ buf-add ; "
" :struct vpool-pooled_v ptr(i32) content ;  "
" :s pooled_v thingy_id s: >index 4 * " + std::to_string(offsetof(dcon::data_container, thingy) + offsetof(dcon::internal::thingy_class, m_pooled_v)+ offsetof(dcon::internal::thingy_class::dtype_pooled_v, values)) + " + data-container @ buf-add ptr-cast ptr(i32) make vpool-pooled_v .content! ; "
" :s big_array thingy_id i32 s: >index swap >index swap " + std::to_string(offsetof(dcon::data_container, thingy) + offsetof(dcon::internal::thingy_class, m_big_array)+ offsetof(dcon::internal::thingy_class::dtype_big_array, values)) + " + data-container @ buf-add ptr-cast ptr(ptr(nil)) @ swap 1 + " + std::to_string((sizeof(float) + 64 - (sizeof(float) & 63) + sizeof(float) * 1200 + 64 - ((1200 * sizeof(float)) & 63))) + " * " + std::to_string(sizeof(float) + 64 - (sizeof(float) & 63)) + " swap buf-add swap " + std::to_string(sizeof(float)) + " * swap buf-add ptr-cast ptr(nil) ; "
" :s big_array_bf thingy_id i32 s: >index swap >index swap " + std::to_string(offsetof(dcon::data_container, thingy) + offsetof(dcon::internal::thingy_class, m_big_array_bf)+ offsetof(dcon::internal::thingy_class::dtype_big_array_bf, values)) + " + data-container @ buf-add ptr-cast ptr(ptr(nil)) @ swap 1 + " + std::to_string((64 + (1200 + 7) / 8 + 64 - (( (1200 + 7) / 8) & 63))) + " * 64 swap buf-add swap dup let tidx 8 / swap buf-add ptr-cast ptr(i8) make bit-proxy .byte! tidx >i32 8 mod swap .bit! ; "
" :s wheels car_id s: >index " + std::to_string(sizeof(int32_t)) + " * " + std::to_string(offsetof(dcon::data_container, car) + offsetof(dcon::internal::car_class, m_wheels)+ offsetof(dcon::internal::car_class::dtype_wheels, values)) + " + data-container @ buf-add ptr-cast ptr(i32) ; "
" :s resale_value car_id s: >index " + std::to_string(sizeof(float)) + " * " + std::to_string(offsetof(dcon::data_container, car) + offsetof(dcon::internal::car_class, m_resale_value)+ offsetof(dcon::internal::car_class::dtype_resale_value, values)) + " + data-container @ buf-add ptr-cast ptr(f32) ; "
" :s age person_id s: >index " + std::to_string(sizeof(int32_t)) + " * " + std::to_string(offsetof(dcon::data_container, person) + offsetof(dcon::internal::person_class, m_age)+ offsetof(dcon::internal::person_class::dtype_age, values)) + " + data-container @ buf-add ptr-cast ptr(i32) ; "
" :s ownership_date car_ownership_id s: >index " + std::to_string(sizeof(int32_t)) + " * " + std::to_string(offsetof(dcon::data_container, car_ownership) + offsetof(dcon::internal::car_ownership_class, m_ownership_date)+ offsetof(dcon::internal::car_ownership_class::dtype_ownership_date, values)) + " + data-container @ buf-add ptr-cast ptr(i32) ; "
" :s car_ownership-owner person_id s: >index " + std::to_string(sizeof(dcon::car_ownership_id)) + " * " + std::to_string(offsetof(dcon::data_container, car_ownership) + offsetof(dcon::internal::car_ownership_class, m_owner)+ offsetof(dcon::internal::car_ownership_class::dtype_owner, values)) + " + data-container @ buf-add ptr-cast ptr(person) make-index-view ; "
" :struct vpool-owner ptr(i32) content ;  "
" :s owner car_ownership_id s: >index 4 * " + std::to_string(offsetof(dcon::data_container, car_ownership) + offsetof(dcon::internal::car_ownership_class, m_array_owner)+ offsetof(dcon::internal::car_ownership_class::dtype_array_owner, values)) + " + data-container @ buf-add ptr-cast ptr(i32) make vpool-owner .content! ; "
" :s car_ownership-owned_car car_id s: >index >car_ownership_id ; "
" :s owned_car car_ownership_id s: >index >car_id ; "
;
} 
}


