#pragma once
#include <stdint.h>
#include <elf.h>
#include <string>
#include <map>
#include <vector>
#include <filesystem>

#include "common.h"

const uint32_t DWARF_32BIT_FORMAT = 0x01;
const uint32_t DWARF_64BIT_FORMAT = 0x02;

// DWARF5 P199 7.5.1 Unit Headers
enum
{
    DW_UT_compile       = 0x01,
    DW_UT_type          = 0x02,
    DW_UT_partial       = 0x03,
    DW_UT_skeleton      = 0x04,
    DW_UT_split_compile = 0x05,
    DW_UT_split_type    = 0x06,
    DW_UT_lo_user       = 0x80,
    DW_UT_hi_user       = 0xff,
};

enum
{
    DW_TAG_array_type               = 0x01,
    DW_TAG_class_type               = 0x02,
    DW_TAG_entry_point              = 0x03,
    DW_TAG_enumeration_type         = 0x04,
    DW_TAG_formal_parameter         = 0x05,
    DW_TAG_imported_declaration     = 0x08,
    DW_TAG_label                    = 0x0a,
    DW_TAG_lexical_block            = 0x0b,
    DW_TAG_member                   = 0x0d,
    DW_TAG_pointer_type             = 0x0f,
    DW_TAG_reference_type           = 0x10,
    DW_TAG_compile_unit             = 0x11,
    DW_TAG_string_type              = 0x12,
    DW_TAG_structure_type           = 0x13,
    DW_TAG_subroutine_type          = 0x15,
    DW_TAG_typedef                  = 0x16,
    DW_TAG_union_type               = 0x17,
    DW_TAG_unspecified_parameters   = 0x18,
    DW_TAG_variant                  = 0x19,
    DW_TAG_common_block             = 0x1a,
    DW_TAG_common_inclusion         = 0x1b,
    DW_TAG_inheritance              = 0x1c,
    DW_TAG_inlined_subroutine       = 0x1d,
    DW_TAG_module                   = 0x1e,
    DW_TAG_ptr_to_member_type       = 0x1f,
    DW_TAG_set_type                 = 0x20,
    DW_TAG_subrange_type            = 0x21,
    DW_TAG_with_stmt                = 0x22,
    DW_TAG_access_declaration       = 0x23,
    DW_TAG_base_type                = 0x24,
    DW_TAG_catch_block              = 0x25,
    DW_TAG_const_type               = 0x26,
    DW_TAG_constant                 = 0x27,
    DW_TAG_enumerator               = 0x28,
    DW_TAG_file_type                = 0x29,
    DW_TAG_friend                   = 0x2a,
    DW_TAG_namelist                 = 0x2b,
    DW_TAG_namelist_item            = 0x2c,
    DW_TAG_packed_type              = 0x2d,
    DW_TAG_subprogram               = 0x2e,
    DW_TAG_template_type_parameter  = 0x2f,
    DW_TAG_template_value_parameter = 0x30,
    DW_TAG_thrown_type              = 0x31,
    DW_TAG_try_block                = 0x32,
    DW_TAG_variant_part             = 0x33,
    DW_TAG_variable                 = 0x34,
    DW_TAG_volatile_type            = 0x35,
    DW_TAG_dwarf_procedure          = 0x36,
    DW_TAG_restrict_type            = 0x37,
    DW_TAG_interface_type           = 0x38,
    DW_TAG_namespace                = 0x39,
    DW_TAG_imported_module          = 0x3a,
    DW_TAG_unspecified_type         = 0x3b,
    DW_TAG_partial_unit             = 0x3c,
    DW_TAG_imported_unit            = 0x3d,
    DW_TAG_condition                = 0x3f,
    DW_TAG_shared_type              = 0x40,
    DW_TAG_type_unit                = 0x41,
    DW_TAG_rvalue_reference_type    = 0x42,
    DW_TAG_template_alias           = 0x43,

    DW_TAG_lo_user = 0x4080,
    DW_TAG_hi_user = 0xffff,
};

enum
{
	DW_LANG_C89            = 0x0001, // 0
	DW_LANG_C              = 0x0002, // 0
	DW_LANG_Ada83          = 0x0003, // 1
	DW_LANG_C_plus_plus    = 0x0004, // 0
	DW_LANG_Cobol74        = 0x0005, // 1
	DW_LANG_Cobol85        = 0x0006, // 1
	DW_LANG_Fortran77      = 0x0007, // 1
	DW_LANG_Fortran90      = 0x0008, // 1
	DW_LANG_Pascal83       = 0x0009, // 1
	DW_LANG_Modula2        = 0x000a, // 1
	DW_LANG_Java           = 0x000b, // 0
	DW_LANG_C99            = 0x000c, // 0
	DW_LANG_Ada95          = 0x000d, // 1
	DW_LANG_Fortran95      = 0x000e, // 1
	DW_LANG_PLI            = 0x000f, // 1
	DW_LANG_ObjC           = 0x0010, // 0
	DW_LANG_ObjC_plus_plus = 0x0011, // 0
	DW_LANG_UPC            = 0x0012, // 0
	DW_LANG_D              = 0x0013, // 0
	DW_LANG_Python         = 0x0014, // 0
	DW_LANG_OpenCL         = 0x0015, // 0
	DW_LANG_Go             = 0x0016, // 0
	DW_LANG_Modula3        = 0x0017, // 1
	DW_LANG_Haskell        = 0x0018, // 0
	DW_LANG_C_plus_plus_03 = 0x0019, // 0
	DW_LANG_C_plus_plus_11 = 0x001a, // 0
	DW_LANG_OCaml          = 0x001b, // 0
	DW_LANG_Rust           = 0x001c, // 0
	DW_LANG_C11            = 0x001d, // 0
	DW_LANG_Swift          = 0x001e, // 0
	DW_LANG_Julia          = 0x001f, // 1
	DW_LANG_Dylan          = 0x0020, // 0
	DW_LANG_C_plus_plus_14 = 0x0021, // 0
	DW_LANG_Fortran03      = 0x0022, // 1
	DW_LANG_Fortran08      = 0x0023, // 1
	DW_LANG_RenderScript   = 0x0024, // 0
	DW_LANG_BLISS          = 0x0025, // 0
	DW_LANG_lo_user        = 0x8000,
	DW_LANG_hi_user        = 0xFFFF,
};

// ============================================================================
// Attribute Code define
// ============================================================================
enum
{
    DW_AT_sibling              = 0x01,   // reference
    DW_AT_location             = 0x02,   // exprloc, loclistptr
    DW_AT_name                 = 0x03,   // string
    DW_AT_ordering             = 0x09,   // constant
    DW_AT_byte_size            = 0x0b,   // constant, exprloc, reference
    DW_AT_bit_offset           = 0x0c,   // constant, exprloc, reference
    DW_AT_bit_size             = 0x0d,   // constant, exprloc, reference
    DW_AT_stmt_list            = 0x10,   // lineptr
    DW_AT_low_pc               = 0x11,   // address
    DW_AT_high_pc              = 0x12,   // address, constant
    DW_AT_language             = 0x13,   // constant
    DW_AT_discr                = 0x15,   // reference
    DW_AT_discr_value          = 0x16,   // constant
    DW_AT_visibility           = 0x17,   // constant
    DW_AT_import               = 0x18,   // reference
    DW_AT_string_length        = 0x19,   // exprloc, loclistptr
    DW_AT_common_reference     = 0x1a,   // reference
    DW_AT_comp_dir             = 0x1b,   // string
    DW_AT_const_value          = 0x1c,   // block, constant, string
    DW_AT_containing_type      = 0x1d,   // reference
    DW_AT_default_value        = 0x1e,   // reference
    DW_AT_inline               = 0x20,   // constant
    DW_AT_is_optional          = 0x21,   // flag
    DW_AT_lower_bound          = 0x22,   // constant, exprloc, reference
    DW_AT_producer             = 0x25,   // string
    DW_AT_prototyped           = 0x27,   // flag
    DW_AT_return_addr          = 0x2a,   // exprloc, loclistptr
    DW_AT_start_scope          = 0x2c,   // Constant, rangelistptr
    DW_AT_bit_stride           = 0x2e,   // constant, exprloc, reference
    DW_AT_upper_bound          = 0x2f,   // constant, exprloc, reference
    DW_AT_abstract_origin      = 0x31,   // reference
    DW_AT_accessibility        = 0x32,   // constant
    DW_AT_address_class        = 0x33,   // constant
    DW_AT_artificial           = 0x34,   // flag
    DW_AT_base_types           = 0x35,   // reference
    DW_AT_calling_convention   = 0x36,   // constant
    DW_AT_count                = 0x37,   // constant, exprloc, reference
    DW_AT_data_member_location = 0x38,   // constant, exprloc, loclistptr
    DW_AT_decl_column          = 0x39,   // constant
    DW_AT_decl_file            = 0x3a,   // constant
    DW_AT_decl_line            = 0x3b,   // constant
    DW_AT_declaration          = 0x3c,   // flag
    DW_AT_discr_list           = 0x3d,   // block
    DW_AT_encoding             = 0x3e,   // constant
    DW_AT_external             = 0x3f,   // flag
    DW_AT_frame_base           = 0x40,   // exprloc, loclistptr
    DW_AT_friend               = 0x41,   // reference
    DW_AT_identifier_case      = 0x42,   // constant
    DW_AT_macro_info           = 0x43,   // macptr
    DW_AT_namelist_item        = 0x44,   // reference
    DW_AT_priority             = 0x45,   // reference
    DW_AT_segment              = 0x46,   // exprloc, loclistptr
    DW_AT_specification        = 0x47,   // reference
    DW_AT_static_link          = 0x48,   // exprloc, loclistptr
    DW_AT_type                 = 0x49,   // reference
    DW_AT_use_location         = 0x4a,   // exprloc, loclistptr
    DW_AT_variable_parameter   = 0x4b,   // flag
    DW_AT_virtuality           = 0x4c,   // constant
    DW_AT_vtable_elem_location = 0x4d,   // exprloc, loclistptr
    DW_AT_allocated            = 0x4e,   // constant, exprloc, reference
    DW_AT_associated           = 0x4f,   // constant, exprloc, reference
    DW_AT_data_location        = 0x50,   // exprloc
    DW_AT_byte_stride          = 0x51,   // constant, exprloc, reference
    DW_AT_entry_pc             = 0x52,   // address
    DW_AT_use_UTF8             = 0x53,   // flag
    DW_AT_extension            = 0x54,   // reference
    DW_AT_ranges               = 0x55,   // rangelistptr
    DW_AT_trampoline           = 0x56,   // address, flag, reference, string
    DW_AT_call_column          = 0x57,   // constant
    DW_AT_call_file            = 0x58,   // constant
    DW_AT_call_line            = 0x59,   // constant
    DW_AT_description          = 0x5a,   // string
    DW_AT_binary_scale         = 0x5b,   // constant
    DW_AT_decimal_scale        = 0x5c,   // constant
    DW_AT_small                = 0x5d,   // reference
    DW_AT_decimal_sign         = 0x5e,   // constant
    DW_AT_digit_count          = 0x5f,   // constant
    DW_AT_picture_string       = 0x60,   // string
    DW_AT_mutable              = 0x61,   // flag
    DW_AT_threads_scaled       = 0x62,   // flag
    DW_AT_explicit             = 0x63,   // flag
    DW_AT_object_pointer       = 0x64,   // reference
    DW_AT_endianity            = 0x65,   // constant
    DW_AT_elemental            = 0x66,   // flag
    DW_AT_pure                 = 0x67,   // flag
    DW_AT_recursive            = 0x68,   // flag
    DW_AT_signature            = 0x69,   // reference
    DW_AT_main_subprogram      = 0x6a,   // flag
    DW_AT_data_bit_offset      = 0x6b,   // constant
    DW_AT_const_expr           = 0x6c,   // flag
    DW_AT_enum_class           = 0x6d,   // flag
    DW_AT_linkage_name         = 0x6e,   // string
    DW_AT_lo_user              = 0x2000, // ---

    // see https://sourceware.org/elfutils/DwarfExtensions
    DW_AT_MIPS_linkage_name = 0x2007,
    // GNU Extensions
    sf_names                       = 0x2101,
    src_info                       = 0x2102,
    mac_info                       = 0x2103,
    src_coords                     = 0x2104,
    body_begin                     = 0x2105,
    body_end                       = 0x2106,
    GNU_vector                     = 0x2107,
    GNU_odr_signature              = 0x210f,
    GNU_template_name              = 0x2110,
    GNU_call_site_value            = 0x2111,
    GNU_call_site_data_value       = 0x2112,
    GNU_call_site_target           = 0x2113,
    GNU_call_site_target_clobbered = 0x2114,
    GNU_tail_call                  = 0x2115,
    GNU_all_tail_call_sites        = 0x2116,
    GNU_all_call_sites             = 0x2117,
    GNU_all_source_call_sites      = 0x2118,
    GNU_macros                     = 0x2119,
    GNU_deleted                    = 0x211a,
    GNU_dwo_name                   = 0x2130,
    GNU_dwo_id                     = 0x2131,
    GNU_ranges_base                = 0x2132,
    GNU_addr_base                  = 0x2133,
    GNU_pubnames                   = 0x2134,
    GNU_pubtypes                   = 0x2135,
    GNU_discriminator              = 0x2136,
    GNU_locviews                   = 0x2137,
    GNU_entry_view                 = 0x2138,

    DW_AT_hi_user = 0x3fff // ---
};

// ============================================================================
// Form Code define
// ============================================================================
enum {
    DW_FORM_addr           = 0x01, // address
    DW_FORM_block2         = 0x03, // block
    DW_FORM_block4         = 0x04, // block
    DW_FORM_data2          = 0x05, // constant
    DW_FORM_data4          = 0x06, // constant
    DW_FORM_data8          = 0x07, // constant
    DW_FORM_string         = 0x08, // string
    DW_FORM_block          = 0x09, // block
    DW_FORM_block1         = 0x0a, // block
    DW_FORM_data1          = 0x0b, // constant
    DW_FORM_flag           = 0x0c, // flag
    DW_FORM_sdata          = 0x0d, // constant
    DW_FORM_strp           = 0x0e, // string
    DW_FORM_udata          = 0x0f, // constant
    DW_FORM_ref_addr       = 0x10, // reference
    DW_FORM_ref1           = 0x11, // reference
    DW_FORM_ref2           = 0x12, // reference
    DW_FORM_ref4           = 0x13, // reference
    DW_FORM_ref8           = 0x14, // reference
    DW_FORM_ref_udata      = 0x15, // reference
    DW_FORM_indirect       = 0x16, // (see Section 7.5.3)
    DW_FORM_sec_offset     = 0x17, // lineptr, loclistptr, macptr, rangelistptr
    DW_FORM_exprloc        = 0x18, // exprloc
    DW_FORM_flag_present   = 0x19, // flag
    DW_FORM_strx           = 0x1a, // string(DWARF5～)
    DW_FORM_addrx          = 0x1b, // addresss(DWARF5～)
    DW_FORM_ref_sup4       = 0x1c, // reference(DWARF5～)
    DW_FORM_strp_sup       = 0x1d, // string(DWARF5～)
    DW_FORM_data16         = 0x1e, // constant(DWARF5～)
    DW_FORM_line_strp      = 0x1f, // string(DWARF5～)
    DW_FORM_ref_sig8       = 0x20, // reference
    DW_FORM_implicit_const = 0x21, // constant
    DW_FORM_loclistx       = 0x22, // loclist
    DW_FORM_rnglistx       = 0x23, // rnglist
    DW_FORM_ref_sup8       = 0x24, // reference
    DW_FORM_strx1          = 0x25, // string
    DW_FORM_strx2          = 0x26, // string
    DW_FORM_strx3          = 0x27, // string
    DW_FORM_strx4          = 0x28, // string
    DW_FORM_addrx1         = 0x29, // address
    DW_FORM_addrx2         = 0x2a, // address
    DW_FORM_addrx3         = 0x2b, // address
    DW_FORM_addrx4         = 0x2c, // address
};

enum
{
	DW_CHILDREN_no  = 0x00,
	DW_CHILDREN_yes = 0x01,
};

// ============================================================================
// DW_OP
// ============================================================================
enum
{
	DW_OP_addr                = 0x03,
	DW_OP_deref               = 0x06,
	DW_OP_const1u             = 0x08,
	DW_OP_const1s             = 0x09,
	DW_OP_const2u             = 0x0a,
	DW_OP_const2s             = 0x0b,
	DW_OP_const4u             = 0x0c,
	DW_OP_const4s             = 0x0d,
	DW_OP_const8u             = 0x0e,
	DW_OP_const8s             = 0x0f,
	DW_OP_constu              = 0x10,
	DW_OP_consts              = 0x11,
	DW_OP_dup                 = 0x12,
	DW_OP_drop                = 0x13,
	DW_OP_over                = 0x14,
	DW_OP_pick                = 0x15,
	DW_OP_swap                = 0x16,
	DW_OP_rot                 = 0x17,
	DW_OP_xderef              = 0x18,
	DW_OP_abs                 = 0x19,
	DW_OP_and                 = 0x1a,
	DW_OP_div                 = 0x1b,
	DW_OP_minus               = 0x1c,
	DW_OP_mod                 = 0x1d,
	DW_OP_mul                 = 0x1e,
	DW_OP_neg                 = 0x1f,
	DW_OP_not                 = 0x20,
	DW_OP_or                  = 0x21,
	DW_OP_plus                = 0x22,
	DW_OP_plus_uconst         = 0x23,
	DW_OP_shl                 = 0x24,
	DW_OP_shr                 = 0x25,
	DW_OP_shra                = 0x26,
	DW_OP_xor                 = 0x27,
	DW_OP_skip                = 0x2f,
	DW_OP_bra                 = 0x28,
	DW_OP_eq                  = 0x29,
	DW_OP_ge                  = 0x2a,
	DW_OP_gt                  = 0x2b,
	DW_OP_le                  = 0x2c,
	DW_OP_lt                  = 0x2d,
	DW_OP_ne                  = 0x2e,
	DW_OP_lit0                = 0x30,
	DW_OP_lit1                = 0x31,
	DW_OP_lit2                = 0x32,
	DW_OP_lit3                = 0x33,
	DW_OP_lit4                = 0x34,
	DW_OP_lit5                = 0x35,
	DW_OP_lit6                = 0x36,
	DW_OP_lit7                = 0x37,
	DW_OP_lit8                = 0x38,
	DW_OP_lit9                = 0x39,
	DW_OP_lit10               = 0x3A,
	DW_OP_lit11               = 0x3B,
	DW_OP_lit12               = 0x3C,
	DW_OP_lit13               = 0x3D,
	DW_OP_lit14               = 0x3E,
	DW_OP_lit15               = 0x3F,
	DW_OP_lit16               = 0x40,
	DW_OP_lit17               = 0x41,
	DW_OP_lit18               = 0x42,
	DW_OP_lit19               = 0x43,
	DW_OP_lit20               = 0x44,
	DW_OP_lit21               = 0x45,
	DW_OP_lit22               = 0x46,
	DW_OP_lit23               = 0x47,
	DW_OP_lit24               = 0x48,
	DW_OP_lit25               = 0x49,
	DW_OP_lit26               = 0x4A,
	DW_OP_lit27               = 0x4B,
	DW_OP_lit28               = 0x4C,
	DW_OP_lit29               = 0x4D,
	DW_OP_lit30               = 0x4E,
	DW_OP_lit31               = 0x4F,
	DW_OP_reg0                = 0x50,
	DW_OP_reg1                = 0x51,
	DW_OP_reg2                = 0x52,
	DW_OP_reg3                = 0x53,
	DW_OP_reg4                = 0x54,
	DW_OP_reg5                = 0x55,
	DW_OP_reg6                = 0x56,
	DW_OP_reg7                = 0x57,
	DW_OP_reg8                = 0x58,
	DW_OP_reg9                = 0x59,
	DW_OP_reg10               = 0x5A,
	DW_OP_reg11               = 0x5B,
	DW_OP_reg12               = 0x5C,
	DW_OP_reg13               = 0x5D,
	DW_OP_reg14               = 0x5E,
	DW_OP_reg15               = 0x5F,
	DW_OP_reg16               = 0x60,
	DW_OP_reg17               = 0x61,
	DW_OP_reg18               = 0x62,
	DW_OP_reg19               = 0x63,
	DW_OP_reg20               = 0x64,
	DW_OP_reg21               = 0x65,
	DW_OP_reg22               = 0x66,
	DW_OP_reg23               = 0x67,
	DW_OP_reg24               = 0x68,
	DW_OP_reg25               = 0x69,
	DW_OP_reg26               = 0x6A,
	DW_OP_reg27               = 0x6B,
	DW_OP_reg28               = 0x6C,
	DW_OP_reg29               = 0x6D,
	DW_OP_reg30               = 0x6E,
	DW_OP_reg31               = 0x6f,
	DW_OP_breg0               = 0x70,
	DW_OP_breg1               = 0x71,
	DW_OP_breg2               = 0x72,
	DW_OP_breg3               = 0x73,
	DW_OP_breg4               = 0x74,
	DW_OP_breg5               = 0x75,
	DW_OP_breg6               = 0x76,
	DW_OP_breg7               = 0x77,
	DW_OP_breg8               = 0x78,
	DW_OP_breg9               = 0x79,
	DW_OP_breg10              = 0x7A,
	DW_OP_breg11              = 0x7B,
	DW_OP_breg12              = 0x7C,
	DW_OP_breg13              = 0x7D,
	DW_OP_breg14              = 0x7E,
	DW_OP_breg15              = 0x7F,
	DW_OP_breg16              = 0x80,
	DW_OP_breg17              = 0x81,
	DW_OP_breg18              = 0x82,
	DW_OP_breg19              = 0x83,
	DW_OP_breg20              = 0x84,
	DW_OP_breg21              = 0x85,
	DW_OP_breg22              = 0x86,
	DW_OP_breg23              = 0x87,
	DW_OP_breg24              = 0x88,
	DW_OP_breg25              = 0x89,
	DW_OP_breg26              = 0x8A,
	DW_OP_breg27              = 0x8B,
	DW_OP_breg28              = 0x8C,
	DW_OP_breg29              = 0x8D,
	DW_OP_breg30              = 0x8E,
	DW_OP_breg31              = 0x8F,
	DW_OP_regx                = 0x90,
	DW_OP_fbreg               = 0x91,
	DW_OP_bregx               = 0x92,
	DW_OP_piece               = 0x93,
	DW_OP_deref_size          = 0x94,
	DW_OP_xderef_size         = 0x95,
	DW_OP_nop                 = 0x96,
	DW_OP_push_object_address = 0x97,
	DW_OP_call2               = 0x98,
	DW_OP_call4               = 0x99,
	DW_OP_call_ref            = 0x9a,
	DW_OP_form_tls_address    = 0x9b,
	DW_OP_call_frame_cfa      = 0x9c,
	DW_OP_bit_piece           = 0x9d,
	DW_OP_implicit_value      = 0x9e,
	DW_OP_stack_value         = 0x9f,
	DW_OP_lo_user             = 0xe0,
	DW_OP_hi_user             = 0xff,
};

// DWARF5 P237 Table 7.27
// Line number header entry format encodings
enum
{
    DW_LNCT_path            = 0x1,
    DW_LNCT_directory_index = 0x2,
    DW_LNCT_timestamp       = 0x3,
    DW_LNCT_size            = 0x4,
    DW_LNCT_MD5             = 0x5,
    DW_LNCT_lo_user         = 0x2000,
    DW_LNCT_hi_user         = 0x3f,
};

enum
{
	DW_LNE_end_sequence      = 0x01,
	DW_LNE_set_address       = 0x02,
	DW_LNE_define_file       = 0x03,
	DW_LNE_set_discriminator = 0x04,
	DW_LNE_lo_user           = 0x80,
	DW_LNE_hi_user           = 0xFF,
};

enum
{
	DW_LNS_copy               = 0x01,
	DW_LNS_advance_pc         = 0x02,
	DW_LNS_advance_line       = 0x03,
	DW_LNS_set_file           = 0x04,
	DW_LNS_set_column         = 0x05,
	DW_LNS_negate_stmt        = 0x06,
	DW_LNS_set_basic_block    = 0x07,
	DW_LNS_const_add_pc       = 0x08,
	DW_LNS_fixed_advance_pc   = 0x09,
	DW_LNS_set_prologue_end   = 0x0A,
	DW_LNS_set_epilogue_begin = 0x0B,
	DW_LNS_set_isa            = 0x0C,
};
// Compilation Unit Header
// see 7.5.1.1 Compilation Unit Header
// this struct include DWARF Format(32/64), so not same as header size
typedef struct
{
	uint64_t UnitLength;
	uint8_t DwarfFormat;
	uint16_t Version;
	uint8_t UnitType;           // DWARF5 or later
	uint32_t DebugAbbrevOffset;
	uint8_t AddressSize;
	uint64_t UnitID;
	uint64_t TypeSignature;
	uint64_t TypeOffset;
} DwarfCuHdr;

// Line Number Program Header
// see 6.2.4 The Line Number Program Header
struct FileNameInfo
{
    std::string Name;
    uint64_t DirIdx;
    uint64_t LastModified;
    uint64_t Size;
};

struct EntryFormat
{
    uint64_t TypeCode;
    uint64_t FormCode;
} ;

// this struct include DWARF Format(32/64), so not same as header size
struct DwarfLineInfoHdr
{
    uint64_t UnitLength;
    uint8_t DwarfFormat;
    uint16_t Version;
    uint32_t HeaderLength;
    uint8_t AddressSize;                            // only verion5 or later
    uint8_t SegmentSelectorSize;                    // only verion5 or later
    uint8_t MinInstLength;
    uint8_t MaxInstLength;                          // only version4 or later
    uint8_t DefaultIsStmt;
    int8_t LineBase;
    uint8_t LineRange;
    uint8_t OpcodeBase;
    std::vector<uint8_t> StdOpcodeLengths;
    uint8_t DirectoryEntryFormatCount;              // only verion5 or later
    std::vector<EntryFormat> DirectoryEntryFormats; // only verion5 or later
    uint64_t DirectoriesCount;                      // only verion5 or later
    std::vector<std::string> Directories;           // only verion5 or later
    uint8_t FileNameEntryFormatCount;               // only verion5 or later
    std::vector<EntryFormat> FileNameEntryFormats;  // only verion5 or later
    uint64_t FileNamesCount;                        // only verion5 or later
    std::vector<std::string> IncludeDirs;           // only verion5 or later
    std::vector<FileNameInfo> Files;
};

struct DwarfFuncInfo
{
    std::string SrcFilePath;
    std::string Name;
    std::string LinkageName;
    uint64_t Addr;
    uint32_t Size;
};

class DwarfCuDebugInfo
{
public:
    bool IsRust()
    {
	    return (Language == "Rust");
    };
    std::string FilePath()
    {
        std::string relPath = StringHelper::strprintf("%s/%s", CompileDir, FileName);
        std::filesystem::path absPath = std::filesystem::absolute(relPath);
        return absPath.string();
    };

public:
    std::string FileName;
    std::string Producer;
    std::string Language;
    std::string CompileDir;
    std::map<uint64_t, DwarfFuncInfo> Funcs;
};

struct DwarfSegmentInfo
{
    uint64_t Address;
    uint64_t Length;
};

struct DwarfArangeInfoHdr
{
    uint64_t UnitLength;
    uint8_t DwarfFormat;
    uint16_t Version;
    uint32_t DebugInfoOffset;
    uint8_t AddressSize;
    uint8_t SegmentSize;
};

struct AbbrevAttr
{
    uint64_t Attr;
    uint64_t Form;
    uint64_t Const;    // DWARF5～
};

struct Abbrev
{
    uint64_t Id;
    uint64_t Tag;
    bool HasChildren;
    std::vector<AbbrevAttr> Attrs;
};

struct LineNumberStateMachine
{
public:
    LineNumberStateMachine(uint8_t defaultIsStmt) :
        Address(0),
        OpIndex(0),
        File(1),
        Line(1),
        Column(0),
        BasicBlock(false),
        EndSequence(false),
        PrologueEnd(false),
        Isa(0)
    {
        IsStmt = (defaultIsStmt == 1);
    };

    uint64_t Address;       // The program-counter value corresponding to a machine instruction generated by the compiler.
    uint64_t OpIndex;       // The index of the first operation is 0. For non-VLIW architectures, this register will always be 0.
    uint64_t File;          // the identity of the source file corresponding to a machine instruction.
    uint64_t Line;          // An unsigned integer indicating a source line number 1～ (The compiler may emit the value 0 in cases where an instruction cannot be attributed to any source line.)
    uint64_t Column;        // An unsigned integer indicating a column number within a source line. Columns are numbered beginning at 1. The value 0 is reserved to indicate that a statement begins at the “left edge” of the line.
    bool IsStmt;            // A boolean indicating that the current instruction is a recommended breakpoint location
    bool BasicBlock;        // A boolean indicating that the current instruction is the beginning of a basic block.
    bool EndSequence;       // A boolean indicating that the current address is that of the first byte after the end of a sequence of target machine instructions.
    bool PrologueEnd;       // A boolean indicating that the current address is one (of possibly many) where execution should be suspended for an entry breakpoint of a function.
    bool EpilogueBegin;     // A boolean indicating that the current address is one (of possibly many) where execution should be suspended for an exit breakpoint of a function.
    uint64_t Isa;           // An unsigned integer whose value encodes the applicable instruction set architecture for the current instruction.
    uint64_t Discriminator; // An unsigned integer identifying the block to which the current instruction belongs.
};

struct DwarfArangeInfo
{
    DwarfArangeInfoHdr Header;
    std::vector<DwarfSegmentInfo> Segments;
};

class Dwarf
{
public:
    static uint64_t ReaduLEB128(const uint8_t *bin, const uint64_t size, uint32_t &len);
    static int64_t ReadsLEB128(const uint8_t *bin, const uint64_t size, uint32_t &len);
    static std::map<uint64_t, DwarfArangeInfo> ReadAranges(const uint8_t *bin, const uint64_t size, const Elf64_Shdr &arrangesShdr);
    static std::vector<Abbrev> ReadAbbrevTbl(const uint8_t *bin, const uint64_t size, const Elf64_Shdr &dbgAbbrevShdr, const uint64_t dbgAbbrevOffset);

    static std::map<uint64_t, DwarfLineInfoHdr> ReadLineInfo(const uint8_t *bin, const uint64_t size, const Elf64_Shdr &debugLineShdr, const Elf64_Shdr &debugLineStrShdr, ElfFunctionTable &elfFuncTable);
    static std::vector<DwarfCuDebugInfo> ReadDebugInfo(const uint8_t *bin, const uint64_t size, const Elf64_Shdr &dbgInfoShdr, const Elf64_Shdr &dbgStrShdr, const Elf64_Shdr &dbgLineStrShdr, const Elf64_Shdr &dbgAbbrevShdr, std::map<uint64_t, DwarfArangeInfo> &offsetArangeMap, std::map<uint64_t, DwarfLineInfoHdr> &offsetLineInfoMap);
private:
	static void readLineNumberProgram(const uint8_t *bin, const uint64_t size, const std::string &fileName, const DwarfLineInfoHdr &lineInfoHdr, const uint64_t lnpStart, const uint64_t lnpEnd, ElfFunctionTable &elfFuncTable);
    static void addFuncAddrLineInfo(const DwarfLineInfoHdr &lineInfoHdr, LineNumberStateMachine lnsm, const uint64_t funcAddr, ElfFunctionTable &elfFuncTable);
    static DwarfCuHdr readCompilationUnitHeader(const uint8_t *bin, const uint64_t size, uint64_t offset);
    static std::map<uint64_t, std::string> getTagNameMap();
    static std::map<uint64_t, std::string> getAttrNameMap();
    static std::map<uint64_t, std::string> getLangNameMap();
};
