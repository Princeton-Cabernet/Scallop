
#include <catch.h>

#include <sequence_rewriter.h>

TEST_CASE("SequenceRewriter::()", "[sequence_rewriter]") {

    using Mark = p4sfu::SequenceRewriter::Pkt::Mark;
    using Sim = p4sfu::SequenceRewriter::Pkt::Sim;

    p4sfu::SequenceRewriter rewriter;

    SECTION("rewrites a sequence without packet loss") {

        std::vector<p4sfu::SequenceRewriter::Pkt> pkts= {
            { 3, 655, Mark::start, false, { false, 655 } },
            { 3, 656, Mark::inter, false, { false, 656 } },
            { 3, 657, Mark::end,   false, { false, 657 } },
            { 4, 658, Mark::start, true,  { false, std::nullopt } },
            { 4, 659, Mark::inter, true,  { false, std::nullopt } },
            { 4, 660, Mark::end,   true,  { false, std::nullopt } },
            { 5, 661, Mark::start, false, { false, 658 } },
            { 5, 662, Mark::inter, false, { false, 659 } },
            { 5, 663, Mark::inter, false, { false, 660 } },
            { 5, 664, Mark::inter, false, { false, 661 } },
            { 5, 665, Mark::end,   false, { false, 662 } },
            { 6, 666, Mark::start, true,  { false, std::nullopt } },
            { 6, 667, Mark::inter, true,  { false, std::nullopt } },
            { 6, 668, Mark::inter, true,  { false, std::nullopt } },
            { 6, 669, Mark::end,   true,  { false, std::nullopt } },
            { 7, 670, Mark::start, false, { false, 663 } },
            { 7, 671, Mark::inter, false, { false, 664 } },
            { 7, 672, Mark::inter, false, { false, 665 } },
            { 7, 673, Mark::end,   false, { false, 666 } }
        };

        for (const auto& pkt : pkts) {
            auto seq = rewriter(pkt);
            CHECK(seq == pkt.sim->expectedSequenceNumber);
        }
    }

    SECTION("rewrites a sequence under loss") {

        std::vector<p4sfu::SequenceRewriter::Pkt> pkts= {
            { 3, 655, Mark::start, false, { false, 655 } },
            { 3, 656, Mark::inter, false, { true,  std::nullopt } },
            { 3, 657, Mark::end,   false, { false, 657 } },
            { 4, 658, Mark::start, true,  { false, std::nullopt } },
            { 4, 659, Mark::inter, true,  { true,  std::nullopt } },
            { 4, 660, Mark::end,   true,  { true,  std::nullopt } },
            { 5, 661, Mark::start, false, { false, 658 } },
            { 5, 662, Mark::inter, false, { false, 659 } },
            { 5, 663, Mark::inter, false, { false, 660 } },
            { 5, 664, Mark::inter, false, { false, 661 } },
            { 5, 665, Mark::end,   false, { true,  std::nullopt } },
            { 6, 666, Mark::start, true,  { false, std::nullopt } },
            { 6, 667, Mark::inter, true,  { false, std::nullopt } },
            { 6, 668, Mark::inter, true,  { false, std::nullopt } },
            { 6, 669, Mark::end,   true,  { false, std::nullopt } },
            { 7, 670, Mark::start, false, { true,  std::nullopt } },
            { 7, 671, Mark::inter, false, { false, 664 } },
            { 7, 672, Mark::inter, false, { true,  std::nullopt } },
            { 7, 673, Mark::end,   false, { false, 666 } }
        };

        for (const auto& pkt : pkts) {
            auto seq = rewriter(pkt);
            CHECK(seq == pkt.sim->expectedSequenceNumber);
        }
    }
}
