#include <catch.h>
#include "av1.h"

TEST_CASE("av1", "[av1]") {

    // carries templates 0, 1, 2 - scalability mode: L1T2
    const unsigned char dd1_bytes[] = {
            0x80, 0x00, 0x01, 0x80, 0x01, 0x1e, 0xa8, 0x51,
            0x41, 0x01, 0x0c, 0x04, 0xfc, 0x03, 0xbc
    };

    // carries templates 6, 7, 8 - scalability mode: L1T2
    const unsigned char dd2_bytes[] = {
            0x86, 0x00, 0xb5, 0x80, 0xc1, 0x1e, 0xa8, 0x51,
            0x41, 0x01, 0x0c, 0x09, 0xfc, 0x07, 0x7c
    };

    // does not carry template dependency structure
    const unsigned char dd3_bytes[] = {0x08, 0x00, 0xd8};

    // carries templates 0, 1, 2, 3, 4, 5 - scalability mode: L2T2
    const unsigned char dd4_bytes[] = {
            0xc0, 0x00, 0x01, 0x80, 0x03, 0x18, 0x7a, 0xaa,
            0xf1, 0x30, 0xa0, 0xa0, 0x14, 0xd1, 0x41, 0x38,
            0x23, 0x04, 0x60, 0x08, 0x64, 0x22, 0x22, 0x26,
            0x50, 0x09, 0xf0, 0x07, 0x70, 0x13, 0xf0, 0x0e,
            0xf0
    };

    SECTION("L1T2 template dependency structure - 1") {

        av1::DependencyDescriptor dd1{dd1_bytes, 15};
        CHECK(dd1.mandatoryFields().startOfFrame());
        CHECK_FALSE(dd1.mandatoryFields().endOfFrame());
        CHECK(dd1.mandatoryFields().templateId() == 0);
        CHECK(dd1.mandatoryFields().frameNumber() == 1);

        CHECK(dd1.template_dependency_structure_present_flag());
        CHECK_FALSE(dd1.active_decode_targets_present_flag());
        CHECK_FALSE(dd1.custom_dtis_flag());
        CHECK_FALSE(dd1.custom_fdiffs_flag());
        CHECK_FALSE(dd1.custom_chains_flag());

        CHECK(dd1.dtCnt() == 2);

        CHECK(dd1.templates().size() == 3);
        CHECK(dd1.templates().find(0) != dd1.templates().end());
        CHECK(dd1.templates().find(1) != dd1.templates().end());
        CHECK(dd1.templates().find(2) != dd1.templates().end());

        const auto& dd1_tpl0 = dd1.templates().find(0)->second;
        const auto& dd1_tpl1 = dd1.templates().find(1)->second;
        const auto& dd1_tpl2 = dd1.templates().find(2)->second;

        CHECK(dd1_tpl0.spatial_layer_id == 0);
        CHECK(dd1_tpl0.temporal_layer_id == 0);
        CHECK(dd1_tpl0.dtis.size() == 2);
        CHECK(dd1_tpl0.dtis[0] == av1::DependencyDescriptor::dti::switch_indication);
        CHECK(dd1_tpl0.dtis[1] == av1::DependencyDescriptor::dti::switch_indication);
        // CHECK(dd1_tpl0.fdiffs.empty());

        CHECK(dd1_tpl1.spatial_layer_id == 0);
        CHECK(dd1_tpl1.temporal_layer_id == 0);
        CHECK(dd1_tpl1.dtis.size() == 2);
        CHECK(dd1_tpl1.dtis[0] == av1::DependencyDescriptor::dti::switch_indication);
        CHECK(dd1_tpl1.dtis[1] == av1::DependencyDescriptor::dti::switch_indication);
        // CHECK(dd1_tpl1.fdiffs.size() == 1);
        // CHECK(dd1_tpl1.fdiffs[0] == 2);

        CHECK(dd1_tpl2.spatial_layer_id == 0);
        CHECK(dd1_tpl2.temporal_layer_id == 1);
        CHECK(dd1_tpl2.dtis.size() == 2);
        CHECK(dd1_tpl2.dtis[0] == av1::DependencyDescriptor::dti::not_present_indication);
        CHECK(dd1_tpl2.dtis[1] == av1::DependencyDescriptor::dti::discardable_indication);
        // CHECK(dd1_tpl2.fdiffs.size() == 1);
        // CHECK(dd1_tpl2.fdiffs[0] == 1);
    }

    SECTION("L1T2 template dependency structure - 2") {

        av1::DependencyDescriptor dd2{dd2_bytes, 15};
        CHECK(dd2.mandatoryFields().startOfFrame());
        CHECK_FALSE(dd2.mandatoryFields().endOfFrame());
        CHECK(dd2.mandatoryFields().templateId() == 6);
        CHECK(dd2.mandatoryFields().frameNumber() == 181);

        CHECK(dd2.template_dependency_structure_present_flag());
        CHECK_FALSE(dd2.active_decode_targets_present_flag());
        CHECK_FALSE(dd2.custom_dtis_flag());
        CHECK_FALSE(dd2.custom_fdiffs_flag());
        CHECK_FALSE(dd2.custom_chains_flag());

        CHECK(dd2.dtCnt() == 2);

        CHECK(dd2.templates().size() == 3);
        CHECK(dd2.templates().find(6) != dd2.templates().end());
        CHECK(dd2.templates().find(7) != dd2.templates().end());
        CHECK(dd2.templates().find(8) != dd2.templates().end());

        const auto& dd2_tpl6 = dd2.templates().find(6)->second;
        const auto& dd2_tpl7 = dd2.templates().find(7)->second;
        const auto& dd2_tpl8 = dd2.templates().find(8)->second;

        CHECK(dd2_tpl6.spatial_layer_id == 0);
        CHECK(dd2_tpl6.temporal_layer_id == 0);
        CHECK(dd2_tpl6.dtis.size() == 2);
        CHECK(dd2_tpl6.dtis[0] == av1::DependencyDescriptor::dti::switch_indication);
        CHECK(dd2_tpl6.dtis[1] == av1::DependencyDescriptor::dti::switch_indication);
        // CHECK(dd2_tpl6.fdiffs.empty());

        CHECK(dd2_tpl7.spatial_layer_id == 0);
        CHECK(dd2_tpl7.temporal_layer_id == 0);
        CHECK(dd2_tpl7.dtis.size() == 2);
        CHECK(dd2_tpl7.dtis[0] == av1::DependencyDescriptor::dti::switch_indication);
        CHECK(dd2_tpl7.dtis[1] == av1::DependencyDescriptor::dti::switch_indication);
        // CHECK(dd2_tpl7.fdiffs.size() == 1);
        // CHECK(dd2_tpl7.fdiffs[0] == 2);

        CHECK(dd2_tpl8.spatial_layer_id == 0);
        CHECK(dd2_tpl8.temporal_layer_id == 1);
        CHECK(dd2_tpl8.dtis.size() == 2);
        CHECK(dd2_tpl8.dtis[0] == av1::DependencyDescriptor::dti::not_present_indication);
        CHECK(dd2_tpl8.dtis[1] == av1::DependencyDescriptor::dti::discardable_indication);
        // CHECK(dd2_tpl8.fdiffs.size() == 1);
        // CHECK(dd2_tpl8.fdiffs[0] == 1);
    }

    SECTION("mandatory fields") {

        class av1::DependencyDescriptor::MandatoryFields dd3{dd3_bytes};
        CHECK_FALSE(dd3.startOfFrame());
        CHECK_FALSE(dd3.endOfFrame());
        CHECK(dd3.templateId() == 8);
        CHECK(dd3.frameNumber() == 216);
    }

    SECTION("L2T2 dependency structure") {

        av1::DependencyDescriptor dd4{dd4_bytes, 33};
        CHECK(dd4.mandatoryFields().startOfFrame());
        CHECK(dd4.mandatoryFields().endOfFrame());
        CHECK(dd4.mandatoryFields().templateId() == 0);
        CHECK(dd4.mandatoryFields().frameNumber() == 1);

        CHECK(dd4.template_dependency_structure_present_flag());
        CHECK_FALSE(dd4.active_decode_targets_present_flag());
        CHECK_FALSE(dd4.custom_dtis_flag());
        CHECK_FALSE(dd4.custom_fdiffs_flag());
        CHECK_FALSE(dd4.custom_chains_flag());

        CHECK(dd4.dtCnt() == 4);

        CHECK(dd4.templates().size() == 6);

        CHECK(dd4.templates().find(0) != dd4.templates().end());
        CHECK(dd4.templates().find(1) != dd4.templates().end());
        CHECK(dd4.templates().find(2) != dd4.templates().end());
        CHECK(dd4.templates().find(3) != dd4.templates().end());
        CHECK(dd4.templates().find(4) != dd4.templates().end());
        CHECK(dd4.templates().find(5) != dd4.templates().end());

        const auto& dd4_tpl0 = dd4.templates().find(0)->second;
        const auto& dd4_tpl1 = dd4.templates().find(1)->second;
        const auto& dd4_tpl2 = dd4.templates().find(2)->second;
        const auto& dd4_tpl3 = dd4.templates().find(3)->second;
        const auto& dd4_tpl4 = dd4.templates().find(4)->second;
        const auto& dd4_tpl5 = dd4.templates().find(5)->second;

        CHECK(dd4_tpl0.spatial_layer_id == 0);
        CHECK(dd4_tpl0.temporal_layer_id == 0);
        CHECK(dd4_tpl1.dtis.size() == 4);
        CHECK(dd4_tpl0.dtis[0] == av1::DependencyDescriptor::dti::switch_indication);
        CHECK(dd4_tpl0.dtis[1] == av1::DependencyDescriptor::dti::switch_indication);
        CHECK(dd4_tpl0.dtis[2] == av1::DependencyDescriptor::dti::switch_indication);
        CHECK(dd4_tpl0.dtis[3] == av1::DependencyDescriptor::dti::switch_indication);
        // CHECK(dd4_tpl0.fdiffs.empty());

        CHECK(dd4_tpl1.spatial_layer_id == 0);
        CHECK(dd4_tpl1.temporal_layer_id == 0);
        CHECK(dd4_tpl1.dtis.size() == 4);
        CHECK(dd4_tpl1.dtis[0] == av1::DependencyDescriptor::dti::switch_indication);
        CHECK(dd4_tpl1.dtis[1] == av1::DependencyDescriptor::dti::switch_indication);
        CHECK(dd4_tpl1.dtis[2] == av1::DependencyDescriptor::dti::required_indication);
        CHECK(dd4_tpl1.dtis[3] == av1::DependencyDescriptor::dti::required_indication);
        // CHECK(dd4_tpl1.fdiffs.size() == 1);
        // CHECK(dd4_tpl1.fdiffs[0] == 4);

        CHECK(dd4_tpl2.spatial_layer_id == 0);
        CHECK(dd4_tpl2.temporal_layer_id == 1);
        CHECK(dd4_tpl2.dtis.size() == 4);
        CHECK(dd4_tpl2.dtis[0] == av1::DependencyDescriptor::dti::not_present_indication);
        CHECK(dd4_tpl2.dtis[1] == av1::DependencyDescriptor::dti::discardable_indication);
        CHECK(dd4_tpl2.dtis[2] == av1::DependencyDescriptor::dti::not_present_indication);
        CHECK(dd4_tpl2.dtis[3] == av1::DependencyDescriptor::dti::required_indication);
        // CHECK(dd4_tpl2.fdiffs.size() == 1);
        // CHECK(dd4_tpl2.fdiffs[0] == 2);

        CHECK(dd4_tpl3.spatial_layer_id == 1);
        CHECK(dd4_tpl3.temporal_layer_id == 0);
        CHECK(dd4_tpl2.dtis.size() == 4);
        CHECK(dd4_tpl3.dtis[0] == av1::DependencyDescriptor::dti::not_present_indication);
        CHECK(dd4_tpl3.dtis[1] == av1::DependencyDescriptor::dti::not_present_indication);
        CHECK(dd4_tpl3.dtis[2] == av1::DependencyDescriptor::dti::switch_indication);
        CHECK(dd4_tpl3.dtis[3] == av1::DependencyDescriptor::dti::switch_indication);
        // CHECK(dd4_tpl3.fdiffs.size() == 1);
        // CHECK(dd4_tpl3.fdiffs[0] == 1);

        CHECK(dd4_tpl4.spatial_layer_id == 1);
        CHECK(dd4_tpl4.temporal_layer_id == 0);
        CHECK(dd4_tpl4.dtis.size() == 4);
        CHECK(dd4_tpl4.dtis[0] == av1::DependencyDescriptor::dti::not_present_indication);
        CHECK(dd4_tpl4.dtis[1] == av1::DependencyDescriptor::dti::not_present_indication);
        CHECK(dd4_tpl4.dtis[2] == av1::DependencyDescriptor::dti::switch_indication);
        CHECK(dd4_tpl4.dtis[3] == av1::DependencyDescriptor::dti::switch_indication);
        // CHECK(dd4_tpl4.fdiffs.size() == 2);
        // CHECK(dd4_tpl4.fdiffs[0] == 4);
        // CHECK(dd4_tpl4.fdiffs[1] == 1);

        CHECK(dd4_tpl5.spatial_layer_id == 1);
        CHECK(dd4_tpl5.temporal_layer_id == 1);
        CHECK(dd4_tpl5.dtis.size() == 4);
        CHECK(dd4_tpl5.dtis[0] == av1::DependencyDescriptor::dti::not_present_indication);
        CHECK(dd4_tpl5.dtis[1] == av1::DependencyDescriptor::dti::not_present_indication);
        CHECK(dd4_tpl5.dtis[2] == av1::DependencyDescriptor::dti::not_present_indication);
        CHECK(dd4_tpl5.dtis[3] == av1::DependencyDescriptor::dti::discardable_indication);
        // CHECK(dd4_tpl5.fdiffs.size() == 2);
        // CHECK(dd4_tpl5.fdiffs[0] == 2);
        // CHECK(dd4_tpl5.fdiffs[1] == 1);
    }
}

TEST_CASE("av1::svc::L1T2::drop", "[av1]") {

    av1::svc::L1T2 svc;

    CHECK_FALSE(svc.drop(0));
    CHECK_FALSE(svc.drop(1));
    CHECK_FALSE(svc.drop(2));

    svc.decodeTarget = av1::svc::L1T2::DecodeTarget::lo;
    CHECK_FALSE(svc.drop(0));
    CHECK_FALSE(svc.drop(1));
    CHECK(svc.drop(2));
    CHECK_FALSE(svc.drop(3));
    CHECK_FALSE(svc.drop(4));
    CHECK(svc.drop(5));
    CHECK_FALSE(svc.drop(6));
    CHECK_FALSE(svc.drop(7));
    CHECK(svc.drop(8));

    svc.decodeTarget = av1::svc::L1T2::DecodeTarget::hi;

    CHECK_FALSE(svc.drop(9));
    CHECK_FALSE(svc.drop(10));
    CHECK_FALSE(svc.drop(12));
}

TEST_CASE("av1::svc::L1T3::drop", "[av1]") {

    av1::svc::L1T3 svc;

    CHECK_FALSE(svc.drop(0));
    CHECK_FALSE(svc.drop(1));
    CHECK_FALSE(svc.drop(2));
    CHECK_FALSE(svc.drop(3));
    CHECK_FALSE(svc.drop(4));
    CHECK_FALSE(svc.drop(5));
    CHECK_FALSE(svc.drop(6));
    CHECK_FALSE(svc.drop(7));
    CHECK_FALSE(svc.drop(8));
    CHECK_FALSE(svc.drop(9));

    svc.decodeTarget = av1::svc::L1T3::DecodeTarget::lo;
    CHECK_FALSE(svc.drop(0));
    CHECK_FALSE(svc.drop(1));
    CHECK(svc.drop(2));
    CHECK(svc.drop(3));
    CHECK(svc.drop(4));
    CHECK_FALSE(svc.drop(5));
    CHECK_FALSE(svc.drop(6));
    CHECK(svc.drop(7));
    CHECK(svc.drop(8));
    CHECK(svc.drop(9));

    svc.decodeTarget = av1::svc::L1T3::DecodeTarget::mid;
    CHECK_FALSE(svc.drop(0));
    CHECK_FALSE(svc.drop(1));
    CHECK_FALSE(svc.drop(2));
    CHECK(svc.drop(3));
    CHECK(svc.drop(4));
    CHECK_FALSE(svc.drop(5));
    CHECK_FALSE(svc.drop(6));
    CHECK_FALSE(svc.drop(7));
    CHECK(svc.drop(8));
    CHECK(svc.drop(9));

    svc.decodeTarget = av1::svc::L1T3::DecodeTarget::hi;
    CHECK_FALSE(svc.drop(0));
    CHECK_FALSE(svc.drop(1));
    CHECK_FALSE(svc.drop(2));
    CHECK_FALSE(svc.drop(3));
    CHECK_FALSE(svc.drop(4));
    CHECK_FALSE(svc.drop(5));
    CHECK_FALSE(svc.drop(6));
    CHECK_FALSE(svc.drop(7));
    CHECK_FALSE(svc.drop(8));
    CHECK_FALSE(svc.drop(9));
}