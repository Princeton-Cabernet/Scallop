
#include "sequence_rewriter.h"

#include <algorithm>

p4sfu::SequenceRewriter::Pkt::Sim::Sim(bool lost, std::optional<unsigned> expectedSequenceNumber)
    : lost(lost),
      expectedSequenceNumber(expectedSequenceNumber) { }

p4sfu::SequenceRewriter::Pkt::Pkt(std::uint16_t frame, std::uint16_t seq, Mark mark, bool drop,
    std::optional<Sim> sim)
    : frame(frame),
      seq(seq),
      mark(mark),
      drop(drop),
      sim(sim) { }

p4sfu::SequenceRewriter::Pkt::Pkt(std::uint16_t frame, std::uint16_t seq, Mark mark, bool drop,
    Sim sim)
    : Pkt(frame, seq, mark, drop, std::make_optional(sim)) { }

std::optional<unsigned> p4sfu::SequenceRewriter::operator()(std::uint16_t frame,
        std::uint16_t seq, bool startOfFrame, bool endOfFrame, bool drop) {

    return (*this)(Pkt{frame, seq, _markFromAV1FrameFlags(startOfFrame, endOfFrame), drop});
}

std::optional<unsigned> p4sfu::SequenceRewriter::operator()(const Pkt& pkt) {

    using Mark = Pkt::Mark;
    std::optional<unsigned> rewrittenSeq;

    // if the packet is marked as lost (for simulation purposes only)
    if (pkt.sim.has_value() && pkt.sim->lost) {
        return std::nullopt;
    }

    if (pkt.frame > 1 && _maxFrame == 0) { // first packet

        _maxFrame = pkt.frame;
        _dropMaxFrame = pkt.drop;

        _maxFrameStartSeq = pkt.mark == Mark::start ? pkt.seq : pkt.seq - 1;
        _maxFrameMaxSeq = pkt.mark == Mark::end ? pkt.seq : pkt.seq + 1;

        if (!pkt.drop) {
            rewrittenSeq = pkt.seq;
        } else {
            rewrittenSeq = std::nullopt;

            if (pkt.mark == Mark::end) {
                _dropCount = 2;
            }
        }

        if (pkt.mark == Mark::end) {
            _maxFrameEndSeen = true;
        }

    } else if (pkt.frame == _maxFrame) { // in the same frame

        if (pkt.mark == Mark::start) {
            _maxFrameStartSeq = pkt.seq;
        } else if (pkt.seq == _maxFrameStartSeq) {
            _maxFrameStartSeq = pkt.seq - 1;
        }

        if (pkt.mark == Mark::end) {
            _maxFrameMaxSeq = pkt.seq;
        } else if (!_maxFrameEndSeen) {
            _maxFrameMaxSeq = pkt.seq + 1;
        }

        if (!pkt.drop) {
            rewrittenSeq = pkt.seq - _dropCount;
        } else {
            rewrittenSeq = std::nullopt;

            if (pkt.mark == Mark::end) {
                _dropCount += (pkt.seq - _maxFrameStartSeq + 1);
            }
        }

        if (pkt.mark == Mark::end) {
            _maxFrameEndSeen = true;
        }

    } else if (pkt.frame == _maxFrame + 1) { // moved to the next consecutive frame

        _maxFrame = pkt.frame;

        if (_dropMaxFrame && !_maxFrameEndSeen) {
            _dropCount += pkt.mark == Mark::start ? pkt.seq - _maxFrameStartSeq : 3;
        } else if (pkt.drop && pkt.mark == Mark::end) {
            _dropCount += _maxFrameEndSeen ? pkt.seq - _maxFrameMaxSeq : 2;
        }

        _dropMaxFrame = pkt.drop;
        _maxFrameStartSeq = pkt.mark == Mark::start ? pkt.seq : pkt.seq - 1;
        _maxFrameMaxSeq = pkt.mark == Mark::end ? pkt.seq : pkt.seq + 1;

        if (!pkt.drop) {
            rewrittenSeq = pkt.seq - _dropCount;
        } else {
            rewrittenSeq = std::nullopt;
        }

        _maxFrameEndSeen = pkt.mark == Mark::end;

    } else if (pkt.frame > _maxFrame + 1) { // skipped to a future frame

        if (!_dropMaxFrame && !pkt.drop && pkt.frame - _maxFrame + 1 >= SKIP_CADENCE) {
            _dropCount += 2;
        }

        _maxFrame = pkt.frame;

        if (_dropMaxFrame && !_maxFrameEndSeen) {
            _dropCount += _maxFrameMaxSeq - _maxFrameStartSeq;
        }

        _dropMaxFrame = pkt.drop;

        if (pkt.mark == Mark::start) {
            _maxFrameStartSeq = pkt.seq;
        } else if (pkt.seq == _maxFrameStartSeq) {
            _maxFrameStartSeq = pkt.seq - 1;
        } else if (pkt.mark == Mark::inter) {
            _maxFrameStartSeq = pkt.seq - 1;
        } else if (pkt.mark == Mark::end) {
            _maxFrameStartSeq = pkt.seq - 2;
        }

        _maxFrameMaxSeq = pkt.mark == Mark::end ? pkt.seq : pkt.seq + 1;

        if (!pkt.drop) {
            rewrittenSeq = pkt.seq - _dropCount;
        } else {
            rewrittenSeq = std::nullopt;

            if (pkt.mark == Mark::end) {
                _dropCount += 2;
            }
        }

        _maxFrameEndSeen = pkt.mark == Mark::end;

    } else if (pkt.frame < _maxFrame) { // skipped to a past frame

        if (pkt.drop) {
            rewrittenSeq = std::nullopt;
        } else if (pkt.frame + 1 == _maxFrame) {

            if (!_dropMaxFrame) {
                rewrittenSeq = pkt.seq - _dropCount;
            } else if (!_maxFrameEndSeen) {
                rewrittenSeq = pkt.seq - _dropCount;
            } else {
                rewrittenSeq = pkt.seq - (_dropCount - (_maxFrameMaxSeq-_maxFrameStartSeq+1));
            }
        } else if (pkt.frame > _maxDropFrame) {
            rewrittenSeq = pkt.seq - _dropCount;
        } else {

            if (_dropWhenUnsure) {
                rewrittenSeq = std::nullopt;
            } else {
                rewrittenSeq = pkt.seq - _dropCount;
            }
        }
    }

    // adjust max dropped frame number:
    if (pkt.drop) {
        _maxDropFrame = std::max(pkt.frame, _maxDropFrame);
    }

    return rewrittenSeq;
}

p4sfu::SequenceRewriter::Pkt::Mark p4sfu::SequenceRewriter::_markFromAV1FrameFlags(
    bool start, bool end) {

    // Does not handle the case where a packet is both the start and end of a frame.
    // Frame gets marked as end in this case for now.

    auto mark = Pkt::Mark::inter;

    if (start) {
        mark = Pkt::Mark::start;
    }

    if (end) {
        mark = Pkt::Mark::end;
    }

    return mark;
}
