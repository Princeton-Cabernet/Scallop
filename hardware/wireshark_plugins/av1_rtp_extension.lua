-- Define the AV1 RTP Extension protocol
local av1_proto = Proto("av1", "AV1 RTP Extension")

-- Define the fields for the AV1 protocol
local f_frame_start = ProtoField.bool("av1.frame_start", "Frame Start")
local f_frame_end = ProtoField.bool("av1.frame_end", "Frame End")
local f_dependency_template_id = ProtoField.uint8("av1.dependency_template_id", "Dependency Template ID", base.DEC)
local f_frame_number = ProtoField.uint16("av1.frame_number", "Frame Number", base.DEC)

av1_proto.fields = {f_frame_start, f_frame_end, f_dependency_template_id, f_frame_number}

-- Reads and returns the One-Byte header extension elements.
-- Returns when it encounters ID=12 or exhausts the extension data.
local function parse_one_byte_extensions(buffer, offset, ext_end, pinfo, tree)
    while offset < ext_end do
        if buffer:len() < offset + 1 then
            break  -- Not enough data for extension
        end

        local ext_byte = buffer(offset, 1):uint()
        offset = offset + 1

        -- Skip padding bytes (zero)
        if ext_byte == 0 then
            -- Do nothing, move to the next byte
        else
            local ext_id   = bit32.rshift(ext_byte, 4)
            local ext_len  = bit32.band(ext_byte, 0x0F) + 1  -- Length is len+1 bytes

            -- Ensure there's enough data for the extension
            if buffer:len() < offset + ext_len then
                break  -- Not enough data
            end

            -- Check if this extension is ID 12
            if ext_id == 12 then
                local ext_data = buffer(offset, ext_len)

                -- Parse the AV1 extension data
                if ext_data:len() >= 3 then
                    local tvb = ext_data

                    local first_data_byte = tvb(0,1):uint()
                    local frame_start = bit32.band(first_data_byte, 0x80) ~= 0
                    local frame_end = bit32.band(first_data_byte, 0x40) ~= 0
                    local dependency_template_id = bit32.band(first_data_byte, 0x3F)
                    local frame_number = tvb(1,2):uint()

                    -- Add the AV1 protocol to the tree
                    local av1_tree = tree:add(av1_proto, tvb(0,3), "AV1 RTP Extension")
                    av1_tree:add(f_frame_start, tvb(0,1), frame_start)
                    av1_tree:add(f_frame_end, tvb(0,1), frame_end)
                    av1_tree:add(f_dependency_template_id, tvb(0,1), dependency_template_id)
                    av1_tree:add(f_frame_number, tvb(1,2), frame_number)

                    -- Update the protocol column to include AV1
                    pinfo.cols.protocol:append(" / AV1")

                    -- Stop after processing this extension
                    -- break
                end
            end

            -- Move to the next extension
            offset = offset + ext_len
        end
    end

    return offset
end

-- Reads and returns the Two-Byte header extension elements.
-- Returns when it encounters ID=12 or exhausts the extension data.
local function parse_two_byte_extensions(buffer, offset, ext_end, pinfo, tree)
    while offset < ext_end do
        if buffer:len() < offset + 2 then
            break  -- Not enough data for extension
        end

        local ext_id   = buffer(offset, 1):uint()
        local ext_len  = buffer(offset + 1, 1):uint()
        offset = offset + 2

        -- If ID==0 => it's padding in Two-Byte extension format
        if ext_id == 0 then
            -- skip or do any checks needed
        else
            local actual_len = ext_len  -- Some RFCs do ext_len + 1; check your environment

            if offset + actual_len > buffer:len() then
                break
            end

            if ext_id == 12 then
                local ext_data = buffer(offset, actual_len)

                -- Parse the AV1 extension data
                if ext_data:len() >= 3 then
                    local tvb = ext_data

                    local first_data_byte = tvb(0,1):uint()
                    local frame_start = bit32.band(first_data_byte, 0x80) ~= 0
                    local frame_end = bit32.band(first_data_byte, 0x40) ~= 0
                    local dependency_template_id = bit32.band(first_data_byte, 0x3F)
                    local frame_number = tvb(1,2):uint()

                    -- Add the AV1 protocol to the tree
                    local av1_tree = tree:add(av1_proto, tvb(0,3), "AV1 RTP Extension")
                    av1_tree:add(f_frame_start, tvb(0,1), frame_start)
                    av1_tree:add(f_frame_end, tvb(0,1), frame_end)
                    av1_tree:add(f_dependency_template_id, tvb(0,1), dependency_template_id)
                    av1_tree:add(f_frame_number, tvb(1,2), frame_number)

                    -- Update the protocol column to include AV1
                    pinfo.cols.protocol:append(" / AV1")

                    -- Stop after processing this extension
                    -- break
                end
            end

            -- Move to the next extension
            offset = offset + actual_len
        end
    end

    return offset
end

-- Define the AV1 dissector function
function av1_proto.dissector(buffer, pinfo, tree)
    -- Only process UDP packets where either source or destination port is 3000
    if pinfo.src_port ~= 3000 and pinfo.dst_port ~= 3000 then
        return
    end

    -- Check if the packet is too short for STUN, RTP, or RTCP
    if buffer:len() < 1 then
        return
    end

    -- Peek at the very first byte of the UDP payload
    local first_payload_byte = buffer(0,1):uint()
    local version_bits       = bit32.rshift(first_payload_byte, 6)

    -- Get the built-in RTP dissector
    local rtp_dissector  = Dissector.get("rtp")

    -- Check for RTP (version = 2)
    if version_bits == 2 then
        -- RTP packet
        pinfo.cols.protocol:set("RTP")
        rtp_dissector:call(buffer, pinfo, tree)
    else
        return
    end
    
    -- Now, parse the RTP packet manually to find the extensions
    local offset = 0

    -- Parse the RTP header
    local first_byte = buffer(offset,1):uint()
    local version = bit32.rshift(first_byte, 6)
    local padding = bit32.band(bit32.rshift(first_byte, 5), 0x01)
    local extension = bit32.band(bit32.rshift(first_byte, 4), 0x01)
    local csrc_count = bit32.band(first_byte, 0x0F)
    offset = offset + 1

    -- Second byte
    local second_byte = buffer(offset,1):uint()
    local marker = bit32.rshift(second_byte, 7)
    local payload_type = bit32.band(second_byte, 0x7F)
    offset = offset + 1

    -- Sequence number
    local sequence_number = buffer(offset,2):uint()
    offset = offset + 2

    -- Timestamp
    local timestamp = buffer(offset,4):uint()
    offset = offset + 4

    -- SSRC
    local ssrc = buffer(offset,4):uint()
    offset = offset + 4

    -- Skip CSRC identifiers if any
    offset = offset + csrc_count * 4

    -- Check for extension header
    if extension == 1 then
        -- Extension header
        if buffer:len() < offset + 4 then
            return  -- Not enough data for extension header
        end

        local ext_header = buffer(offset,4)
        local profile_specific = ext_header(0,2):uint()
        local ext_length = ext_header(2,2):uint() * 4  -- Length in bytes
        offset = offset + 4  -- Move past extension header

        -- Now parse the extensions
        local ext_end = offset + ext_length

        -- Decide which extension header format is in use based on profile
        -- RFC 5285 suggests 0xBEDE for One-Byte, 0x100 (or others) for Two-Byte
        if profile_specific == 0xBEDE then
            -- parse One-Byte header extensions
            parse_one_byte_extensions(buffer, offset, ext_end, pinfo, tree)
        else
            -- parse Two-Byte header extensions
            parse_two_byte_extensions(buffer, offset, ext_end, pinfo, tree)
        end
    end
end

-- Register the AV1 dissector for UDP port 3000
local udp_port_table = DissectorTable.get("udp.port")
udp_port_table:add(3000, av1_proto)