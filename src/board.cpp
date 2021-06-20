#include "board.h"
#include "log.h"
#include <cstring>
#include <cstdlib>

constexpr MoveFlag PROMOTIONS[] = { N_PROM, B_PROM, R_PROM, Q_PROM };
constexpr auto P_ATTACKS = attacks_pawn();
constexpr auto N_ATTACKS = attacks<KNIGHT>();
constexpr auto K_ATTACKS = attacks<KING>();
const auto B_ATTACKS = attacks_magic<BISHOP>(B_MAGIC_NUM);
const auto R_ATTACKS = attacks_magic<ROOK>(R_MAGIC_NUM);

static bool str_to_int(const char *str, long *val, int base, char **end)
{
    long res = strtol(str, end, base);

    if (*end > str) {
        *val = res;
        return true;
    }
    return false;
}

void Board::print() const
{
    LOGC('\n');
    for (Rank r = RANK_8; r >= RANK_1; --r) {
        LOG("%c   ", rank_c(r));
        for (File f = FILE_A; f <= FILE_H; ++f) {

            Square s    = sq(r, f);
            char c      = ' ';

            if (!get(all(), s))
                c = '.';
            else if (get(kings, s))
                c = 'k';
            else if (get(pawns(), s))
                c = 'p';
            else if (get(bishops, s))
                c = get(rooks, s) ? 'q' : 'b';
            else if (get(rooks, s))
                c = 'r';
            else
                c = 'n';

            if (get(pieces[WHITE], s)) 
                c ^= bit(5);

            LOG("%c ", c);
        }
        LOGC('\n');
    }
    LOG("\n   ");
    for (File f = FILE_A; f <= FILE_H; ++f)
        LOG(" %c", file_c(f));
    LOG("\n\n");
    LOG("cast:  %c%c%c%c    \n", 
        castle & WKCA ? 'K' : '-', castle & WQCA ? 'Q' : '-', 
        castle & BKCA ? 'k' : '-', castle & BQCA ? 'q' : '-');
    LOG("enps:  %c          \n", enpass() ? file_c(File(enpass() - 1)) : '-');
    LOG("half:  %u          \n", half_clk);
    LOG("full:  %u          \n", full_clk);
    LOG("side:  %c          \n", side_c(side));
    LOG("size:  %lu bytes   \n", sizeof(Board));
    LOGC('\n');
}

void Board::clr_pos()
{
    memset(reinterpret_cast<void*>(this), 0, sizeof(Board));
}

bool Board::set_pos(const char *c)
{
    LOG("%s: %s \n", __func__, c);

    clr_pos();

    Square s    = A8;

    while (*c != ' ') {

        if (*c >= '1' && *c <= '8') {
            s += Direction(EAST  * (*c - '0'));
        } else if (*c == '/') {
            s += Direction(SOUTH * 2);
        } else {
            switch (*c) {
                case 'p':
                case 'P': set(pawns_en, s); break;
                case 'n':
                case 'N':                   break;
                case 'b':
                case 'B': set(bishops, s);  break;
                case 'q':
                case 'Q': set(bishops, s);
                case 'r':
                case 'R': set(rooks, s);    break;
                case 'k':
                case 'K': set(kings, s);    break;
                default: return false;
            }
            if (*c & bit(5))
                set(pieces[BLACK], s);
            else
                set(pieces[WHITE], s);
            ++s;
        }
        ++c;

        if (s < A1 || s > H8 + 1)
            return false;
    }
    ++c;

    switch (*c++) {
        case 'w': side = WHITE; break;
        case 'b': side = BLACK; break;
        default: return false;
    }
    if (*c++ != ' ')
        return false;

    constexpr const char *castle_str = "KQkq";

    if (*c != '-') {
        for (int i = 0; i < 4; ++i, ++c) {
            if (*c != castle_str[i] &&
                *c != '-') 
            {
                return false;
            }
            if (*c != '-')
                castle |= 1 << i;
        }
    } else {
        ++c;
    }
    if (*c++ != ' ')
        return false;

    if (*c != '-') {

        File f = File(c[0] - 'a');
        Rank r = Rank(c[1] - '1');

        if (f < FILE_A || 
            f > FILE_H || 
            (side == WHITE && r != RANK_6) || 
            (side == BLACK && r != RANK_3))
        {
            return false;
        }
        set(pawns_en, sq(RANK_1, f));
        ++c;
    }
    ++c;

    if (*c++ != ' ')
        return false;

    long val;
    char *end = NULL;

    if (!str_to_int(c, &val, 10, &end) || val >= 100)
        return false; 
    half_clk = val;

    c = end;

    if (*c++ != ' ')
        return false;

    if (!str_to_int(c, &val, 10, &end) || val < 1)
        return false; 
    full_clk = (val - 1) * 2 + side;

    LOG("%s: remainder - [%s] \n", __func__, end);

    return true;
}

bool Board::attacked(Square s, Color att_clr) const 
{
    Bitboard they = pieces[att_clr];

    // Check bishops and queens
    if (B_ATTACKS[s][all()] & they & bishops)
        return true;
    // Check rooks and queens
    if (R_ATTACKS[s][all()] & they & rooks)
        return true;
    // Check pawns
    if (P_ATTACKS[!att_clr][s] & they & pawns())
        return true;
    // Check knights
    if (N_ATTACKS[s] & they & knights())
        return true;
    // Check king
    if (K_ATTACKS[s] & they & kings)
        return true;

    return false;
}

MoveList Board::moves_pseudo(/*Move *list, size_t max*/) const
{
    Bitboard attacker = pieces[side];
    Bitboard both     = all();

    MoveList list;
    list.reserve(100);

    auto save = [&] (Square src, Square dst) 
    {
        list.push_back(mv(src, dst));
    };
    auto save_f = [&] (Square src, Square dst, MoveFlag flag) 
    {
        list.push_back(mv(src, dst, flag));
    };
    // Pawn
    for (auto src : BitIter(attacker & pawns())) {

        Direction dir   = side == WHITE ? NORTH : SOUTH;
        Square dst      = src + dir;
        Rank prom       = Rank((7 + side) & 7);
        Rank push       = Rank(2 + side * 3);
        
        if (!get(both, dst)) {

            if (rank(dst) != prom) {
                // Quiet
                save_f(src, dst, QUIET);
                // Double push
                if (rank(dst) == push && !get(both, dst + dir))
                    save_f(src, dst + dir, PUSH);
            } else {
                // Promotions
                for (auto type : PROMOTIONS)
                    save_f(src, dst, type);
            }
        }
        // Captures
        Bitboard attacked = P_ATTACKS[side][src] & pieces[!side];

        for (auto dst : BitIter(attacked)) {
            if (rank(dst) == prom) {
                // Promotions with capture
                for (auto type : PROMOTIONS)
                    save_f(src, dst, type | CAPTURE);
            } else {
                save(src, dst);
            }
        }
        // enPassant
        if (enpass()) {
            File enps_f     = File(enpass());
            Bitboard enps   = file_bb(enps_f) & P_ATTACKS[side][src];

            if (enps)
                save_f(src, Square(lsb(enps)), ENPASS);
        }
    }
    // Knight
    for (auto src : BitIter(attacker & knights())) {

        Bitboard attacked = N_ATTACKS[src]       & ~attacker;

        for (auto dst : BitIter(attacked))
            save(src, dst);
    }
    // Bishop / queen
    for (auto src : BitIter(attacker & bishops)) {

        Bitboard attacked = B_ATTACKS[src][both] & ~attacker;

        for (auto dst : BitIter(attacked))
            save(src, dst);
    }
    // Rook / queen
    for (auto src : BitIter(attacker & rooks)) {

        Bitboard attacked = R_ATTACKS[src][both] & ~attacker;

        for (auto dst : BitIter(attacked))
            save(src, dst);
    }
    // King
    // TODO

    return list;
}
