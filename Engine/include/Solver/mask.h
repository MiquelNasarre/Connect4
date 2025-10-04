#pragma once

// In this representation, the board is a 8x8 grid (64 cells).
// The map bit index is calculated as: index = column * 8 + row.
/*
  NW          (<< 8)   < N >   (>> 8)         NE            BIT MASK               ADDITIVE             SUBTRACTIVE

        8   16   24   32   40   48   56   64           0x80 >> 2 * column     0xFF >> 2 * column     0x80 >> 2 * column

        7   15   23   31   39   47   55   63           0x40 >> 2 * column     0x7F >> 2 * column     0xC0 >> 2 * column
(<< 1)
        6   14   22   30   38   46   54   62           0x20 >> 2 * column     0x3F >> 2 * column     0xE0 >> 2 * column

  ^     5   13   21   29   37   45   53   61    ^      0x10 >> 2 * column     0x1F >> 2 * column     0xF0 >> 2 * column
  W                                             E                  
  v     4   12   20   28   36   44   52   60    v      0x08 >> 2 * column     0x0F >> 2 * column     0xF8 >> 2 * column

        3   11   19   27   35   43   51   59           0x04 >> 2 * column     0x07 >> 2 * column     0xFC >> 2 * column
(>> 1)
        2   10   18   26   34   42   50   58           0x02 >> 2 * column     0x03 >> 2 * column     0xFE >> 2 * column

        1    9   17   25   33   41   49   57           0x01 >> 2 * column     0x01 >> 2 * column     0xFF >> 2 * column

  SW          (>> 8)   < S >   (<< 8)         SE

*/

#pragma region // Shift pieces in a bitmap in any cardinal direction.

#define SHIFT_EAST(M)   ( (M) << 8 )
#define SHIFT_WEST(M)   ( (M) >> 8 )
#define SHIFT_NORTH(M)  ( (M) << 1 )
#define SHIFT_SOUTH(M)  ( (M) >> 1 )

#define SHIFT_NW(M)     ( (M) >> 7 )
#define SHIFT_NE(M)     ( (M) << 9 )
#define SHIFT_SW(M)     ( (M) >> 9 )
#define SHIFT_SE(M)     ( (M) << 7 )

#pragma endregion

#pragma region // Shift pieces in a bitmap in any cardinal direction twice.

#define SHIFT_2_EAST(M)   ( (M) << 16 )
#define SHIFT_2_WEST(M)   ( (M) >> 16 )
#define SHIFT_2_NORTH(M)  ( (M) <<  2 )
#define SHIFT_2_SOUTH(M)  ( (M) >>  2 )

#define SHIFT_2_NW(M)     ( (M) >> 14 )
#define SHIFT_2_NE(M)     ( (M) << 18 )
#define SHIFT_2_SW(M)     ( (M) >> 18 )
#define SHIFT_2_SE(M)     ( (M) << 14 )

#pragma endregion

#pragma region // Shift pieces in a bitmap in any cardinal direction three times.

#define SHIFT_3_EAST(M)   ( (M) << 24 )
#define SHIFT_3_WEST(M)   ( (M) >> 24 )
#define SHIFT_3_NORTH(M)  ( (M) <<  3 )
#define SHIFT_3_SOUTH(M)  ( (M) >>  3 )

#define SHIFT_3_NW(M)     ( (M) >> 21 )
#define SHIFT_3_NE(M)     ( (M) << 27 )
#define SHIFT_3_SW(M)     ( (M) >> 27 )
#define SHIFT_3_SE(M)     ( (M) << 21 )

#pragma endregion

#pragma region // Masks for any given column.

inline constexpr uint64_t Column0 = 0x00000000000000FFULL; // Mask for the column 0
inline constexpr uint64_t Column1 = 0x000000000000FF00ULL; // Mask for the column 1
inline constexpr uint64_t Column2 = 0x0000000000FF0000ULL; // Mask for the column 2
inline constexpr uint64_t Column3 = 0x00000000FF000000ULL; // Mask for the column 3
inline constexpr uint64_t Column4 = 0x000000FF00000000ULL; // Mask for the column 4
inline constexpr uint64_t Column5 = 0x0000FF0000000000ULL; // Mask for the column 5
inline constexpr uint64_t Column6 = 0x00FF000000000000ULL; // Mask for the column 6
inline constexpr uint64_t Column7 = 0xFF00000000000000ULL; // Mask for the column 7

#define COL_MASK(c) (Column0 << ((c) * 8)) // Macro to get the mask for a given column

#pragma endregion

#pragma region // Masks for any given row.

inline constexpr uint64_t Row0 = 0x0101010101010101ULL; // Mask for the row 0
inline constexpr uint64_t Row1 = 0x0202020202020202ULL; // Mask for the row 1
inline constexpr uint64_t Row2 = 0x0404040404040404ULL; // Mask for the row 2
inline constexpr uint64_t Row3 = 0x0808080808080808ULL; // Mask for the row 3
inline constexpr uint64_t Row4 = 0x1010101010101010ULL; // Mask for the row 4
inline constexpr uint64_t Row5 = 0x2020202020202020ULL; // Mask for the row 5
inline constexpr uint64_t Row6 = 0x4040404040404040ULL; // Mask for the row 6
inline constexpr uint64_t Row7 = 0x8080808080808080ULL; // Mask for the row 7

#define ROW_MASK(r) (Row0 << (r)) // Macro to get the mask for a given row

#pragma endregion

#pragma region // Masks for corners. To clear invalid shitings.

inline constexpr uint64_t MASK_1NW = 0x00FEFEFEFEFEFEFEULL;
inline constexpr uint64_t MASK_2NW = 0x0000FCFCFCFCFCFCULL;
inline constexpr uint64_t MASK_3NW = 0x000000F8F8F8F8F8ULL;

inline constexpr uint64_t MASK_1SW = 0x007F7F7F7F7F7F7FULL;
inline constexpr uint64_t MASK_2SW = 0x00003F3F3F3F3F3FULL;
inline constexpr uint64_t MASK_3SW = 0x0000001F1F1F1F1FULL; // To check win in diagonals collapsed SW

inline constexpr uint64_t MASK_1NE = 0xFEFEFEFEFEFEFE00ULL;
inline constexpr uint64_t MASK_2NE = 0xFCFCFCFCFCFC0000ULL;
inline constexpr uint64_t MASK_3NE = 0xF8F8F8F8F8000000ULL;

inline constexpr uint64_t MASK_1SE = 0x7F7F7F7F7F7F7F00ULL;
inline constexpr uint64_t MASK_2SE = 0x3F3F3F3F3F3F0000ULL;
inline constexpr uint64_t MASK_3SE = 0x1F1F1F1F1F000000ULL; // To check win in diagonals collapsed SE

inline constexpr uint64_t MASK_1W = 0x00FFFFFFFFFFFFFFULL;
inline constexpr uint64_t MASK_2W = 0x0000FFFFFFFFFFFFULL;
inline constexpr uint64_t MASK_3W = 0x000000FFFFFFFFFFULL;

inline constexpr uint64_t MASK_1E = 0xFFFFFFFFFFFFFF00ULL;
inline constexpr uint64_t MASK_2E = 0xFFFFFFFFFFFF0000ULL;
inline constexpr uint64_t MASK_3E = 0xFFFFFFFFFF000000ULL; // To check win in horozontal collapsed E

inline constexpr uint64_t MASK_1N = 0xFEFEFEFEFEFEFEFEULL;
inline constexpr uint64_t MASK_2N = 0xFCFCFCFCFCFCFCFCULL;
inline constexpr uint64_t MASK_3N = 0xF8F8F8F8F8F8F8F8ULL;

inline constexpr uint64_t MASK_1S = 0x7F7F7F7F7F7F7F7FULL;
inline constexpr uint64_t MASK_2S = 0x3F3F3F3F3F3F3F3FULL;
inline constexpr uint64_t MASK_3S = 0x1F1F1F1F1F1F1F1FULL; // To check win in vertical collapsed S

#pragma endregion

#pragma region // Masks to find winning holes.

#define COLLAPSE_HORIZONTAL_3L(M)        (SHIFT_3_EAST(M) & SHIFT_2_EAST(M) &   SHIFT_EAST(M) & MASK_3E)
#define COLLAPSE_HORIZONTAL_2L1R(M)      (SHIFT_2_EAST(M) &   SHIFT_EAST(M) &   SHIFT_WEST(M) & MASK_2E & MASK_1W)
#define COLLAPSE_HORIZONTAL_1L2R(M)      (  SHIFT_EAST(M) &   SHIFT_WEST(M) & SHIFT_2_WEST(M) & MASK_1E & MASK_2W)
#define COLLAPSE_HORIZONTAL_3R(M)        (  SHIFT_WEST(M) & SHIFT_2_WEST(M) & SHIFT_3_WEST(M) & MASK_3W)

#define TOTAL_COLLAPSE_HORIZONTAL(M)     (COLLAPSE_HORIZONTAL_3L(M) | COLLAPSE_HORIZONTAL_2L1R(M) | COLLAPSE_HORIZONTAL_1L2R(M) | COLLAPSE_HORIZONTAL_3R(M))

#define COLLAPSE_DIAGONALSW_3L(M)        (SHIFT_3_NE(M) & SHIFT_2_NE(M) &   SHIFT_NE(M) & MASK_3NE)
#define COLLAPSE_DIAGONALSW_2L1R(M)      (SHIFT_2_NE(M) &   SHIFT_NE(M) &   SHIFT_SW(M) & MASK_2NE & MASK_1SW)
#define COLLAPSE_DIAGONALSW_1L2R(M)      (  SHIFT_NE(M) &   SHIFT_SW(M) & SHIFT_2_SW(M) & MASK_1NE & MASK_2SW)
#define COLLAPSE_DIAGONALSW_3R(M)        (  SHIFT_SW(M) & SHIFT_2_SW(M) & SHIFT_3_SW(M) & MASK_3SW)

#define TOTAL_COLLAPSE_DIAGONALSW(M)     (COLLAPSE_DIAGONALSW_3L(M) | COLLAPSE_DIAGONALSW_2L1R(M) | COLLAPSE_DIAGONALSW_1L2R(M) | COLLAPSE_DIAGONALSW_3R(M))

#define COLLAPSE_DIAGONALNW_3L(M)        (SHIFT_3_SE(M) & SHIFT_2_SE(M) &   SHIFT_SE(M) & MASK_3SE)
#define COLLAPSE_DIAGONALNW_2L1R(M)      (SHIFT_2_SE(M) &   SHIFT_SE(M) &   SHIFT_NW(M) & MASK_2SE & MASK_1NW)
#define COLLAPSE_DIAGONALNW_1L2R(M)      (  SHIFT_SE(M) &   SHIFT_NW(M) & SHIFT_2_NW(M) & MASK_1SE & MASK_2NW)
#define COLLAPSE_DIAGONALNW_3R(M)        (  SHIFT_NW(M) & SHIFT_2_NW(M) & SHIFT_3_NW(M) & MASK_3NW)

#define TOTAL_COLLAPSE_DIAGONALNW(M)     (COLLAPSE_DIAGONALNW_3L(M) | COLLAPSE_DIAGONALNW_2L1R(M) | COLLAPSE_DIAGONALNW_1L2R(M) | COLLAPSE_DIAGONALNW_3R(M))

#define COLLAPSE_ALL_WINNING_MOVES(M)    (TOTAL_COLLAPSE_DIAGONALNW(M) | TOTAL_COLLAPSE_DIAGONALSW(M) | TOTAL_COLLAPSE_HORIZONTAL(M))

#pragma endregion

#pragma region // Mask for filling columns.

#define FILL_UP_SPACES(M)  ((SHIFT_NORTH(M) & MASK_1N)           | (SHIFT_2_NORTH(M) & MASK_2N)         | (SHIFT_3_NORTH(M) & MASK_3N) | \
                            (((M) << 4) & 0xF0F0F0F0F0F0F0F0ULL) | (((M) << 5) & 0xE0E0E0E0E0E0E0E0ULL) | (((M) << 6) & 0xC0C0C0C0C0C0C0C0ULL))

#pragma endregion