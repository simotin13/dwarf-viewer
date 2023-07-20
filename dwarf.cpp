#include <cassert>
#include "elf_parser.h"
#include "dwarf.h"
#include "binutil.h"
#include "logger.h"
#include "common.h"

std::map<uint64_t, DwarfArangeInfo> Dwarf::ReadAranges(const uint8_t* bin, const uint64_t size, const Elf64_Shdr &arrangesShdr)
{
    Logger::TLog("ReadAranges In...");
    uint64_t offset = arrangesShdr.sh_offset; 
    uint64_t sectionEnd  = arrangesShdr.sh_offset + arrangesShdr.sh_size;

    std::map<uint64_t, DwarfArangeInfo> arrangesMap;
    while (offset < sectionEnd)
    {
        uint64_t headerTop  = offset;
        uint64_t nextHdrTop = 0;
        uint64_t hdrSize    = 0;
        DwarfArangeInfo arangeInfo;
        DwarfArangeInfoHdr arangeInfoHdr;

        // TODO 4 or 8
        // unit_length initial length(4 or 8 bytes)
        uint32_t tmp = BinUtil::FromLeToUInt32(&bin[offset]);
        offset += 4;
        hdrSize += 4;

        if (tmp < 0xFFFFFF00)
        {
            // 32-bit DWARF Format
            arangeInfoHdr.UnitLength = (uint64_t)tmp;
            arangeInfoHdr.DwarfFormat = DWARF_32BIT_FORMAT;
        }
        else
        {
            // 64-bit DWARF Format
            arangeInfoHdr.UnitLength  = BinUtil::FromLeToUInt64(&bin[offset]);
            arangeInfoHdr.DwarfFormat = DWARF_64BIT_FORMAT;
            offset += 8;
            hdrSize += 8;
        }
        nextHdrTop = headerTop + hdrSize + arangeInfoHdr.UnitLength;

        // version uhalf
        arangeInfoHdr.Version = BinUtil::FromLeToUInt16(&bin[offset]);
        offset += 2;

        // header_length 32bit-DWARF/64bit-DWARF
        arangeInfoHdr.DebugInfoOffset = BinUtil::FromLeToUInt32(&bin[offset]);
        offset += 4;

        // address_size ubyte
        // The size of an address in bytes on the target architecture.
        arangeInfoHdr.AddressSize = bin[offset];
        offset += 1;

        // segment_size ubyte
        // The size of a segment selector in bytes on the target architecture.
        // If the target system uses a flat address space, this value is 0.
        arangeInfoHdr.SegmentSize = bin[offset];
        offset += 1;

        // header size must be devided by (AddressSize x 2)
        uint64_t alighnmentSize = arangeInfoHdr.AddressSize;
        uint64_t paddingSize = alighnmentSize - (offset-headerTop) % alighnmentSize;
        offset += paddingSize;

        arangeInfo.Header = arangeInfoHdr;
        arangeInfo.Segments = std::vector<DwarfSegmentInfo>();

        // TODO
        // spec version 2 is pair of address, length
        while (true)
        {
            DwarfSegmentInfo seg;
            if (arangeInfoHdr.AddressSize == 8)
            {
                uint64_t address = BinUtil::FromLeToUInt64(&bin[offset]);
                offset += 8;
                uint64_t length = BinUtil::FromLeToUInt64(&bin[offset]);
                offset += 8;

                //Logger::TLog("address:0x%x, length: 0x%x\n", address, length);
                if (address == 0 && length == 0)
                {
                    break;
                }
                seg.Address = address;
                seg.Length = length;
            }
            else
            {
                // TODO 32bit address
                uint32_t address = BinUtil::FromLeToUInt32(&bin[offset]);
                offset += 4;
                uint32_t length = BinUtil::FromLeToUInt32(&bin[offset]);
                offset += 4;

                //Logger::TLog("address:0x%x, length: 0x%x\n", address, length);
                if (address == 0 && length == 0)
                {
                    break;
                }

                seg.Address = (uint64_t)address;
                seg.Length = (uint64_t)length;
            }
            arangeInfo.Segments.push_back(seg);
        }
        if (nextHdrTop != offset)
        {
            offset += nextHdrTop - offset;
        }
        arrangesMap[arangeInfo.Header.DebugInfoOffset] = arangeInfo;
    }

    Logger::TLog("ReadAranges Out...");
    return arrangesMap;
}

std::vector<Abbrev> Dwarf::ReadAbbrevTbl(const uint8_t *bin, const uint64_t size, const Elf64_Shdr &dbgAbbrevShdr, const uint64_t dbgAbbrevOffset)
{
    std::vector<Abbrev> abbrevTbl;
    uint64_t offset     = dbgAbbrevOffset;
    uint64_t secEndPos  = dbgAbbrevShdr.sh_offset + dbgAbbrevShdr.sh_size;

    // for debug
    std::map<uint64_t, std::string> attrNameMap = getAttrNameMap();
    while(offset < secEndPos)
    {
        Abbrev abbrev;
        uint32_t len;
        abbrev.Id = ReaduLEB128(&bin[offset], secEndPos - offset, len);
        offset += len;
        if (abbrev.Id == 0)
        {
            // Abbreviations Tables end with an entry consisting of a 0 byte for the abbreviation code.
            break;
        }

        abbrev.Tag = ReaduLEB128(&bin[offset], secEndPos - offset, len);
        offset += len;
        uint8_t hasChildren = bin[offset];
        offset++;

        abbrev.HasChildren = (hasChildren == DW_CHILDREN_yes);

        // Read Attributes
        while (true)
        {
            uint64_t attrCode = 0;
            uint64_t formCode = 0;
            uint64_t Const = 0;
            attrCode = ReaduLEB128(&bin[offset], secEndPos - offset, len);
            offset += len;

            formCode = ReaduLEB128(&bin[offset], secEndPos - offset, len);
            offset += len;
            if ((attrCode == 0) && (formCode == 0))
            {
                break;
            }

            // DWARF5 or later, FORM special case
            if (formCode == DW_FORM_implicit_const)
            {
                Const = ReaduLEB128(&bin[offset], secEndPos - offset, len);
                offset += len;
            }

            AbbrevAttr attr = AbbrevAttr{attrCode, formCode, Const};
            Logger::DLog("attr:%s", attrNameMap[attr.Attr]);

            abbrev.Attrs.push_back(attr);
        }
        abbrevTbl.push_back(abbrev);
    }

    return abbrevTbl;
}

std::vector<DwarfCuDebugInfo> Dwarf::ReadDebugInfo(const uint8_t *bin, const uint64_t size, const Elf64_Shdr &dbgInfoShdr, const Elf64_Shdr &dbgStrShdr, const Elf64_Shdr &dbgLineStrShdr, const Elf64_Shdr &dbgAbbrevShdr, std::map<uint64_t, DwarfArangeInfo> &offsetArangeMap, std::map<uint64_t, DwarfLineInfoHdr> &offsetLineInfoMap)
{
    Logger::TLog("ReadDebugInfo In...");

    uint64_t offset     = dbgInfoShdr.sh_offset;
    uint64_t dbgInfoEnd = dbgInfoShdr.sh_offset + dbgInfoShdr.sh_size;
    std::vector<DwarfCuDebugInfo> dbgInfos;
    uint64_t count = 0;
    uint64_t cuLineInfoOffset  = 0;

    std::map<uint64_t, DwarfFuncInfo> cppTmpFuncMap;
    uint8_t *pDbgStrSec = (uint8_t *)&bin[dbgStrShdr.sh_offset];
    uint64_t dbgStrSecSize = dbgStrShdr.sh_size;

    uint8_t *pDbgLineStrSec = (uint8_t *)&bin[dbgLineStrShdr.sh_offset];
    uint64_t dbgLineStrSecSize = dbgLineStrShdr.sh_size;

    std::map<uint64_t,std::string> tagNameMap   = getTagNameMap();
    std::map<uint64_t,std::string> attrNameMap  = getAttrNameMap();
    std::map<uint64_t, std::string> langNameMap = getLangNameMap();
    while (offset < dbgInfoEnd)
    {
        uint64_t cuTop = offset;
        uint64_t dbgInfoOffset = offset - dbgInfoShdr.sh_offset;
        if (offsetArangeMap.find(dbgInfoOffset) == offsetArangeMap.end())
        {
            std::string msg = StringHelper::strprintf("offsetArangeMap.find(offset) == offsetArangeMap.end()");
            Logger::ELog(msg);
            assert(false);
            break;
        }

        DwarfArangeInfo &curArrangeInfo = offsetArangeMap[dbgInfoOffset];
        DwarfCuHdr cuh = readCompilationUnitHeader(bin, size, offset);
        Logger::DLog("******** cu header info ********");
        Logger::DLog("size: 0x%x\n", cuh.UnitLength);
        Logger::DLog("version: %d\n", cuh.Version);
        Logger::DLog("debug_abbrev_offset: %d\n", cuh.DebugAbbrevOffset);
        Logger::DLog("address_size: %d\n", cuh.AddressSize);
        DwarfCuDebugInfo cuDbgInfo;

        // compilation unit header
        uint64_t dbgAbbrevOffset = cuh.DebugAbbrevOffset + dbgAbbrevShdr.sh_offset;
        std::vector<Abbrev> abbrevTbl = Dwarf::ReadAbbrevTbl(bin, size, dbgAbbrevShdr, dbgAbbrevOffset);
        std::map<uint64_t, Abbrev> abbrevMap;
        for (auto it = abbrevTbl.begin(); it != abbrevTbl.end(); it++)
        {
            abbrevMap[it->Id] = *it;
        }
        uint64_t cuEnd = (cuh.UnitLength + 4);
        if (cuh.DwarfFormat == DWARF_64BIT_FORMAT)
        {
            cuEnd = offset + uint64_t(cuh.UnitLength + 8);
        }
        cuEnd += cuTop;
        offset += 11;

        uint32_t len;
        while (offset < cuEnd)
        {
            uint64_t entryOffset = offset - dbgInfoShdr.sh_offset;
            uint64_t id = ReaduLEB128(&bin[offset], dbgInfoEnd - offset, len);
            if (id == 0)
            {
                offset++;
                continue;
            }

            Abbrev &abbrev = abbrevMap[id];
            offset += len;
            DwarfFuncInfo dwarfFuncInfo;
            for (auto it = abbrev.Attrs.begin(); it != abbrev.Attrs.end(); it++)
            {
                AbbrevAttr attr = *it;
                std::string attrName = attrNameMap[attr.Attr];
                Logger::DLog("[%6x] %s", entryOffset, attrName);
                switch (attr.Form)
                {
                case DW_FORM_addr:
                {
                    // TODO addr
                    uint64_t funcaddr = 0;
                    if (cuh.AddressSize == 2)
                    {
                        uint16_t addr = BinUtil::FromLeToUInt16(&bin[offset]);
                        funcaddr = addr;
                        Logger::TLog("Attr: %s value:0x%04x\n", attrName, addr);
                    }
                    else if (cuh.AddressSize == 4)
                    {
                        uint32_t addr = BinUtil::FromLeToUInt32(&bin[offset]);
                        funcaddr = addr;
                        Logger::TLog("Attr: %s value:0x%08x\n", attrName, addr);
                    }
                    else if (cuh.AddressSize == 8)
                    {
                        uint64_t addr = BinUtil::FromLeToUInt64(&bin[offset]);
                        funcaddr = addr;
                        Logger::TLog("Attr: %s value:0x%016x\n", attrName, addr);
                    }

                    // DW_AT_low_pc  is function start address,
                    // DW_AT_high_pc is function end address,
                    if (attr.Attr == DW_AT_low_pc)
                    {
                        dwarfFuncInfo.Addr = funcaddr;
                    }
                    offset += cuh.AddressSize;
                }
                break;

                case DW_FORM_block2:
                {
                    uint16_t blk2 = BinUtil::FromLeToUInt16(&bin[offset]);
                    offset += 2;
                    offset += blk2;
                    Logger::DLog("Attr: %s value:0x%016x\n", attrName, blk2);
                }
                break;
                case DW_FORM_block4:
                {
                    uint32_t blk4 = BinUtil::FromLeToUInt32(&bin[offset]);
                    offset += 4;
                    offset += blk4;
                    Logger::DLog("Attr: %s value:0x%016x\n", attrName, blk4);
                }
                break;
                case DW_FORM_strp:
                {
                    uint32_t dbgStrOffset = BinUtil::FromLeToUInt32(&bin[offset]);
                    offset += 4;
                    std::string str = BinUtil::GetString(pDbgStrSec, dbgStrSecSize, dbgStrOffset);
                    Logger::DLog("%s: %s\n", attrName, str);
                    if (abbrev.Tag == DW_TAG_compile_unit)
                    {
                        if (attr.Attr == DW_AT_name)
                        {
                            // for Rust
                            if (cuDbgInfo.IsRust())
                            {
                                auto idx = str.find_last_of("@");
                                if (idx != std::string::npos)
                                {
                                    str = str.substr(0, idx);
                                }
                            }
                            cuDbgInfo.FileName = str;
                        }
                        else if (attr.Attr == DW_AT_comp_dir)
                        {
                            cuDbgInfo.CompileDir = str;
                        }
                        else if (attr.Attr == DW_AT_producer)
                        {
                            cuDbgInfo.Producer = str;
                        }
                        else
                        {
                            std::cerr << "not name!" << std::endl;
                            assert(false);
                        }
                    }
                    if (abbrev.Tag == DW_TAG_subprogram)
                    {
                        if (attr.Attr == DW_AT_name)
                        {
                            dwarfFuncInfo.Name = str;
                        }
                        else if (attr.Attr == DW_AT_linkage_name)
                        {
                            dwarfFuncInfo.LinkageName = str;
                        }
                        else if (attr.Attr == DW_AT_MIPS_linkage_name)
                        {
                            // arm-none-eabi-gcc
                            dwarfFuncInfo.Name = str;
                        }
                        else
                        {
                            std::cerr << "not name!" << std::endl;
                            assert(false);
                        }
                    }
                }
                break;
                case DW_FORM_data1:
                {
                    // TODO check value
                    // P207 TOOD DW_FORM_implicit_const
                    uint8_t tmp = bin[offset];
                    offset++;
                    if (attr.Attr == DW_AT_decl_file)
                    {
                        DwarfLineInfoHdr &lineInfoHdr = offsetLineInfoMap[cuLineInfoOffset];
                        std::string fileName = lineInfoHdr.Files[tmp-1].Name;
                        Logger::TLog("Attr: %s filename:%s\n", attrName, fileName);
                    }
                    else
                    {
                        Logger::TLog("Attr: %s value:0x%02x\n", attrName, tmp);
                    }
                }
                break;
                case DW_FORM_data2:
                {
                    // TODO check value
                    uint16_t val = BinUtil::FromLeToUInt16(&bin[offset]);
                    Logger::TLog("Attr: %s value:0x%04x\n", attrName, val);
                    if (attr.Attr == DW_AT_high_pc)
                    {
                        dwarfFuncInfo.Size = val;
                    }
                    if (attr.Attr == DW_AT_language)
                    {
                        std::string lang = "unknown language";
                        if (langNameMap.find(val) != langNameMap.end())
                        {
                            lang = langNameMap[val];
                        }
                    }
                    offset += 2;
                }
                break;
                case DW_FORM_data4:
                {
                    // TODO check value
                    uint32_t val = BinUtil::FromLeToUInt32(&bin[offset]);
                    Logger::TLog("Attr: %s value:0x%08x\n", attrName, val);
                    if (attr.Attr == DW_AT_high_pc)
                    {
                        dwarfFuncInfo.Size = val;
                    }
                    offset += 4;
                }
                break;
                case DW_FORM_data8:
                {
                    // TODO check value
                    uint32_t val = BinUtil::FromLeToUInt64(&bin[offset]);
                    if (attr.Attr == DW_AT_high_pc)
                    {
                        dwarfFuncInfo.Size = val;
                    }
                    Logger::TLog("Attr: %s value:0x%016x\n", attrName, val);
                    offset += 8;
                }
                break;
                case DW_FORM_string:
                {
                    std::string str = BinUtil::GetString(bin, dbgInfoEnd, offset);
                    offset += str.size() + 1;
                    Logger::DLog("str: %s \n", str);
                    if (abbrev.Tag == DW_TAG_subprogram)
                    {
                        if (attr.Attr == DW_AT_name)
                        {
                            dwarfFuncInfo.Name = str;
                        }
                        else if (attr.Attr == DW_AT_linkage_name)
                        {
                            dwarfFuncInfo.LinkageName = str;
                        }
                        else
                        {
                            std::cerr << "not name!" << std::endl;
                            assert(false);
                        }
                    }
                }
                break;
                case DW_FORM_block: // LEB128
                {
                    // TODO use Block info
                    ReaduLEB128(&bin[offset], dbgInfoEnd - offset, len);
                    offset += len;
                }
                break;
                case DW_FORM_block1: // 1byte(0～255)
                {
                    // TODO use Block info
                    uint8_t blockLen = bin[offset];
                    offset += 1;
                    Logger::TLog("Block1: len:%d\n", blockLen, blockLen);
                    offset += blockLen;
                }
                break;
                case DW_FORM_flag: // 1byte
                {
                    uint8_t flagVal = bin[offset];
                    offset += 1;
                    Logger::TLog("flag: val:%d\n", flagVal);
                }
                break;
                case DW_FORM_sdata:
                {
                    // TODO use constant
                    ReaduLEB128(&bin[offset], dbgInfoEnd - offset, len);
                    offset += len;
                }
                break;
                case DW_FORM_udata:
                {
                    // TODO use constant
                    ReaduLEB128(&bin[offset], dbgInfoEnd - offset, len);
                    offset += len;
                }
                break;
                case DW_FORM_ref1:
                {
                    uint8_t val = bin[offset];
                    uint64_t refval = cuTop + val;
                    Logger::TLog("Attr: %s value:0x%02x\n", attrName, refval);
                    offset += 1;
                }
                break;
                case DW_FORM_ref2:
                {
                    uint16_t val = BinUtil::FromLeToUInt16(&bin[offset]);
                    uint64_t refval = cuTop + val;
                    Logger::TLog("Attr: %s value:0x%04x\n", attrName, refval);
                    offset += 2;
                }
                break;
                case DW_FORM_ref4:
                {
                    uint32_t val = BinUtil::FromLeToUInt32(&bin[offset]);
                    uint64_t refval = cuTop + val;
                    uint64_t funcOffset =  refval - dbgInfoShdr.sh_offset;
                    Logger::TLog("Attr: %s value:0x%04x\n", attrName, funcOffset);
                    if (attr.Attr == DW_AT_specification)
                    {
                        if (cppTmpFuncMap.find(funcOffset) != cppTmpFuncMap.end())
                        {
                            // take function reference
                            dwarfFuncInfo = cppTmpFuncMap[funcOffset];
                        }
                        else
                        {
                            Logger::DLog("ref func not found");
                        } 
                    }
                    else if (attr.Attr == DW_AT_sibling)
                    {
                        // TODO
                    }
                    else if (attr.Attr == DW_AT_type)
                    {
                        // TODO
                    }
                    else
                    {
                        // TODO
                    }
                    /*
                        if attr.Attr == DW_AT_frame_base {
                            fmt.Println("!!!!!!!!!!!!!")
                        }
                    */
                    offset += 4;
                }
                break;
                case DW_FORM_sec_offset:
                {
                    switch (attr.Attr)
                    {
                    case DW_AT_stmt_list:
                    {
                        // DW_AT_stmt_list is a section offset to the line number information
                        // for this compilation unit
                        uint64_t tmp = BinUtil::FromLeToUInt32(&bin[offset]);
                        cuLineInfoOffset = tmp;
                        offset += 4;
                        Logger::TLog("%s: 0x%02x\n", attrName, cuLineInfoOffset);
                    }
                    break;
                    case DW_AT_ranges:
                    {
                        // A beginning address offset.
                        // A range list entry consists of:
                        // 1. A beginning address offset.
                        //    This address offset has the size of an address and is relative to the applicable base address of the compilation unit referencing this range list.
                        //    It marks the beginning of an address range.
                        // 2. An ending address offset.
                        //    This address offset again has the size of an address and is relative to the applicable base address of the compilation unit referencing this range list.
                        //    It marks the first address past the end of the address range.
                        //    The ending address must be greater than or equal to the beginning address.

                        // P162 rangelistptr
                        // This is an offset into the .debug_loc section (DW_FORM_sec_offset).
                        // It consists of an offset from the beginning of the .debug_loc section to the first byte of the data making up the location list for the compilation unit.
                        // It is relocatable in a relocatable object file, and relocated in an executable or shared object.
                        // In the 32-bit DWARF format, this offset is a 4-byte unsigned value; in the 64-bit DWARF format, it is an 8-byte unsigned value (see Section 7.4).
                        // TODO for 64bit impl
                        uint64_t loclistptr;
                        if (cuh.DwarfFormat == DWARF_32BIT_FORMAT)
                        {
                            uint32_t tmp = BinUtil::FromLeToUInt32(&bin[offset]);
                            loclistptr = tmp;
                            offset += 4;
                        } else {
                            loclistptr = BinUtil::FromLeToUInt64(&bin[offset]);
                            offset += 8;
                        }
                        Logger::TLog("loclistptr:%x", loclistptr);
                    }
                    break;

                    case DW_AT_location:
                    {
                        Logger::TLog("%x:%s\n", abbrev.Tag, tagNameMap[abbrev.Tag]);
                        uint64_t loclistptr;
                        if (cuh.DwarfFormat == DWARF_32BIT_FORMAT)
                        {
                            uint32_t tmp = BinUtil::FromLeToUInt32(&bin[offset]);
                            loclistptr = tmp;
                            offset += 4;
                        }
                        else
                        {
                            loclistptr = BinUtil::FromLeToUInt64(&bin[offset]);
                            offset += 8;
                        }
                        Logger::TLog("loclistptr:%x", loclistptr);
                        // GNU extensions
                    }
                    break;
                    case GNU_locviews:
                    {
                        // TODO
                        Logger::TLog("%x:%s\n", abbrev.Tag, tagNameMap[abbrev.Tag]);
                        uint64_t loclistptr;
                        if (cuh.DwarfFormat == DWARF_32BIT_FORMAT)
                        {
                            uint32_t tmp = BinUtil::FromLeToUInt32(&bin[offset]);
                            loclistptr = tmp;
                            offset += 4;
                        } else {
                            loclistptr = BinUtil::FromLeToUInt64(&bin[offset]);
                            offset += 8;
                        }
                        Logger::TLog("loclistptr:%x", loclistptr);
                    }
                    break;
                    default:
                    {
                        std::string msg = StringHelper::strprintf("unexpected attr:%d(%x)", attr.Attr, attr.Attr);
                        Logger::DLog(msg);
                        assert(false);
                    }
                    break;
                    }
                }
                break;
                case DW_FORM_exprloc:
                {
                    Logger::TLog("attr:%x,%s", attr.Attr, attrNameMap[attr.Attr]);
                    // following size
                    uint64_t length = ReaduLEB128(&bin[offset], dbgInfoEnd - offset, len);
                    offset += len;
                    for (uint32_t i = 0; i < length; i++)
                    {
                        // dwarf exp OP Code
                        uint8_t ins = bin[offset];
                        i++;
                        offset += 1;
                        if ((DW_OP_lo_user <= ins) && (ins <= DW_OP_hi_user))
                        {
                            // TODO skip extensions
                            offset += length - i;
                            i += length - i;
                            continue;
                        }

                        switch (ins)
                        {
                        case DW_OP_addr:
                        {
                            // size target specific
                            uint64_t addr = BinUtil::FromLeToUInt64(&bin[offset]);
                            Logger::TLog("DW_OP_addr:%x", addr);
                            offset += curArrangeInfo.Header.AddressSize;
                            i += curArrangeInfo.Header.AddressSize;
                        }
                        break;

                        case DW_OP_deref:
                        break;

                        case DW_OP_const1u:
                        {
                            uint8_t const1u = bin[offset];
                            Logger::TLog("DW_OP_const1u:%x", const1u);
                            offset++;
                            i++;
                        }
                        break;

                        case DW_OP_const1s:
                        {
                            int8_t const1s = (int8_t)bin[offset];
                            Logger::TLog("DW_OP_const1s :%d", const1s);
                            offset++;
                            i++;
                        }
                        break;

                        case DW_OP_const2u:
                        {
                            uint16_t const2u = BinUtil::FromLeToUInt16(&bin[offset]);
                            Logger::TLog("DW_OP_const2u :%d", const2u);
                            offset += 2;
                            i += 2;
                        }
                        break;

                        case DW_OP_const2s:
                        {
                            int16_t const2s = BinUtil::FromLeToInt16(&bin[offset]);
                            Logger::TLog("DW_OP_const2s :%d", const2s);
                            offset += 2;
                            i += 2;
                        }
                        break;

                        case DW_OP_const4u:
                        {
                            uint32_t const4u = BinUtil::FromLeToUInt32(&bin[offset]);
                            Logger::TLog("DW_OP_const4u :%d", const4u);
                            offset += 4;
                            i += 4;
                        }
                        break;

                        case DW_OP_const4s:
                        {
                            int32_t const4s = BinUtil::FromLeToInt32(&bin[offset]);
                            Logger::TLog("DW_OP_const4s :%d", const4s);
                            offset += 4;
                            i += 4;
                        }
                        break;
                        
                        case DW_OP_const8u:
                        {
                            uint64_t const8u = BinUtil::FromLeToUInt64(&bin[offset]);
                            Logger::TLog("DW_OP_const8u :%d", const8u);
                            offset += 8;
                            i += 8;
                        }
                        break;
                        
                        case DW_OP_const8s:
                        {
                            int64_t const8s = BinUtil::FromLeToInt64(&bin[offset]);
                            Logger::TLog("DW_OP_const8s :%d", const8s);
                            offset += 8;
                            i += 8;
                        }
                        break;
                        case DW_OP_constu:
                        {
                            int64_t constu = ReadsLEB128(&bin[offset], dbgInfoEnd - offset, len);
                            offset += len;
                            i += len;
                            Logger::TLog("DW_OP_constu:%d\n", constu);
                        }
                        break;

                        case DW_OP_consts:
                        {
                            int64_t consts = ReadsLEB128(&bin[offset], dbgInfoEnd - offset, len);
                            offset += len;
                            i += len;
                            Logger::TLog("DW_OP_consts:%d\n", consts);
                        }
                        break;

                        case DW_OP_drop:
                            break;
                        case DW_OP_over:
                            break;
                        case DW_OP_swap:
                            break;
                        case DW_OP_abs:
                            break;
                        case DW_OP_and:
                            break;
                        case DW_OP_div:
                            break;
                        case DW_OP_minus:
                            break;
                        case DW_OP_mod:
                            break;
                        case DW_OP_mul:
                            break;
                        case DW_OP_neg:
                            break;
                        case DW_OP_not:
                            break;
                        case DW_OP_or:
                            break;
                        case DW_OP_plus:
                            break;
                        case DW_OP_plus_uconst:
                        {
                            uint64_t operand = ReaduLEB128(&bin[offset], dbgInfoEnd - offset, len);
                            offset += len;
                            i += len;
                            Logger::TLog("\toperand:%d\n", operand);
                        }
                        break;

                        case DW_OP_shl:
                            break;
                        case DW_OP_shr:
                            break;
                        case DW_OP_shra:
                            break;
                        case DW_OP_xor:
                            break;
                        case DW_OP_skip: // 0x2f
                        {
                            int16_t operand = BinUtil::FromLeToInt16(&bin[offset]);
                            offset += 2;
                            i += 2;
                            Logger::TLog("\toperand:%d\n", operand);
                        }
                        break;

                        case DW_OP_bra: //  0x28
                        {
                            int16_t operand = BinUtil::FromLeToInt16(&bin[offset]);
                            offset += 2;
                            i += 2;
                            Logger::TLog("\toperand:%d\n", operand);
                        }
                        break;
                        case DW_OP_eq: // = 0x29
                            break;
                        case DW_OP_ge: // = 0x2a
                            break;
                        case DW_OP_gt: // = 0x2b
                            break;
                        case DW_OP_le: // = 0x2c
                            break;
                        case DW_OP_lt: // = 0x2d
                            break;
                        case DW_OP_ne: // = 0x2e
                            break;
                        case DW_OP_fbreg:
                        {
                            int64_t operand = ReadsLEB128(&bin[offset], dbgInfoEnd - offset, len);
                            offset += len;
                            i += len;
                            Logger::TLog("\toperand:%d\n", operand);
                        }
                        break;
                        case DW_OP_call_frame_cfa:
                            // no operand
                            break;
                        case DW_OP_lit0:
                        case DW_OP_lit1:
                        case DW_OP_lit2:
                        case DW_OP_lit3:
                        case DW_OP_lit4:
                        case DW_OP_lit5:
                        case DW_OP_lit6:
                        case DW_OP_lit7:
                        case DW_OP_lit8:
                        case DW_OP_lit9:
                        case DW_OP_lit10:
                        case DW_OP_lit11:
                        case DW_OP_lit12:
                        case DW_OP_lit13:
                        case DW_OP_lit14:
                        case DW_OP_lit15:
                        case DW_OP_lit16:
                        case DW_OP_lit17:
                        case DW_OP_lit18:
                        case DW_OP_lit19:
                        case DW_OP_lit20:
                        case DW_OP_lit21:
                        case DW_OP_lit22:
                        case DW_OP_lit23:
                        case DW_OP_lit24:
                        case DW_OP_lit25:
                        case DW_OP_lit26:
                        case DW_OP_lit27:
                        case DW_OP_lit28:
                        case DW_OP_lit29:
                        case DW_OP_lit30:
                        case DW_OP_lit31:
                            // TODO lit
                            break;
                        case DW_OP_reg0:
                        case DW_OP_reg1:
                        case DW_OP_reg2:
                        case DW_OP_reg3:
                        case DW_OP_reg4:
                        case DW_OP_reg5:
                        case DW_OP_reg6:
                        case DW_OP_reg7:
                        case DW_OP_reg8:
                        case DW_OP_reg9:
                        case DW_OP_reg10:
                        case DW_OP_reg11:
                        case DW_OP_reg12:
                        case DW_OP_reg13:
                        case DW_OP_reg14:
                        case DW_OP_reg15:
                        case DW_OP_reg16:
                        case DW_OP_reg17:
                        case DW_OP_reg18:
                        case DW_OP_reg19:
                        case DW_OP_reg20:
                        case DW_OP_reg21:
                        case DW_OP_reg22:
                        case DW_OP_reg23:
                        case DW_OP_reg24:
                        case DW_OP_reg25:
                        case DW_OP_reg26:
                        case DW_OP_reg27:
                        case DW_OP_reg28:
                        case DW_OP_reg29:
                        case DW_OP_reg30:
                        case DW_OP_reg31:
                            // TODO reg0 ~ reg31
                            break;
                        case DW_OP_breg0:
                        case DW_OP_breg1:
                        case DW_OP_breg2:
                        case DW_OP_breg3:
                        case DW_OP_breg4:
                        case DW_OP_breg5:
                        case DW_OP_breg6:
                        case DW_OP_breg7:
                        case DW_OP_breg8:
                        case DW_OP_breg9:
                        case DW_OP_breg10:
                        case DW_OP_breg11:
                        case DW_OP_breg12:
                        case DW_OP_breg13:
                        case DW_OP_breg14:
                        case DW_OP_breg15:
                        case DW_OP_breg16:
                        case DW_OP_breg17:
                        case DW_OP_breg18:
                        case DW_OP_breg19:
                        case DW_OP_breg20:
                        case DW_OP_breg21:
                        case DW_OP_breg22:
                        case DW_OP_breg23:
                        case DW_OP_breg24:
                        case DW_OP_breg25:
                        case DW_OP_breg26:
                        case DW_OP_breg27:
                        case DW_OP_breg28:
                        case DW_OP_breg29:
                        case DW_OP_breg30:
                        case DW_OP_breg31:
                        {
                            // The single operand of the DW_OP_bregn operations provides a signed LEB128 offset
                            // from the specified register.
                            ReadsLEB128(&bin[offset], dbgInfoEnd - offset, len);
                            offset += len;
                            i += len;
                        }
                        break;
                        case DW_OP_deref_size:
                        {
                            offset++;
                            i += 1;
                        }
                        break;
                        case DW_OP_implicit_value:
                        {
                            uint64_t length = ReaduLEB128(&bin[offset], dbgInfoEnd - offset, len);
                            offset += len;
                            offset += length;
                            i += size;
                            i += length;
                        }
                        break;
                        case DW_OP_stack_value:
                            // TODO
                            break;
                        default:
                        {
                            std::string msg = StringHelper::strprintf("TODO Not decoded op 0x%02x\n", ins);
                            Logger::DLog(msg);
                            assert(false);
                        }
                        break;
                        } // switch (ins)
                    }
                }
                break;
                case DW_FORM_flag_present:
                {
                    // flag exist
                    Logger::TLog("Attr: %s flag exists\n", attrName);
                }
                break;
                case DW_FORM_line_strp:
                {
                    uint64_t strOffset = 0;
                    if (cuh.DwarfFormat == DWARF_32BIT_FORMAT)
                    {
                        // 4byte
                        uint32_t tmp = BinUtil::FromLeToUInt32(&bin[offset]);
                        offset += 4;
                        strOffset = tmp;
                    }
                    else
                    {
                        // 8byte
                        uint64_t tmp = BinUtil::FromLeToUInt64(&bin[offset]);
                        offset += 8;
                        strOffset = tmp;
                    }
                    
                    std::string name = BinUtil::GetString(pDbgLineStrSec, dbgLineStrSecSize, strOffset);
                    if (abbrev.Tag == DW_TAG_compile_unit)
                    {
                        if (attr.Attr == DW_AT_name)
                        {
                            // for Rust
                            if (cuDbgInfo.IsRust())
                            {
                                auto idx = name.find_last_of("@");
                                if (idx != std::string::npos)
                                {
                                    name = name.substr(0, idx);
                                }
                            }
                            cuDbgInfo.FileName = name;
                        }
                        else if (attr.Attr == DW_AT_comp_dir)
                        {
                            cuDbgInfo.CompileDir = name;
                        }
                        else
                        {
                            assert(false);
                        }
                    }
                }
                break;
                case DW_FORM_implicit_const:
                    if (attr.Attr == DW_AT_decl_file)
                    {
                        DwarfLineInfoHdr lineInfoHdr = offsetLineInfoMap[cuLineInfoOffset];
                        std::string fileName = lineInfoHdr.Files[attr.Const].Name;
                        Logger::TLog("Attr: %s filename:%s\n", attrName, fileName);
                    }
                    else
                    {
                        Logger::TLog("Attr: %s value:0x%02x\n", attrName, attr.Const);
                    }
                    break;
                default:
                {
                    std::string msg = StringHelper::strprintf("Unknown Form:0x%x\n", attr.Form);
                    Logger::DLog(msg);
                    assert(false);
                }
                break;

                }
            }
            if (abbrev.Tag == DW_TAG_subprogram)
            {
                if (dwarfFuncInfo.Name == "")
                {
                    if (cuDbgInfo.Funcs.find(dwarfFuncInfo.Addr) != cuDbgInfo.Funcs.end())
                    {
                        DwarfFuncInfo &dbgFunc = cuDbgInfo.Funcs[dwarfFuncInfo.Addr];
                        Logger::TLog("name:%s, addr:0x%X already registed\n", dbgFunc.Name, dwarfFuncInfo.Addr);
                    }
                    else
                    {
                        // TODO For Rust
                        cppTmpFuncMap[entryOffset] = dwarfFuncInfo;
                        Logger::DLog("addr:0x:%x function not found\n", dwarfFuncInfo.Addr);
                        count++;
                        // TODO Comment out For Rust
                        //os.Exit(2)
                        continue;
                    }
                }
                if (dwarfFuncInfo.Addr != 0)
                {
                    // skip if addr not set(must be library function)
                    Logger::TLog("name:%s, linkageName:%s addr:0x%X\n", dwarfFuncInfo.Name, dwarfFuncInfo.LinkageName, dwarfFuncInfo.Addr);
                    cuDbgInfo.Funcs[dwarfFuncInfo.Addr] = dwarfFuncInfo;
                }
                else
                {
                    // addr not fixed, maybe c++ function delc, add tmpFuncs
                    cppTmpFuncMap[entryOffset] = dwarfFuncInfo;
                }
            }
            count++;
        }
        dbgInfos.push_back(cuDbgInfo);
    }

    
    Logger::TLog("ReadDebugInfo Out...");
    return dbgInfos;
}

std::map<uint64_t, DwarfLineInfoHdr> Dwarf::ReadLineInfo(const uint8_t *bin, const uint64_t size, const Elf64_Shdr &debugLineShdr, const Elf64_Shdr &debugLineStrShdr, ElfFunctionTable &elfFuncTable)
{
    Logger::TLog("ReadLineInfo In...");
    std::map<uint64_t, DwarfLineInfoHdr> offsetLineInfoHdrMap;
    uint64_t hdrOffset  = debugLineShdr.sh_offset;
    uint64_t sectionEnd = debugLineShdr.sh_offset + debugLineShdr.sh_size;
    while (hdrOffset < sectionEnd)
    {
        uint64_t offset = hdrOffset;
        DwarfLineInfoHdr lineInfoHdr;
        // unit_length initial length(4 or 8 bytes)
        uint32_t tmp = BinUtil::FromLeToUInt32(&bin[offset]);
        offset += 4;
        if (tmp < 0xffffff00)
        {
            // 32-bit DWARF Format
            lineInfoHdr.UnitLength = (uint64_t)tmp;
            lineInfoHdr.DwarfFormat = DWARF_32BIT_FORMAT;
        }
        else
        {
            // 64-bit DWARF Format
            lineInfoHdr.UnitLength = BinUtil::FromLeToUInt64(&bin[offset]);
            lineInfoHdr.DwarfFormat = DWARF_64BIT_FORMAT;
            offset += 8;
        }

        // version uhalf
        lineInfoHdr.Version = BinUtil::FromLeToUInt16(&bin[offset]);
        offset += 2;

        if (5 <= lineInfoHdr.Version)
        {
            // DWARF Version 5 or later
            lineInfoHdr.AddressSize = bin[offset];
            offset += 1;
            lineInfoHdr.SegmentSelectorSize = bin[offset];
            offset += 1;
        }

        // header_length 32bit-DWARF/64bit-DWARF
        lineInfoHdr.HeaderLength = BinUtil::FromLeToUInt32(&bin[offset]);
        offset += 4;

        // minimum_instruction_length ubyte
        lineInfoHdr.MinInstLength = bin[offset];
        offset += 1;

        // maximum_operations_per_instruction ubyte
        if (4 <= lineInfoHdr.Version) {
            lineInfoHdr.MaxInstLength = bin[offset];
            offset += 1;
        }

        // default_is_stmt ubyte
        lineInfoHdr.DefaultIsStmt = bin[offset];
        offset += 1;

        // line_base (sbyte)
        lineInfoHdr.LineBase = (int8_t)(bin[offset]);
        offset += 1;

        // line_range ubyte
        lineInfoHdr.LineRange = bin[offset];
        offset += 1;

        // opcode_base ubyte
        // The number assigned to the first special opcode.
        lineInfoHdr.OpcodeBase = bin[offset];
        offset += 1;

        // standard_opcode_lengths array of ubyte
        // This array specifies the number of LEB128 operands for each of the standard opcodes.
        // The first element of the array corresponds to the opcode whose value is 1, and
        // the last element corresponds to the opcode whose value is opcode_base - 1.

        // TODO 要確認 vector 初期化
        lineInfoHdr.StdOpcodeLengths.clear();
        for (uint32_t i = 0; i < (uint32_t)lineInfoHdr.OpcodeBase-1; i++)
        {
            lineInfoHdr.StdOpcodeLengths.push_back(bin[offset]);
            offset++;
        }

        if (5 <= lineInfoHdr.Version)
        {
            // DWARF Version 5 or later
            lineInfoHdr.IncludeDirs.clear();

            // directories
            lineInfoHdr.DirectoryEntryFormatCount = bin[offset];
            offset++;

            // P156
            uint32_t len = 0;
            for (uint32_t i = 0; i < lineInfoHdr.DirectoryEntryFormatCount; i++)
            {
                EntryFormat entryFmt;
                entryFmt.TypeCode = ReaduLEB128(&bin[offset], sectionEnd-offset, len);
                offset += len;
                entryFmt.FormCode  = ReaduLEB128(&bin[offset], sectionEnd-offset, len);
                offset += len;
                lineInfoHdr.DirectoryEntryFormats.push_back(entryFmt);
            }

            lineInfoHdr.DirectoriesCount = ReaduLEB128(&bin[offset], sectionEnd-offset, len);
            offset += len;

            for (uint32_t i = 0; i < lineInfoHdr.DirectoriesCount; i++)
            {
                for (uint32_t j = 0; j < lineInfoHdr.DirectoryEntryFormatCount; j++)
                {
                    uint64_t typeCode = lineInfoHdr.DirectoryEntryFormats[j].TypeCode;
                    uint64_t formCode = lineInfoHdr.DirectoryEntryFormats[j].FormCode;
                    switch (typeCode)
                    {
                    case DW_LNCT_path:
                        {
                            std::string dirName = "";
                            if (formCode == DW_FORM_line_strp)
                            {
                                // offset in the .debug_str, size follows Dwarf format(4 or 8)
                                uint64_t strOffset = 0;
                                if (lineInfoHdr.DwarfFormat == DWARF_32BIT_FORMAT)
                                {
                                    // 4byte
                                    tmp = BinUtil::FromLeToUInt32(&bin[offset]);
                                    offset += 4;
                                    strOffset = tmp;
                                }
                                else
                                {
                                    // 8byte
                                    tmp = BinUtil::FromLeToUInt64(&bin[offset]);
                                    offset += 8;
                                    strOffset = tmp;
                                }
                                uint64_t strSecEndPos = debugLineStrShdr.sh_offset + debugLineStrShdr.sh_size;
                                dirName = BinUtil::GetString(&bin[debugLineStrShdr.sh_offset], strSecEndPos, strOffset);
                            }
                            lineInfoHdr.IncludeDirs.push_back(dirName);
                        }
                        break;
                    default:
                        // TODO unexpected
                        assert(false);
                    }
                }
            }

            // file names
            lineInfoHdr.FileNameEntryFormatCount = bin[offset];
            offset++;

            for (uint32_t i = 0; i < lineInfoHdr.FileNameEntryFormatCount; i++)
             {
                EntryFormat entryFmt;
                entryFmt.TypeCode = ReaduLEB128(&bin[offset], sectionEnd - offset, len);
                offset += len;
                entryFmt.FormCode = ReaduLEB128(&bin[offset], sectionEnd - offset, len);
                offset += len;
                lineInfoHdr.FileNameEntryFormats.push_back(entryFmt);
            }

            lineInfoHdr.FileNamesCount = ReaduLEB128(&bin[offset], sectionEnd - offset, len);
            offset += len;

            for (uint32_t i = 0; i < lineInfoHdr.FileNamesCount; i++)
            {
                FileNameInfo fileNameInfo;
                uint64_t fileIdx = 0;
                for (uint32_t j = 0; j < lineInfoHdr.FileNameEntryFormatCount; j++)
                {
                    uint64_t typeCode = lineInfoHdr.FileNameEntryFormats[j].TypeCode;
                    uint64_t formCode = lineInfoHdr.FileNameEntryFormats[j].FormCode;
                    switch (typeCode)
                    {
                        case DW_LNCT_path:
                        {
                            if (formCode == DW_FORM_line_strp)
                            {
                                // offset in the .debug_str, size follows Dwarf format(4 or 8)
                                uint64_t strOffset = 0;
                                if (lineInfoHdr.DwarfFormat == DWARF_32BIT_FORMAT)
                                {
                                    // 4byte
                                    tmp = BinUtil::FromLeToUInt32(&bin[offset]);
                                    offset += 4;
                                    strOffset = tmp;
                                }
                                else
                                {
                                    // 8byte
                                    tmp = BinUtil::FromLeToUInt64(&bin[offset]);
                                    offset += 8;
                                    strOffset = tmp;
                                }
                                uint64_t strSecEndPos = debugLineStrShdr.sh_offset + debugLineStrShdr.sh_size;
                                fileNameInfo.Name = BinUtil::GetString(&bin[debugLineStrShdr.sh_offset], strSecEndPos, strOffset);
                            }
                            else
                            {
                                // TODO unexpected
                                assert(false);
                            }
                        }
                        break;

                        case DW_LNCT_directory_index:
                        {
                            switch (formCode)
                            {
                                case DW_FORM_data1:
                                {
                                    fileIdx = bin[offset];
                                    offset++;
                                }
                                break;

                                case DW_FORM_data2:
                                {
                                    tmp = BinUtil::FromLeToUInt16(&bin[offset]);
                                    fileIdx += tmp;
                                    offset += 2;
                                }
                                break;

                                case DW_FORM_udata:
                                {
                                    fileIdx = ReaduLEB128(&bin[offset], sectionEnd - offset, len);
                                    offset += len;
                                }
                                break;

                                default:
                                    // TODO unexpected
                                    assert(false);
                                    break;
                            }
                            fileNameInfo.DirIdx = fileIdx;
                        }
                        break;
                    default:
                        // TODO unexpected
                        assert(false);
                    }
                }
                // TODO save fileIdx info
                lineInfoHdr.Files.push_back(fileNameInfo);
            }

            uint64_t endOffset = hdrOffset + lineInfoHdr.UnitLength;
            if (lineInfoHdr.DwarfFormat == DWARF_32BIT_FORMAT)
            {
                endOffset += 4;
            }
            else
            {
                endOffset += 8;
            }

            std::string fileName = lineInfoHdr.Files[0].Name;
            if (0 < (endOffset - offset))
            {
                readLineNumberProgram(bin, size, fileName, lineInfoHdr, offset, endOffset, elfFuncTable);
            }
        }
        else
        {
            // include_directories
            while (true)
            {
                std::string dirName = BinUtil::GetString(&bin[offset], sectionEnd - offset, 0);
                uint32_t sLen = dirName.size();
                if (sLen == 0)
                {
                    offset++;
                    break;
                }
                offset += sLen + 1;
                lineInfoHdr.IncludeDirs.push_back(dirName);
            }

            // file_names
            while(true)
            {
                FileNameInfo fileNameInfo;

                // name
                fileNameInfo.Name = BinUtil::GetString(&bin[offset], sectionEnd - offset, 0);
                uint32_t sLen = fileNameInfo.Name.size();
                if (sLen == 0)
                {
                    offset++;
                    break;
                }
                offset += (uint64_t)(sLen + 1);

                // directory Idx
                uint32_t len;
                fileNameInfo.DirIdx = ReaduLEB128(&bin[offset], sectionEnd - offset, len);
                offset += len;

                // last modified
                fileNameInfo.LastModified = ReaduLEB128(&bin[offset], sectionEnd - offset, len);
                offset += len;

                // file size
                fileNameInfo.Size = ReaduLEB128(&bin[offset], sectionEnd - offset, len);
                offset += len;

                lineInfoHdr.Files.push_back(fileNameInfo);
            }

            uint64_t endOffset = hdrOffset + lineInfoHdr.UnitLength;
            if (lineInfoHdr.DwarfFormat == DWARF_32BIT_FORMAT)
            {
                endOffset += 4;
            } else {
                endOffset += 8;
            }

            std::string fileName = lineInfoHdr.Files[0].Name;
            if (0 < (endOffset - offset))
            {
                readLineNumberProgram(bin, size, fileName, lineInfoHdr, offset, endOffset, elfFuncTable);
            }
        }

        uint32_t lineInfoHdrOffset = hdrOffset - debugLineShdr.sh_offset;
        offsetLineInfoHdrMap[lineInfoHdrOffset] = lineInfoHdr;
        hdrOffset += lineInfoHdr.UnitLength;
        if (lineInfoHdr.DwarfFormat == DWARF_32BIT_FORMAT)
        {
            hdrOffset += 4;
        }
        else
        {
            hdrOffset += 8;
        }
    }
    return offsetLineInfoHdrMap;
}
// DW_LNS name map

void Dwarf::readLineNumberProgram(const uint8_t *bin, const uint64_t size, const std::string &fileName, const DwarfLineInfoHdr &lineInfoHdr, const uint64_t lnpStart, const uint64_t lnpEnd, ElfFunctionTable &elfFuncTable)
{
    uint8_t *lnpIns = (uint8_t *)(&bin[lnpStart]);
    const uint64_t length  = lnpEnd - lnpStart;
    uint64_t offset = 0;
    uint64_t curFuncAddr = 0;
    LineNumberStateMachine lnsm(lineInfoHdr.DefaultIsStmt);

    // for debug map
    std::map<uint8_t, std::string> dwLnsNameMap;
    dwLnsNameMap[DW_LNS_copy]               =   "DW_LNS_copy";
    dwLnsNameMap[DW_LNS_advance_pc]         =   "DW_LNS_advance_pc";
    dwLnsNameMap[DW_LNS_advance_line]       =   "DW_LNS_advance_line";
    dwLnsNameMap[DW_LNS_set_file]           =   "DW_LNS_set_file";
    dwLnsNameMap[DW_LNS_set_column]         =   "DW_LNS_set_column";
    dwLnsNameMap[DW_LNS_negate_stmt]        =   "DW_LNS_negate_stmt";
    dwLnsNameMap[DW_LNS_set_basic_block]    =   "DW_LNS_set_basic_block";
    dwLnsNameMap[DW_LNS_const_add_pc]       =   "DW_LNS_const_add_pc";
    dwLnsNameMap[DW_LNS_fixed_advance_pc]   =   "DW_LNS_fixed_advance_pc";
    dwLnsNameMap[DW_LNS_set_prologue_end]   =   "DW_LNS_set_prologue_end";
    dwLnsNameMap[DW_LNS_set_epilogue_begin] =   "DW_LNS_set_epilogue_begin";
    dwLnsNameMap[DW_LNS_set_isa]            =   "DW_LNS_set_isa";

    bool endOfSeq = false;
    while (offset < length)
     {
        endOfSeq = false;

        // read opecode
        uint8_t opcode = lnpIns[offset];

        // for debug
        uint64_t opOffset = lnpStart + offset;
        if (dwLnsNameMap.find(opcode) != dwLnsNameMap.end())
        {
            std::string dwLnsName = dwLnsNameMap[opcode];
            Logger::TLog("[%6x] opcode: %d(0x%x), %s", opOffset, opcode, opcode, dwLnsName);
        }
        else
        {
            Logger::TLog("[%6x] opcode: %d(0x%x)", opOffset, opcode, opcode);
        }

        offset++;
        uint32_t len = 0;
        switch (opcode)
        {
        case 0x00: // extended opcodes
            {
                uint64_t tmp = ReaduLEB128(&lnpIns[offset], lnpEnd - offset, len);
                offset += len;
                uint8_t extendedOpcode = lnpIns[offset];
                offset++;
                switch (extendedOpcode)
                {
                case DW_LNE_end_sequence:
                    lnsm = LineNumberStateMachine(lineInfoHdr.DefaultIsStmt);
                    //offset += length
                    // break parse loop
                    endOfSeq = true;
                    break;
                case DW_LNE_set_address:
                    {
                        uint64_t addrSize = tmp - 1;
                        if (addrSize == 8)
                        {
                            uint64_t address = BinUtil::FromLeToUInt64(&lnpIns[offset]);
                            lnsm.Address = address;
                            curFuncAddr = address;
                        }
                        else
                        {
                            uint32_t address = BinUtil::FromLeToUInt32(&lnpIns[offset]);
                            lnsm.Address = address;
                            curFuncAddr = address;
                        }
                        offset += tmp - 1;
                    }
                    break;
                case DW_LNE_define_file:
                    {
                        // TODO
                        offset += tmp - 1;
                    }
                    break;
                case DW_LNE_set_discriminator:
                    {
                        // TODO
                        // Bug. gcc version 9.3.0 (Ubuntu 9.3.0-17ubuntu1~20.04)
                        // DW_LNE_set_discriminator is defined DWARF4, but section header's DWARF version is 3...
                        uint64_t discriminator = ReaduLEB128(&lnpIns[offset], lnpEnd - offset, len);
                        lnsm.Discriminator = discriminator;
                        offset += len;
                    }
                    break;
                case DW_LNE_lo_user:
                    // TODO
                    offset += tmp - 1;
                    break;
                case DW_LNE_hi_user:
                    // TODO
                    offset += tmp - 1;
                    break;
                default:
                    {
                        std::string msg = StringHelper::strprintf("Unexpected extended opcode:%d(0x%x)", extendedOpcode, extendedOpcode);
                        std::cerr << msg << std::endl;
                        assert(false);
                    }
                    break;
                }
            }
            break;
        case DW_LNS_copy:
            {
                if (lnsm.IsStmt)
                {
                    addFuncAddrLineInfo(lineInfoHdr, lnsm, curFuncAddr, elfFuncTable);
                }
                lnsm.BasicBlock = false;
                lnsm.PrologueEnd = false;
                lnsm.EpilogueBegin = false;
            }
            break;
        case DW_LNS_advance_pc:
            {
                uint64_t addrInc = ReaduLEB128(&lnpIns[offset], lnpEnd - offset, len);
                lnsm.Address += addrInc * (uint64_t)lineInfoHdr.MinInstLength;
                offset += len;
            }
            break;
        case DW_LNS_advance_line:
            {
                int64_t lineInc = ReadsLEB128(&lnpIns[offset], lnpEnd - offset, len);
                lnsm.Line = (uint64_t)lnsm.Line + lineInc;
                offset += len;
            }
            break;
        case DW_LNS_set_file:
            {
                uint64_t fileIdx = ReaduLEB128(&lnpIns[offset], lnpEnd - offset, len);
                lnsm.File = fileIdx;
                offset += len;
            }
            break;
        case DW_LNS_set_column:
            // column set
            {
                uint64_t coperand = ReaduLEB128(&lnpIns[offset], lnpEnd - offset, len);
                lnsm.Column = coperand;
                offset += len;
            }
            break;
        case DW_LNS_negate_stmt:
            {
                // no operand
                lnsm.IsStmt = !(lnsm.IsStmt);
            }
            break;
        case DW_LNS_set_basic_block:
            {
                // no operand
                lnsm.BasicBlock = true;
            }
            break;
        case DW_LNS_const_add_pc:
            {
                // no operand
                // increment addres is same as special opcode.
                uint8_t adjOpcode = opcode - lineInfoHdr.OpcodeBase;
                uint64_t addrInc = (adjOpcode / lineInfoHdr.LineRange) * lineInfoHdr.MinInstLength;
                lnsm.Address = lnsm.Address + addrInc;
            }
            break;
        case DW_LNS_fixed_advance_pc:
            {
                // The DW_LNS_fixed_advance_pc opcode takes a single uhalf (unencoded) operand
                // and adds it to the address register of the state machine and sets the op_index register to 0.
                uint16_t address = BinUtil::FromLeToUInt16(&lnpIns[offset]);
                lnsm.Address = lnsm.Address + address;
                lnsm.OpIndex = 0;
                offset += 2;
            }
            break;
        case DW_LNS_set_prologue_end:
            {
                lnsm.PrologueEnd = true;
            }
            break;
        case DW_LNS_set_epilogue_begin:
            {
                lnsm.EpilogueBegin = true;
            }
            break;
        case DW_LNS_set_isa:
            {
                // read and skip value
                ReadsLEB128(&lnpIns[offset], lnpEnd - offset, len);
                offset += len;
            }
            break;
        default:
            {
                // special opcode
                // no operand
                // See Dwarf3.pdf 6.2.5.1 Special Opcodes
                // opcode = (desired line increment - line_base) + (line_range * address advance) + opcode_base
                // address increment = (adjusted opcode / line_range) * minimim_instruction_length
                // line increment = line_base + (adjusted opcode % line_range)
                uint8_t adjOpcode = opcode - lineInfoHdr.OpcodeBase;
                uint64_t addrInc = (adjOpcode / lineInfoHdr.LineRange) * lineInfoHdr.MinInstLength;
                int64_t lineInc = lineInfoHdr.LineBase + (adjOpcode % lineInfoHdr.LineRange);
                lnsm.Line = lnsm.Line + lineInc;

                // check function
                uint64_t addr = lnsm.Address + addrInc;
                lnsm.Address = addr;
                lnsm.BasicBlock = false;
                lnsm.PrologueEnd = false;
                lnsm.EpilogueBegin = false;
                curFuncAddr = lnsm.Address;
                if (lnsm.IsStmt)
                {
                    addFuncAddrLineInfo(lineInfoHdr, lnsm, curFuncAddr, elfFuncTable);
                }
                Logger::DLog("special opcode:0x%02X, address inc:%d, line inc:%d", opcode, addrInc, lineInc);
            }
        }
    }

    if (!endOfSeq)
    {
        std::cerr << "Error DW_LNE_end_sequence not found" << std::endl;
        assert(false);
    }

    Logger::TLog("ReadLineInfo Out...");
    return;
}

void Dwarf::addFuncAddrLineInfo(const DwarfLineInfoHdr &lineInfoHdr, LineNumberStateMachine lnsm, const uint64_t funcAddr, ElfFunctionTable &elfFuncInfos)
{
    FileNameInfo file = lineInfoHdr.Files[lnsm.File-1];
    if (elfFuncInfos.AddrFuncIdxMap.find(funcAddr) == elfFuncInfos.AddrFuncIdxMap.end())
    {
        // TDOO: must Fix this
        Logger::DLog("function not exist in %s, funcAddr:0x%x\n", elfFuncInfos.Path, funcAddr);
        return;
    }
    uint32_t funcIdx = elfFuncInfos.AddrFuncIdxMap[funcAddr];
    ElfFunctionInfo &elfFuncInfo = elfFuncInfos.ElfFuncInfos[funcIdx];
    elfFuncInfo.SrcDirName = "";
    elfFuncInfo.SrcFileName = file.Name;
    LineAddrInfo lineAddr;
    lineAddr.Line = lnsm.Line;
    lineAddr.Addr = lnsm.Address;
    lineAddr.IsStmt = lnsm.IsStmt;

    if (5 <= lineInfoHdr.Version)
    {
        // get src file info
        lineAddr.SrcDirName = lineInfoHdr.IncludeDirs[file.DirIdx];
        elfFuncInfo.SrcDirName = lineInfoHdr.IncludeDirs[file.DirIdx];
        elfFuncInfo.LineAddrs[lnsm.Line] = lineAddr;

        // update elf function info
        elfFuncInfos.ElfFuncInfos[funcIdx] = elfFuncInfo;
    }
    else
    {
        if (file.DirIdx < 1)
        {
            // TODO find src path...
        }
        else
        {
            // implemented at libray
            //lineAddr.IsLibrary = true

            // get src file info
            lineAddr.SrcDirName = lineInfoHdr.IncludeDirs[file.DirIdx-1];
            elfFuncInfo.SrcDirName = lineInfoHdr.IncludeDirs[file.DirIdx-1];
        }
        elfFuncInfo.LineAddrs[lnsm.Line] = lineAddr;

        // update elf function info
        elfFuncInfos.ElfFuncInfos[funcIdx] = elfFuncInfo;
    }
}

uint64_t Dwarf::ReaduLEB128(const uint8_t *bin, const uint64_t size, uint32_t &len)
{
    uint64_t pos = 0;
    uint64_t val = 0;
    uint64_t tmp;
    while (pos < size)
    {
        tmp = (uint64_t)(bin[pos] & 0x7F);
        tmp = tmp << (7 * pos);
        val += tmp;
        if ((bin[pos] & 0x80) == 0)
        {
            break;
        }
        pos++;
    }
    len = pos + 1;
    return val;
}

 int64_t Dwarf::ReadsLEB128(const uint8_t *bin, const uint64_t size, uint32_t &len)
 {
    int64_t val = 0;
    uint32_t pos = 0;
    while (pos < size)
    {
        uint64_t tmp = int64_t(bin[pos] & 0x7F);
        tmp = tmp << (7 * pos);
        val += tmp;
        if ((bin[pos] & 0x80) == 0)
        {
            if ((bin[pos] & 0x40) != 0)
            {
                // negative value
                val |= ~0 << (7 * (pos+1));
            }
            break;
        }
        pos++;
    }
    len = pos + 1;
    return val;
}

DwarfCuHdr Dwarf::readCompilationUnitHeader(const uint8_t *bin, const uint64_t size, uint64_t offset)
{
    // ================================================
    // Read Compilation Unit Header
    // ================================================
    DwarfCuHdr cuh;
    uint32_t tmp = BinUtil::FromLeToUInt32(&bin[offset]);
    offset += 4;
    if (tmp < 0xFFFFFF00)
    {
        // 32-bit DWARF Format
        cuh.UnitLength = tmp;
        cuh.DwarfFormat = DWARF_32BIT_FORMAT;
    }
    else
    {
        // 64-bit DWARF Format
        cuh.UnitLength = BinUtil::FromLeToUInt64(&bin[offset]);
        cuh.DwarfFormat = DWARF_64BIT_FORMAT;
        offset += 8;
    }

    cuh.Version = BinUtil::FromLeToUInt16(&bin[offset]);
    offset += 2;
    if (cuh.Version < 5)
    {
        // debug_abbrev_offset
        cuh.DebugAbbrevOffset = BinUtil::FromLeToUInt32(&bin[offset]);
        offset += 4;

        // address_size
        cuh.AddressSize = bin[offset];
        offset++;
    }
    else
    {
        // DWARF 5 or later
        cuh.UnitType = bin[offset];
        offset++;

        // address_size
        cuh.AddressSize = bin[offset];
        offset++;

        // debug_abbrev_offset
        cuh.DebugAbbrevOffset = BinUtil::FromLeToUInt32(&bin[offset]);
        offset += 4;

        switch (cuh.UnitType)
        {
            case DW_UT_compile:
            case DW_UT_partial:
            case DW_UT_skeleton:
            case DW_UT_split_compile:
            {
                cuh.UnitID = BinUtil::FromLeToUInt64(&bin[offset]);
                offset += 8;
            }
            break;

            case DW_UT_type:
            case DW_UT_split_type:
            {
                cuh.TypeSignature = BinUtil::FromLeToUInt64(&bin[offset]);
                offset += 8;
                cuh.TypeSignature = BinUtil::FromLeToUInt64(&bin[offset]);
                if (cuh.DwarfFormat == DWARF_32BIT_FORMAT)
                {
                    tmp = BinUtil::FromLeToUInt32(&bin[offset]);
                    offset += 8;
                    cuh.TypeOffset = tmp;
                }
                else
                {
                    cuh.TypeOffset = BinUtil::FromLeToUInt64(&bin[offset]);
                }
            }
            break;
            default:
                std::cerr << "not implemented" << std::endl;
                assert(false);
                break;
        }
    }

    return cuh;
}
std::map<uint64_t, std::string> Dwarf::getTagNameMap()
{
    std::map<uint64_t, std::string> tagNameMap;
	tagNameMap[DW_TAG_array_type]               = "TAG_array_type";
	tagNameMap[DW_TAG_class_type]               = "TAG_class_type";
	tagNameMap[DW_TAG_entry_point]              = "TAG_entry_point";
	tagNameMap[DW_TAG_enumeration_type]         = "TAG_enumeration_type";
	tagNameMap[DW_TAG_formal_parameter]         = "TAG_formal_parameter";
	tagNameMap[DW_TAG_imported_declaration]     = "DW_TAG_imported_declaration";
	tagNameMap[DW_TAG_label]                    = "DW_TAG_label";
	tagNameMap[DW_TAG_lexical_block]            = "DW_TAG_lexical_block";
	tagNameMap[DW_TAG_member]                   = "DW_TAG_member";
	tagNameMap[DW_TAG_pointer_type]             = "DW_TAG_pointer_type";
	tagNameMap[DW_TAG_reference_type]           = "DW_TAG_reference_type";
	tagNameMap[DW_TAG_compile_unit]             = "DW_TAG_compile_unit";
	tagNameMap[DW_TAG_string_type]              = "DW_TAG_string_type";
	tagNameMap[DW_TAG_structure_type]           = "DW_TAG_structure_type";
	tagNameMap[DW_TAG_subroutine_type]          = "DW_TAG_subroutine_type";
	tagNameMap[DW_TAG_typedef]                  = "DW_TAG_typedef";
	tagNameMap[DW_TAG_union_type]               = "DW_TAG_union_type";
	tagNameMap[DW_TAG_unspecified_parameters]   = "DW_TAG_unspecified_parameters";
	tagNameMap[DW_TAG_variant]                  = "DW_TAG_variant";
	tagNameMap[DW_TAG_common_block]             = "DW_TAG_common_block";
	tagNameMap[DW_TAG_common_inclusion]         = "DW_TAG_common_inclusion";
	tagNameMap[DW_TAG_inheritance]              = "DW_TAG_inheritance";
	tagNameMap[DW_TAG_inlined_subroutine]       = "DW_TAG_inlined_subroutine";
	tagNameMap[DW_TAG_module]                   = "DW_TAG_module";
	tagNameMap[DW_TAG_ptr_to_member_type]       = "DW_TAG_ptr_to_member_type";
	tagNameMap[DW_TAG_set_type]                 = "DW_TAG_set_type";
	tagNameMap[DW_TAG_subrange_type]            = "DW_TAG_subrange_type";
	tagNameMap[DW_TAG_with_stmt]                = "DW_TAG_with_stmt";
	tagNameMap[DW_TAG_access_declaration]       = "DW_TAG_access_declaration";
	tagNameMap[DW_TAG_base_type]                = "DW_TAG_base_type";
	tagNameMap[DW_TAG_catch_block]              = "DW_TAG_catch_block";
	tagNameMap[DW_TAG_const_type]               = "DW_TAG_const_type";
	tagNameMap[DW_TAG_constant]                 = "DW_TAG_constant";
	tagNameMap[DW_TAG_enumerator]               = "DW_TAG_enumerator";
	tagNameMap[DW_TAG_file_type]                = "DW_TAG_file_type";
	tagNameMap[DW_TAG_friend]                   = "DW_TAG_friend";
	tagNameMap[DW_TAG_namelist]                 = "DW_TAG_namelist";
	tagNameMap[DW_TAG_namelist_item]            = "DW_TAG_namelist_item";
	tagNameMap[DW_TAG_packed_type]              = "DW_TAG_packed_type";
	tagNameMap[DW_TAG_subprogram]               = "DW_TAG_subprogram";
	tagNameMap[DW_TAG_template_type_parameter]  = "DW_TAG_template_type_parameter";
	tagNameMap[DW_TAG_template_value_parameter] = "DW_TAG_template_value_parameter";
	tagNameMap[DW_TAG_thrown_type]              = "DW_TAG_thrown_type";
	tagNameMap[DW_TAG_try_block]                = "DW_TAG_try_block";
	tagNameMap[DW_TAG_variant_part]             = "DW_TAG_variant_part";
	tagNameMap[DW_TAG_variable]                 = "DW_TAG_variable";
	tagNameMap[DW_TAG_volatile_type]            = "DW_TAG_volatile_type";
	tagNameMap[DW_TAG_dwarf_procedure]          = "DW_TAG_dwarf_procedure";
	tagNameMap[DW_TAG_restrict_type]            = "DW_TAG_restrict_type";
	tagNameMap[DW_TAG_interface_type]           = "DW_TAG_interface_type";
	tagNameMap[DW_TAG_namespace]                = "DW_TAG_namespace";
	tagNameMap[DW_TAG_imported_module]          = "DW_TAG_imported_module";
	tagNameMap[DW_TAG_unspecified_type]         = "DW_TAG_unspecified_type";
	tagNameMap[DW_TAG_partial_unit]             = "DW_TAG_partial_unit";
	tagNameMap[DW_TAG_imported_unit]            = "DW_TAG_imported_unit";
	tagNameMap[DW_TAG_condition]                = "DW_TAG_condition";
	tagNameMap[DW_TAG_shared_type]              = "DW_TAG_shared_type";
	tagNameMap[DW_TAG_type_unit]                = "DW_TAG_type_unit";
	tagNameMap[DW_TAG_rvalue_reference_type]    = "DW_TAG_rvalue_reference_type";
	tagNameMap[DW_TAG_template_alias]           = "DW_TAG_template_alias";
	tagNameMap[DW_TAG_lo_user]                  = "TAG_lo_user";
	tagNameMap[DW_TAG_hi_user]                  = "TAG_hi_user";
    return tagNameMap;
}

std::map<uint64_t,std::string> Dwarf::getAttrNameMap()
{
    std::map<uint64_t, std::string> attrNameMap;
	attrNameMap[DW_AT_sibling]              =  "DW_AT_sibling";
	attrNameMap[DW_AT_location]             =  "DW_AT_location";
	attrNameMap[DW_AT_name]                 =  "DW_AT_name";
	attrNameMap[DW_AT_ordering]             =  "DW_AT_ordering";
	attrNameMap[DW_AT_byte_size]            =  "DW_AT_byte_size";
	attrNameMap[DW_AT_bit_offset]           =  "DW_AT_bit_offset";
	attrNameMap[DW_AT_bit_size]             =  "DW_AT_bit_size";
	attrNameMap[DW_AT_stmt_list]            =  "DW_AT_stmt_list";
	attrNameMap[DW_AT_low_pc]               =  "DW_AT_low_pc";
	attrNameMap[DW_AT_high_pc]              =  "DW_AT_high_pc";
	attrNameMap[DW_AT_language]             =  "DW_AT_language";
	attrNameMap[DW_AT_discr]                =  "DW_AT_discr";
	attrNameMap[DW_AT_discr_value]          =  "DW_AT_discr_value";
	attrNameMap[DW_AT_visibility]           =  "DW_AT_visibility";
	attrNameMap[DW_AT_import]               =  "DW_AT_import";
	attrNameMap[DW_AT_string_length]        =  "DW_AT_string_length";
	attrNameMap[DW_AT_common_reference]     =  "DW_AT_common_reference";
	attrNameMap[DW_AT_comp_dir]             =  "DW_AT_comp_dir";
	attrNameMap[DW_AT_const_value]          =  "DW_AT_const_value";
	attrNameMap[DW_AT_containing_type]      =  "DW_AT_containing_type";
	attrNameMap[DW_AT_default_value]        =  "DW_AT_default_value";
	attrNameMap[DW_AT_inline]               =  "DW_AT_inline";
	attrNameMap[DW_AT_is_optional]          =  "DW_AT_is_optional";
	attrNameMap[DW_AT_lower_bound]          =  "DW_AT_lower_bound";
	attrNameMap[DW_AT_producer]             =  "DW_AT_producer";
	attrNameMap[DW_AT_prototyped]           =  "DW_AT_prototyped";
	attrNameMap[DW_AT_return_addr]          =  "DW_AT_return_addr";
	attrNameMap[DW_AT_start_scope]          =  "DW_AT_start_scope";
	attrNameMap[DW_AT_bit_stride]           =  "DW_AT_bit_stride";
	attrNameMap[DW_AT_upper_bound]          =  "DW_AT_upper_bound";
	attrNameMap[DW_AT_abstract_origin]      =  "DW_AT_abstract_origin";
	attrNameMap[DW_AT_accessibility]        =  "DW_AT_accessibility";
	attrNameMap[DW_AT_address_class]        =  "DW_AT_address_class";
	attrNameMap[DW_AT_artificial]           =  "DW_AT_artificial";
	attrNameMap[DW_AT_base_types]           =  "DW_AT_base_types";
	attrNameMap[DW_AT_calling_convention]   =  "DW_AT_calling_convention";
	attrNameMap[DW_AT_count]                =  "DW_AT_count";
	attrNameMap[DW_AT_data_member_location] =  "DW_AT_data_member_location";
	attrNameMap[DW_AT_decl_column]          =  "DW_AT_decl_column";
	attrNameMap[DW_AT_decl_file]            =  "DW_AT_decl_file";
	attrNameMap[DW_AT_decl_line]            =  "DW_AT_decl_line";
	attrNameMap[DW_AT_declaration]          =  "DW_AT_declaration";
	attrNameMap[DW_AT_discr_list]           =  "DW_AT_discr_list";
	attrNameMap[DW_AT_encoding]             =  "DW_AT_encoding";
	attrNameMap[DW_AT_external]             =  "DW_AT_external";
	attrNameMap[DW_AT_frame_base]           =  "DW_AT_frame_base";
	attrNameMap[DW_AT_friend]               =  "DW_AT_friend";
	attrNameMap[DW_AT_identifier_case]      =  "DW_AT_identifier_case";
	attrNameMap[DW_AT_macro_info]           =  "DW_AT_macro_info";
	attrNameMap[DW_AT_namelist_item]        =  "DW_AT_namelist_item";
	attrNameMap[DW_AT_priority]             =  "DW_AT_priority";
	attrNameMap[DW_AT_segment]              =  "DW_AT_segment";
	attrNameMap[DW_AT_specification]        =  "DW_AT_specification";
	attrNameMap[DW_AT_static_link]          =  "DW_AT_static_link";
	attrNameMap[DW_AT_type]                 =  "DW_AT_type";
	attrNameMap[DW_AT_use_location]         =  "DW_AT_use_location";
	attrNameMap[DW_AT_variable_parameter]   =  "DW_AT_variable_parameter";
	attrNameMap[DW_AT_virtuality]           =  "DW_AT_virtuality";
	attrNameMap[DW_AT_vtable_elem_location] =  "DW_AT_vtable_elem_location";
	attrNameMap[DW_AT_allocated]            =  "DW_AT_allocated";
	attrNameMap[DW_AT_associated]           =  "DW_AT_associated";
	attrNameMap[DW_AT_data_location]        =  "DW_AT_data_location";
	attrNameMap[DW_AT_byte_stride]          =  "DW_AT_byte_stride";
	attrNameMap[DW_AT_entry_pc]             =  "DW_AT_entry_pc";
	attrNameMap[DW_AT_use_UTF8]             =  "DW_AT_use_UTF8";
	attrNameMap[DW_AT_extension]            =  "DW_AT_extension";
	attrNameMap[DW_AT_ranges]               =  "DW_AT_ranges";
	attrNameMap[DW_AT_trampoline]           =  "DW_AT_trampoline";
	attrNameMap[DW_AT_call_column]          =  "DW_AT_call_column";
	attrNameMap[DW_AT_call_file]            =  "DW_AT_call_file";
	attrNameMap[DW_AT_call_line]            =  "DW_AT_call_line";
	attrNameMap[DW_AT_description]          =  "DW_AT_description";
	attrNameMap[DW_AT_binary_scale]         =  "DW_AT_binary_scale";
	attrNameMap[DW_AT_decimal_scale]        =  "DW_AT_decimal_scale";
	attrNameMap[DW_AT_small]                =  "DW_AT_small";
	attrNameMap[DW_AT_decimal_sign]         =  "DW_AT_decimal_sign";
	attrNameMap[DW_AT_digit_count]          =  "DW_AT_digit_count";
	attrNameMap[DW_AT_picture_string]       =  "DW_AT_picture_string";
	attrNameMap[DW_AT_mutable]              =  "DW_AT_mutable";
	attrNameMap[DW_AT_threads_scaled]       =  "DW_AT_threads_scaled";
	attrNameMap[DW_AT_explicit]             =  "DW_AT_explicit";
	attrNameMap[DW_AT_object_pointer]       =  "DW_AT_object_pointer";
	attrNameMap[DW_AT_endianity]            =  "DW_AT_endianity";
	attrNameMap[DW_AT_elemental]            =  "DW_AT_elemental";
	attrNameMap[DW_AT_pure]                 =  "DW_AT_pure";
	attrNameMap[DW_AT_recursive]            =  "DW_AT_recursive";
	attrNameMap[DW_AT_signature]            =  "DW_AT_signature";
	attrNameMap[DW_AT_main_subprogram]      =  "DW_AT_main_subprogram";
	attrNameMap[DW_AT_data_bit_offset]      =  "DW_AT_data_bit_offset";
	attrNameMap[DW_AT_const_expr]           =  "DW_AT_const_expr";
	attrNameMap[DW_AT_enum_class]           =  "DW_AT_enum_class";
	attrNameMap[DW_AT_linkage_name]         =  "DW_AT_linkage_name";
	attrNameMap[DW_AT_lo_user]              =  "DW_AT_lo_user";
	attrNameMap[DW_AT_hi_user]              =  "DW_AT_hi_user";
    return attrNameMap;
}

std::map<uint64_t, std::string> Dwarf::getLangNameMap()
{
    std::map<uint64_t, std::string> langNameMap;
	langNameMap[DW_LANG_C89]            = "C89";
	langNameMap[DW_LANG_C]              = "C";
	langNameMap[DW_LANG_Ada83]          = "Ada83";
	langNameMap[DW_LANG_C_plus_plus]    = "C++";
	langNameMap[DW_LANG_Cobol74]        = "Cobol74";
	langNameMap[DW_LANG_Cobol85]        = "Cobol85";
	langNameMap[DW_LANG_Fortran77]      = "Fortran77";
	langNameMap[DW_LANG_Fortran90]      = "Fortran90";
	langNameMap[DW_LANG_Pascal83]       = "Pascal83";
	langNameMap[DW_LANG_Modula2]        = "Modula2";
	langNameMap[DW_LANG_Java]           = "Java";
	langNameMap[DW_LANG_C99]            = "C99";
	langNameMap[DW_LANG_Ada95]          = "Ada95";
	langNameMap[DW_LANG_Fortran95]      = "Fortran95";
	langNameMap[DW_LANG_PLI]            = "PLI";
	langNameMap[DW_LANG_ObjC]           = "Objective-C";
	langNameMap[DW_LANG_ObjC_plus_plus] = "Objective-C++";
	langNameMap[DW_LANG_UPC]            = "UPC";
	langNameMap[DW_LANG_D]              = "D";
	langNameMap[DW_LANG_Python]         = "Python";
	langNameMap[DW_LANG_OpenCL]         = "OpenCL";
	langNameMap[DW_LANG_Go]             = "Go";
	langNameMap[DW_LANG_Modula3]        = "Modula3";
	langNameMap[DW_LANG_Haskell]        = "Haskell";
	langNameMap[DW_LANG_C_plus_plus_03] = "C++03";
	langNameMap[DW_LANG_C_plus_plus_11] = "C++11";
	langNameMap[DW_LANG_OCaml]          = "OCaml";
	langNameMap[DW_LANG_Rust]           = "Rust";
	langNameMap[DW_LANG_C11]            = "C11";
	langNameMap[DW_LANG_Swift]          = "Swift";
	langNameMap[DW_LANG_Julia]          = "Julia";
	langNameMap[DW_LANG_Dylan]          = "Dylan";
	langNameMap[DW_LANG_C_plus_plus_14] = "C_plus_plus_14";
	langNameMap[DW_LANG_Fortran03]      = "Fortran03";
	langNameMap[DW_LANG_Fortran08]      = "Fortran08";
	langNameMap[DW_LANG_RenderScript]   = "RenderScript";
	langNameMap[DW_LANG_BLISS]          = "BLISS";
    return langNameMap;
}
