#pragma once

namespace nara {

enum gomoku_chess { EMPTY, OUTBX, BLACK, WHITE };

gomoku_chess oppof(gomoku_chess chess) {
    // assert(chess == BLACK || chess == WHITE);
    return chess == BLACK ? WHITE : BLACK;
}

} // namespace nara
