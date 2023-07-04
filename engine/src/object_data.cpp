#include "engine/game/object_data.h"

namespace eng {

    //===== ObjectID =====

    std::ostream& operator<<(std::ostream& os, const ObjectID& id) {
        os << "(" << id.type << "," << id.idx << "," << id.id << ")";
        return os;
    }

}//namespace eng
