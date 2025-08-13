
#include "av1.h"
#include "util.h"

#include <stdexcept>

av1::DependencyDescriptor::MandatoryFields::MandatoryFields(const unsigned char* buf)
    : _buf(buf) { }

bool av1::DependencyDescriptor::MandatoryFields::startOfFrame() const {
    return ((_buf[0] >> 7) & 1) == 1;
}

bool av1::DependencyDescriptor::MandatoryFields::endOfFrame() const {
    return ((_buf[0] >> 6) & 1) == 1;
}

unsigned av1::DependencyDescriptor::MandatoryFields::templateId() const {
    return (_buf[0]) & 0x3f;
}

unsigned av1::DependencyDescriptor::MandatoryFields::frameNumber() const {
    return (_buf[1] << 8) + _buf[2];
}

av1::DependencyDescriptor::DependencyDescriptor(const unsigned char* bytes, unsigned len)
    : _buf(bytes), _len(len), _mandatoryFields(bytes) {

    if (len < MandatoryFields_LEN)
        throw std::logic_error("av1::DependencyDescriptor: len must be at least 3");

    if (len > MandatoryFields_LEN)
        _parse_bytes();
}

const class av1::DependencyDescriptor::MandatoryFields& av1::DependencyDescriptor::mandatoryFields() const {

    return _mandatoryFields;
}

bool av1::DependencyDescriptor::template_dependency_structure_present_flag() const {

    return _template_dependency_structure_present_flag;
}

bool av1::DependencyDescriptor::active_decode_targets_present_flag() const {

    return _active_decode_targets_present_flag;
}

bool av1::DependencyDescriptor::custom_dtis_flag() const {

    return _custom_dtis_flag;
}

bool av1::DependencyDescriptor::custom_fdiffs_flag() const {

    return _custom_fdiffs_flag;
}

bool av1::DependencyDescriptor::custom_chains_flag() const {

    return _custom_chains_flag;
}

unsigned av1::DependencyDescriptor::dtCnt() const {

    return _dt_cnt;
}

const std::map<unsigned, struct av1::DependencyDescriptor::frame_dependency_template>&
av1::DependencyDescriptor::templates() const {

    return _templates;
}

void av1::DependencyDescriptor::_parse_bytes() {

    auto bits = _bit_vector_from_bytes(_buf, _len);

    unsigned total_consumed_bits = MandatoryFields_LEN * 8;

    total_consumed_bits = _parse_extended_descriptor_fields(bits, total_consumed_bits);
    // total_consumed_bits = _parse_frame_dependency_definition(bits, total_consumed_bits);
}

unsigned av1::DependencyDescriptor::_parse_extended_descriptor_fields(
        const std::vector<bool>& bits, unsigned total_consumed_bits) {

    auto i = total_consumed_bits;

    _template_dependency_structure_present_flag = bits[i++];
    _active_decode_targets_present_flag = bits[i++];
    _custom_dtis_flag = bits[i++];
    _custom_fdiffs_flag = bits[i++];
    _custom_chains_flag = bits[i++];

    /*
     std::cout << "template_dependency_structure_present_flag: "
               <<  _template_dependency_structure_present_flag << std::endl;
     std::cout << "active_decode_targets_present_flag: "
               << _active_decode_targets_present_flag << std::endl;
     std::cout << "custom_dtis_flag: " << _custom_dtis_flag << std::endl;
     std::cout << "custom_fdiffs_flag: " << _custom_fdiffs_flag << std::endl;
     std::cout << "custom_chains_flag: " << _custom_chains_flag << std::endl;
    */

    if (_template_dependency_structure_present_flag) {
        i = _parse_template_dependency_structure(bits, i);
        _active_decode_targets_bitmask = (1 << _dt_cnt) - 1;
    }

    /*
    if (_active_decode_targets_present_flag) {
        _active_decode_targets_bitmask = util::extract_bits(bits, i, _dt_cnt);
        i += _dt_cnt;
    }
    */

    return i;
}

/*
unsigned av1::DependencyDescriptor::_parse_frame_dependency_definition(
        const std::vector<bool>& bits, unsigned total_consumed_bits) {

    auto i = total_consumed_bits;
    return i;
}
*/

unsigned av1::DependencyDescriptor::_parse_template_dependency_structure(
        const std::vector<bool>& bits, unsigned total_consumed_bits) {

    auto i = total_consumed_bits;

    _template_id_offset = util::extractBits(bits, i, 6);
    i += 6;
    auto dt_cnt_minus_one = util::extractBits(bits, i, 5);
    i += 5;

    // std::cout << "template_id_offset: " << _template_id_offset << std::endl;
    // std::cout << "dt_cnt_minus_one: " << dt_cnt_minus_one << std::endl;

    _dt_cnt = dt_cnt_minus_one + 1;

    unsigned max_spatial_id = 0, chain_cnt = 0;

    i = _parse_template_layers(bits, i, max_spatial_id);
    i = _parse_template_dtis(bits, i);

    /*
    i = _parse_template_fdiffs(bits, i);

    i = _parse_template_chains(bits, i, chain_cnt);

    i = _parse_decode_target_layers(bits, i);

    _resolutions_present_flag = util::extract_bits(bits, i, 1);
    i += 1;

    if (_resolutions_present_flag) {
       i = _parse_render_resolutions(bits, i, max_spatial_id);
    }
    */

    return i;
}

unsigned av1::DependencyDescriptor::_parse_template_layers(const std::vector<bool>& bits,
                                                            unsigned i, unsigned& max_spatial_id) {

    unsigned temporal_id = 0, spatial_id = 0, max_temporal_id = 0, next_layer_idc = 0;

    do {
        _template_spatial_id.push_back(spatial_id);
        _template_temporal_id.push_back(temporal_id);

        _templates[_template_id_offset + _template_cnt] = {spatial_id, temporal_id};

        _template_cnt++;

        next_layer_idc = util::extractBits(bits, i, 2);
        i += 2;

        if (next_layer_idc == 1) {
            temporal_id++;

            if (temporal_id > max_temporal_id) {
                max_temporal_id = temporal_id;
            }

        } else if (next_layer_idc == 2) {
            temporal_id = 0;
            spatial_id++;
        }

    } while (next_layer_idc != 3);

    max_spatial_id = spatial_id;

    /*
    std::cout << "_template_spatial_id: ";
    for (auto j = 0; j < _template_spatial_id.size(); j++) {
        std::cout << (j > 0 ? ", " : "") << j << " -> " << _template_spatial_id[j];
    }
    std::cout << std::endl;

    std::cout << "_template_temporal_id: ";
    for (auto j = 0; j < _template_temporal_id.size(); j++) {
        std::cout << (j > 0 ? ", " : "") << j << " -> " << _template_temporal_id[j];
    }
    std::cout << std::endl;

    std::cout << "max_spatial_id: " << max_spatial_id << std::endl;
    std::cout << "max_temporal_id: " << max_temporal_id << std::endl;
    */

    return i;
}

unsigned av1::DependencyDescriptor::_parse_template_dtis(const std::vector<bool>&bits, unsigned i) {

    for (auto template_index = 0; template_index < _template_cnt; template_index++) {
        for (auto dt_index = 0; dt_index < _dt_cnt; dt_index++) {
            auto dti = (enum dti) util::extractBits(bits, i, 2);
            _templates[template_index + _template_id_offset].dtis.push_back(dti);
            i += 2;
        }
    }

    return i;
}

/*
unsigned av1::DependencyDescriptor::_parse_template_fdiffs(const std::vector<bool>& bits, unsigned i) {

    unsigned fdiff_cnt = 0;

    for (auto template_index = 0; template_index < _template_cnt; template_index++) {

        fdiff_cnt = 0;
        unsigned fdiff_follows_flag = util::extractBits(bits, i, 1);
        i += 1;

        while (fdiff_follows_flag) {

            unsigned fdiff_minus_one = util::extractBits(bits, i, 4);
            i += 4;
            _templates[template_index + _template_id_offset].fdiffs.push_back(fdiff_minus_one + 1);
            fdiff_cnt++;
            fdiff_follows_flag = util::extractBits(bits, i, 1);
            i += 1;
        }
    }

    return i;
}
*/

unsigned av1::DependencyDescriptor::_parse_template_chains(const std::vector<bool>& bits,
                                                            unsigned i, unsigned& chain_cnt) {

    auto ns_chain_cnt = util::extractNonSymmetric(bits, i, _dt_cnt + 1);
    chain_cnt = ns_chain_cnt.first;
    i += ns_chain_cnt.second;

    // std::cout << "_dt_cnt: " << _dt_cnt << std::endl;
    // std::cout << "chain_cnt: " << chain_cnt << std::endl;

    if (chain_cnt == 0) {
        return i;
    }

    // std::cout << "decode_target_protected_by:" << std::endl;

    for (auto dt_index = 0; dt_index < _dt_cnt; dt_index++) {
        auto ns_decode_target_protected_by = util::extractNonSymmetric(bits, i, chain_cnt);
        i += ns_decode_target_protected_by.second;
        // std::cout << "  " << dt_index << " -> " << ns_decode_target_protected_by.first << std::endl;
    }

    return i;
}

unsigned av1::DependencyDescriptor::_parse_decode_target_layers(const std::vector<bool>& bits, unsigned i) {

    for (unsigned dt_index = 0; dt_index < _dt_cnt; dt_index++) {

        unsigned spatial_id = 0, temporal_id = 0;

        for (unsigned template_index = 0; template_index < _template_cnt; template_index++) {

            if (_template_dti[template_index][dt_index] != 0) {

                if (_template_spatial_id[template_index] > spatial_id) {
                    spatial_id = _template_spatial_id[template_index];
                }

                if (_template_temporal_id[template_index] > temporal_id) {
                    temporal_id = _template_temporal_id[template_index];
                }
            }
        }

        _decode_target_spatial_id.push_back(spatial_id);
        _decode_target_temporal_id.push_back(temporal_id);
    }

    /*
    std::cout << "_decode_target_spatial_id:" << std::endl;
    for (auto i = 0; i < _decode_target_spatial_id.size(); i++)
        std::cout << "  " << i << " -> " << _decode_target_spatial_id[i] << std::endl;

    std::cout << "_decode_target_temporal_id:" << std::endl;
    for (auto i = 0; i < _decode_target_temporal_id.size(); i++)
        std::cout << "  " << i << " -> " << _decode_target_temporal_id[i] << std::endl;
    */

    return i;
}

unsigned av1::DependencyDescriptor::_parse_render_resolutions(
        const std::vector<bool>& bits, unsigned i, const unsigned& max_spatial_id) {

    for (unsigned spatial_id = 0; spatial_id < max_spatial_id; spatial_id++) {
        _max_render_width_minus_one.push_back(util::extractBits(bits, i, 16));
        i += 16;
        _max_render_height_minus_one.push_back(util::extractBits(bits, i, 16));
        i += 16;
    }

    /*
    std::cout << "_max_render_width_minus_one:" << std::endl;
    for (auto i = 0; i < _max_render_width_minus_one.size(); i++)
        std::cout << "  " << i << " -> " << _max_render_width_minus_one[i] << std::endl;

    std::cout << "_max_render_height_minus_one:" << std::endl;
    for (auto i = 0; i < _max_render_height_minus_one.size(); i++)
        std::cout << "  " << i << " -> " << _max_render_height_minus_one[i] << std::endl;
    */

    return i;
}

unsigned av1::DependencyDescriptor::_parse_frame_chains(const std::vector<bool>& bits, unsigned i,
                                                         unsigned& chain_cnt) {

    // std::cout << "frame_chain_diff:" << std::endl;
    for (auto chain_index = 0; chain_index < chain_cnt; chain_index++) {
        // std::cout << "  " << chain_index << " -> " << util::extract_bits(bits, i, 4)
        //           << std::endl;
        i += 4;
    }

    return i;
}

unsigned av1::DependencyDescriptor::_parse_frame_dependency_definition(
        const std::vector<bool>& bits, unsigned i) {

    unsigned template_index = (mandatoryFields().templateId() + 64 - _template_id_offset) % 64;

    if (template_index > _template_cnt) {
        throw std::logic_error("_parse_frame_dependency_definition: template_index < template_cnt");
    }

    auto frame_spatial_id = _template_spatial_id[template_index];
    auto frame_temporal_id = _template_temporal_id[template_index];

    if (_custom_dtis_flag) {
        throw std::logic_error("_custom_dtis_flag: not supported");
    }

    if (_custom_fdiffs_flag) {
        throw std::logic_error("_custom_fdiffs_flag: not supported");
    }

    if (_custom_chains_flag) {
        throw std::logic_error("_custom_chains_flag: not supported");
    }

    if (_resolutions_present_flag) {
        auto frame_max_width =  _max_render_width_minus_one[frame_spatial_id] + 1;
        auto frame_max_height = _max_render_height_minus_one[frame_spatial_id] + 1;
    }

    return i;
}

std::vector<bool> av1::DependencyDescriptor::_bit_vector_from_bytes(
        const unsigned char* buf, unsigned len) {

    std::vector<bool> bits(len * 8);

    for (auto i = 0; i < len; i++)
        for (auto j = 7; j >= 0; j--)
            bits[(i + 1) * 8 - j - 1] = ((buf[i] >> j) & 1);

    return bits;
}
