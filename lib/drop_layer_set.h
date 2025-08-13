
#ifndef P4SFU_DROP_LAYER_SET_H
#define P4SFU_DROP_LAYER_SET_H

#include <unordered_set>
#include <stdexcept>

namespace p4sfu {
    class DropLayerSet {
    public:
        void addDropLayer(unsigned layer) {

            if (!_dropLayers.contains(layer)) {
                _dropLayers.insert(layer);
            } else {
                throw std::logic_error("SequenceRewriter: addDropLayer: already drops layer "
                                       + std::to_string(layer));
            }
        }

        [[nodiscard]] bool dropsLayer(unsigned layer) const {

            return _dropLayers.contains(layer);
        }

        void removeDropLayer(unsigned layer) {

            if (_dropLayers.contains(layer)) {
                _dropLayers.erase(layer);
            } else {
                throw std::logic_error("SequenceRewriter: removeDropLayer: does not contain layer "
                                       + std::to_string(layer));
            }
        }

        void dropT2() {

            for (auto i = 2; i < 30; i += 3) {
                addDropLayer(i);
            }
        }

    private:
        std::unordered_set<unsigned> _dropLayers;
    };
}

#endif
