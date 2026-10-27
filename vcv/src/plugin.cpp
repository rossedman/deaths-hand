#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
    pluginInstance = p;
    p->addModel(modelComplexOsc);
    p->addModel(modelDoombringer);
}
