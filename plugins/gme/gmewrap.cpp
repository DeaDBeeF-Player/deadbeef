#include "gmewrap.h"
#include "Sgc_Impl.h"

void gme_set_sgc_coleco_bios (void *bios) {
    Sgc_Impl::set_coleco_bios(bios);
}
