#pragma once
// ---------------------------------------------------------------------------
// Player (hero) pixel art. All heroes are drawn FACING RIGHT.
// Each frame of one character must have the SAME width (the renderer centres
// a frame on its own width, so different widths make the body jump).
// Only included by Sprites.cpp.
// ---------------------------------------------------------------------------
#include "../Renderer.h"

// Used by DEFENSE and the COIN
static const Palette kHeroPalette = {
    { 'R', 0.73f, 0.24f, 0.20f },  // armour red
    { 'r', 0.40f, 0.18f, 0.17f },  // dark red
    { 'd', 0.26f, 0.11f, 0.11f },  // deepest red
    { 'Y', 0.82f, 0.51f, 0.29f },  // gold
    { 'y', 0.54f, 0.32f, 0.24f },  // dark gold
    { 'O', 0.96f, 0.74f, 0.42f },  // bright gold
    { 'C', 0.24f, 0.79f, 0.87f },  // energy glow (coin: cyan ring segments)
    { 'c', 0.02f, 0.05f, 0.08f },  // glow shadow
    { 'W', 0.93f, 0.97f, 0.92f },  // white glint (coin: core glow)
    { 'K', 0.07f, 0.04f, 0.06f },  // outline
    { 'G', 0.55f, 0.58f, 0.62f },  // steel
    { 'g', 0.30f, 0.32f, 0.36f },  // dark steel
    { 'P', 0.42f, 0.32f, 0.75f },  // coin: purple rim
    { 'D', 0.05f, 0.22f, 0.45f },  // coin: dark blue ring gaps
    { 'L', 0.55f, 0.90f, 0.95f },  // coin: light cyan
    { 'B', 0.70f, 0.95f, 1.00f },  // coin: pale blue triangle edge
};

// =========================== MELEE (Iron Man) ===============================
static const Palette kMeleePalette = {
    { 'K', 0.10f, 0.07f, 0.11f },  // outline
    { 'k', 0.23f, 0.09f, 0.13f },  // dark red
    { 'd', 0.34f, 0.12f, 0.16f },  // dark red
    { 'r', 0.08f, 0.24f, 0.38f },  // outline
    { 'R', 0.45f, 0.14f, 0.15f },  // red shade
    { 'Y', 0.58f, 0.19f, 0.17f },  // red shade
    { 'y', 0.42f, 0.29f, 0.30f },  // red shade
    { 'O', 0.73f, 0.29f, 0.19f },  // armour red
    { 'o', 0.14f, 0.47f, 0.77f },  // repulsor glow
    { 'C', 0.89f, 0.31f, 0.22f },  // armour red
    { 'c', 0.47f, 0.55f, 0.58f },  // blue shade
    { 'W', 0.85f, 0.54f, 0.30f },  // gold
    { 'w', 0.34f, 0.81f, 0.94f },  // repulsor glow
    { 'B', 0.97f, 0.81f, 0.37f },  // gold highlight
    { 'b', 0.83f, 0.79f, 0.60f },  // gold highlight
    { 'P', 0.84f, 0.97f, 0.98f },  // repulsor glow
};

// --- MELEE frame A: normal stance (idle / walking) - padded to the same width as B ---
static const char* kMeleeA[] = {
    "........................kdyyyyyyydk...............................",
    ".....................dcbbbbWCWWbbbbWyK............................",
    "...................yWbWWCCCCCCCOWWWWbbyK..........................",
    ".................kbbWCCCCCCCCCCCYCCCCWbbKr........................",
    ".................bWOOCCCCCCCCCCCOYCCCCCWbd........................",
    "................cyRRYYOCCCCCCCCCWWYCCCCCWWK.......................",
    "...............yydRRYYYYOCOWbbbbbPWYCCCCCCOyr.....................",
    "..............KcddRYYYYYOYWBBBBBBbbYCCCCCCCbK.....................",
    ".............KyydRYYYYYYOYWWBBBBBBbWOCCCCCCWyK....................",
    ".............rcddRYYYYYYYOWWBBBBBBBbYOOOCCCCbkr...................",
    ".............yydRYYYYYYYYOCWBBBBBBBbOYOOOOOObyK...................",
    "............KyydRYYYYYYYYOCWWBBBBBBBbYCCCOOOWbKo..................",
    "............kyddRYYYYYYYYYCCWBBBBBBBbWOOOOOOWBkr..................",
    "............yddRYRRRYYYYYYOCWWBBBBBBBBBBBBBBBByr..................",
    "............dddYOOOYdYYYYYOCWWBBBBBBBBBBBBBBBBOr..................",
    "............kkddRYYCYdYYYYOWWWBBBBBBBBBBBBBBBBWKo.................",
    "...........KdddddYOOCRRYYYOWWWBBBBBBBBBBBBBBBBWKo.................",
    "...........kdddddRCOCYRYYROWWWBBBBBBBBBBBBBBBBWKo.................",
    "...........kdddddROOCYdYYYWWccbbBBBBBBBBBBBBBBWKo.................",
    "...........dRddddRYYCYRYYOWcwwwwbbbBBBBBBBBBBBWKo.................",
    "...........dYRdddRYYOdRYYWBwwPPPwwwcBBBBBBBBBbyro.................",
    "...........dYYRddRRYOkRYYWBwwPPPPPwwcbBBBBBbwworo.................",
    "...........kYYRRddRYYkYYYWBwwPPPPPPPwcccccowwwro..................",
    "..........KKRYYRRdRORdYYYOWbwPPPPPPPwbbbbwwwwwro..................",
    ".......dCOdKKRYRRdRYkRYYYYObwwwwwwPwwbBBBbwwworo..................",
    "......ROCOYRKKdRRddkdRYYYYOWWbbbbbbbbBBBBBcwwro...................",
    ".....dOCCOOYRKKKKKKdRRRRYYYWWWWBBBBBBBBBBBBbcro...................",
    "....kOCBBCCOOkKKKKKdRRRRRYYYWWWBBBBBBBBBBBBbyro...o...............",
    "....OCBBBBOCOYKkdkKKRRRRRYYYWOWBWBBBBBWBBBBbKr..roPor.............",
    "...RCWBBBBBWOOddRRdKkRRRRRYYOOWWBBBBBWWWBBBykKKcoowowo............",
    "...OCBBBBBBBWOYddRRkKkRRRRRYYOWBBBBBWWBBWBckWYOcowwwww............",
    "..ROOOWBBbbbBWWYddddkKKdRRRRYOWWBBBWOOOWWBkyOOcowwwwww............",
    "..YYOWOWBBBWOWWYkddRdkKKkdRddYOOOOOYYYYYOydyRyowwwwwww............",
    ".kYWBBBWOBWOOOYddYYRRRkKKKkddRYYYYYYYYOYRdWRdrowwPPPww............",
    ".dYWBBBWOORkkkkddWWYRRRdkKKKkdRRYYYYYYYdkOCRdrowwPPPww............",
    "KYOOWBWOCOdKKKkdyWWYYYRRRdddkKKkkkddddkkROCdycowPwPPww............",
    "kOOWWCOOCCCdkkkdyyYYYYYYYyyyOYdKKKKKKKkRYOYkkkooPPwwwo............",
    "KOOYYOOOCCCCkKkdddRYYYYYYcwwcOOOKKKddRRRRRdkkdroowwwoo............",
    ".dYddYOOOCCCYkkkkkkdRYYYywwwwcOWkrrkkdRRRRRdkkyooooooo............",
    ".KddddYOOOCOdyykkkkkRYYYywwwwcOOro..rKkkkdddkkKrrrrooo............",
    "..KddddYOOYycbBBWkkRYYYYyowwoyyKr......rrrrrrrroorrooo............",
    "..rKddddYRkbPcbBBRkRYYYYRyocyykro.................................",
    "...rKkdddkcPwwbBBRKkdRRRRYyyykrr..................................",
    ".....KKkKKcwwocWWddRRddRRYYdkWWyr.................................",
    "......rKrKrwwcbWykYOOOYRRRkkcBBBkr................................",
    "........ckKrrydkKKYOOOYRkKkyOWWWYr................................",
    ".......KKKKKKROOdKKdRdkKKKdyOOOOYk................................",
    ".....kddkkKKYWWWBkKKKKKKKdRRYYRRORr...............................",
    ".....ddddkdYOOOWWyo.....KddRRRYOORK...............................",
    ".....ddddRYCBYdOWk.......dddRYOOOdr...............................",
    "....ddddRYYYOOYYkr.......KkkkROOYKo...............................",
    "...KkkdRYYYYYYRkrr.......KKKKkYYdKr...............................",
    "..kddkkRYYRdkKKrr.........KkkdRYRyk...............................",
    "..ddddRRYRkKrrr..........KddkRROWWOkr.............................",
    "..kkkdOCCCRKr............KdkdRYYWOYRr.............................",
    "..KkkdYCCCCkr............KKKkkdRYRRkr.............................",
    "..KKkkRYYRRKr..............rrrKKKKKrr.............................",
    "...rKKKKKKKrr.....................................................",
};

// --- MELEE frame B: arm shoots out of the body (shown on every punch) ---
static const char* kMeleeB[] = {
    "........................kdyyyyyyydk...............................",
    ".....................dcbbbbWCWWbbbbWyK............................",
    "...................yWbWWCCCCCCCOWWWWbbyK..........................",
    ".................kbbWCCCCCCCCCCCYCCCCWbbKr........................",
    ".................bWOOCCCCCCCCCCCOYCCCCCWbd........................",
    "................cyRRYYOCCCCCCCCCWWYCCCCCWWK.......................",
    "...............yydRRYYYYOCOWbbbbbPWYCCCCCCOyr.....................",
    "..............KcddRYYYYYOYWBBBBBBbbYCCCCCCCbK.....................",
    ".............KyydRYYYYYYOYWWBBBBBBbWOCCCCCCWyK....................",
    ".............rcddRYYYYYYYOWWBBBBBBBbYOOOCCCCbkr...................",
    ".............yydRYYYYYYYYOCWBBBBBBBbOYOOOOOObyK...................",
    "............KyydRYYYYYYYYOCWWBBBBBBBbYCCCOOOWbKo..................",
    "............kyddRYYYYYYYYYCCWBBBBBBBbWOOOOOOWBkr..................",
    "............yddRYRRRYYYYYYOCWWBBBBBBBBBBBBBBBByr..................",
    "............dddYOOOYdYYYYYOCWWBBBBBBBBBBBBBBBBOr..................",
    "............kkddRYYCYdYYYYOWWWBBBBBBBBBBBBBBBBWKo.................",
    "...........KdddddYOOCRRYYYOWWWBBBBBBBBBBBBBBBBWKo.................",
    "...........kdddddRCOCYRYYROWWWBBBBBBBBBBBBBBBBWKo.................",
    "...........kdddddROOCYdYYYWWccbbBBBBBBBBBBBBBBWKo.................",
    "...........dRddddRYYCYRYYOWcwwwwbbbBBBBBBBBBBBWKo.................",
    "...........dYRdddRYYOdRYYWBwwPPPwwwcBBBBBBBBBbyro.................",
    "...........dYYRddRRYOkRYYWBwwPPPPPwwcbBBBBBbwworo.................",
    "...........kYYRRddRYYkYYYWBwwPPPPPPPwcccccowwwro..................",
    "..........KKRYYRRdRORdYYYOWbwPPPPPPPwbbbbwwwwwro..................",
    ".......dCOdKKRYRRdRYkRYYYYObwwwwwwPwwbBBBbwwworo..................",
    "......ROCOYRKKdRRddkdRYYYYOWWbbbbbbbbBBBBBcwwro...................",
    ".....dOCCOOYRKKKKKKdRRRRYYYWWWWBBBBBBBBBBBBbcro...................",
    "....kOCBBCCOOkKKKKKdRRRRRYYYWWWBBBBBBBBBBBBbyr............o...o...",
    "....OCBBBBOCOYKkdkKKRRRRRYYYWOWBWBBBBBWBBBBbKr..............roPor.",
    "...RCWBBBBBWOOddRRdKkRRRRRYYOOWWBBBBBWWWBBBykK............Kcoowowo",
    "...OCBBBBBBBWOYddRRkKkRRRRRYYOWBBBBBWWBBWBckWKKKKKKKKKKKKKOcowwwww",
    "..ROOOWBBbbbBWWYddddkKKdRRRRYOWWBBBWOOOWWBkyyWWWWWWyWWWWWWcowwwwww",
    "..YYOWOWBBBWOWWYkddRdkKKkdRddYOOOOOYYYYYOydyyBBBBBByBBBBBBowwwwwww",
    ".kYWBBBWOBWOOOYddYYRRRkKKKkddRYYYYYYYYOYRdWRRWWWWWWyWWWWWWowwPPPww",
    ".dYWBBBWOORkkkkddWWYRRRdkKKKkdRRYYYYYYYdkOCRRCCCCCCkCCCCCCowwPPPww",
    "KYOOWBWOCOdKKKkdyWWYYYRRRdddkKKkkkddddkkROCddOOOOOOkOOOOOOowPwPPww",
    "kOOWWCOOCCCdkkkdyyYYYYYYYyyyOYdKKKKKKKkRYOYkkOOOOOOkOOOOOOooPPwwwo",
    "KOOYYOOOCCCCkKkdddRYYYYYYcwwcOOOKKKddRRRRRdkkYYYYYYkYYYYYYroowwwoo",
    ".dYddYOOOCCCYkkkkkkdRYYYywwwwcOWkrrkkdRRRRRddRRRRRRkRRRRRRyooooooo",
    ".KddddYOOOCOdyykkkkkRYYYywwwwcOOro..rKkkkddddddddddkddddddKrrrrooo",
    "..KddddYOOYycbBBWkkRYYYYyowwoyyKr......rrrrrrKKKKKKKKKKKKKroorrooo",
    "..rKddddYRkbPcbBBRkRYYYYRyocyykro...........K.....................",
    "...rKkdddkcPwwbBBRKkdRRRRYyyykrr..................................",
    ".....KKkKKcwwocWWddRRddRRYYdkWWyr.................................",
    "......rKrKrwwcbWykYOOOYRRRkkcBBBkr................................",
    "........ckKrrydkKKYOOOYRkKkyOWWWYr................................",
    ".......KKKKKKROOdKKdRdkKKKdyOOOOYk................................",
    ".....kddkkKKYWWWBkKKKKKKKdRRYYRRORr...............................",
    ".....ddddkdYOOOWWyo.....KddRRRYOORK...............................",
    ".....ddddRYCBYdOWk.......dddRYOOOdr...............................",
    "....ddddRYYYOOYYkr.......KkkkROOYKo...............................",
    "...KkkdRYYYYYYRkrr.......KKKKkYYdKr...............................",
    "..kddkkRYYRdkKKrr.........KkkdRYRyk...............................",
    "..ddddRRYRkKrrr..........KddkRROWWOkr.............................",
    "..kkkdOCCCRKr............KdkdRYYWOYRr.............................",
    "..KkkdYCCCCkr............KKKkkdRYRRkr.............................",
    "..KKkkRYYRRKr..............rrrKKKKKrr.............................",
    "...rKKKKKKKrr.....................................................",
};

// =========================== RANGED (missile launcher) ======================
static const Palette kLauncherPalette = {
    { 'K', 0.04f, 0.03f, 0.04f },  // outline
    { 'k', 0.09f, 0.09f, 0.12f },  // near-black
    { 'd', 0.25f, 0.12f, 0.12f },  // deep red
    { 'N', 0.12f, 0.18f, 0.23f },  // navy steel
    { 'r', 0.40f, 0.18f, 0.14f },  // dark red
    { 'n', 0.21f, 0.27f, 0.31f },  // slate
    { 'b', 0.54f, 0.26f, 0.17f },  // rust
    { 'G', 0.38f, 0.37f, 0.36f },  // grey steel
    { 'R', 0.76f, 0.26f, 0.17f },  // red
    { 'y', 0.62f, 0.46f, 0.32f },  // bronze
    { 'C', 0.26f, 0.60f, 0.71f },  // glow
    { 'Y', 0.81f, 0.61f, 0.37f },  // gold
    { 'W', 0.85f, 0.84f, 0.76f },  // highlight
};

static const char* kLauncher[] = {
    "......................KkkN.....KkkN.",
    "..................GkRRRRkn.KKbRRRRrK",
    "...............kryWdRRbdKKGWrRRRRkKn",
    "..............dYyNWkkddKyWWWrRbrKdK.",
    "............KyynnkNdKKbybYWWrbrddK..",
    "...........dydnGYKKkyWWWYRYYYddrdN..",
    "..........krNnGYrRdWWWYRRbbGCGnkk...",
    ".........NnnCWrybRbWWbRRRrrnCnKn....",
    ".......knnCWGbRbRbRWYyydrrnGkN......",
    "......KnYWYdRRRRRrRdGGGnnnNKG.......",
    ".....KYWybybRRRbrdddNGGGnKKKkddddK..",
    "......yyyybbbbrddddrkGnKKnKbrbbdKk..",
    "....KyybrrbrrddddrddkKKnWyRdddddK...",
    "....KYGbYRRdddddddkKKdNWWYbdkkdKn...",
    "...NGyrRRRbrdddkKKdddnWWynkdddkN....",
    "...nKGrrRbbrddkNNbrRdCGnnnNddKN.....",
    "...NNnrrrrrddNnnNyrddknnnnnkN.......",
    "...KNkrrdNNNnnnnndrkkkknnkk.........",
    "...KNNNNNNnnnnnNNNNNkkkKN...........",
    "....kNKnNNNNkkNNNNNNNKk.............",
    "....kNKkkddrbdkNNkkKKkN.............",
    ".....kKKddrbbrKKKkKKkbK.............",
    ".......KGybdbrKKKkKNddbn............",
    ".......krYbrbrKCCCCkdkyk............",
    ".......brrGrbrKCCCCNddrd............",
    "......dRrbbrRrGKCCWCKNdyK...........",
    ".....KkdkdkkrddKKKKKKKkrrk..........",
    "....GyyGnnnnGGGGYYYyyGGGGnkn........",
    "...KGnnkkNNNNNNnnnnnnnNnNNnk........",
    "...KGNnkkNNNkNnnnyNnnnNNNkGK........",
    "...KKkkkkNNNNNNnnnNNnNkkkkkKG.......",
    "..YYYYbbbbbbbbbbrryyyyyyyyRRrk......",
    ".kbryrrrrrrrrrbbbbYYYYYYyrdnrK......",
    ".rbbrrrdrrrrrdrrbbyrbbbbbyGGNrN.....",
    "KybbrrknNNNNNNrrrbbRYYYYyYGGnrK.....",
    "KkkdddknnnNNNNrrrbbyYYYYYYdnNdK.....",
    "nnNNNNkkkkkkkKkkkkkdddddNdNNNNn.....",
};

// =========================== DEFENSE ========================================
// (your 76-row Iron Man art, uses kHeroPalette)
static const char* kDefense[] = {
    "............................................................................",
    "............................................................................",
    "............................................................................",
    "............................................................................",
    "............................................................................",
    "............................................................................",
    "............................................................................",
    ".............................cKKKKKc........................................",
    "..........................cKdyOWWWOdc.......................................",
    ".........................cKOOOOOYRRRK.......................................",
    ".........................KYyRRRRRRRRRK......................................",
    ".........................KYrRRRRRRRRdKK.....................................",
    ".........................KyRRRRddRRRKcc.....................................",
    ".........................KrrRRdKKKKKKKKc.....cc.............................",
    "........................KddKKKKKryYydKYydccKryyyrdK.........................",
    ".....................KrYOYRRyKROOOOOOYROOYdKdRRRYOOYdc......................",
    "...................KYWORRRRRKRRRRYYYYYYRRYYRKKdRRRRROYKc....................",
    "..................dOYRRRRydKRRRRRdRYOOOORRRRYdKKdRRRRRYdc.......ccc.........",
    "................KrORRRRddKKdRRRRRdRRYOOOORRRROKdKdRRRRRYKcccKKdryyydK.......",
    "................dWyyRRRRYYYddRRRdRRRYYOOOYRRRRYdORKdRRRRKyYOWWWWOOOOYKc.....",
    "...............KKdddddKRRRRYYdRddRRRRRYOOOYyRYOdRORdRRRKOOWWWOOOYYYYYyc.....",
    "..............KyOOYYYRRKdRRRROrKRRRRRRYOOOOOOOOydROKddKOYOWOYOOYYYYyYOKc....",
    "............KKWORRRRRRRRRKdRRRWrdRRRRRYOOOOOOOOOKdRYKKyOYWOYYydddddrROrc....",
    "...........KdWRRRRRRRRRRRRdKRRROdRRRRyKKYOOOOOOOKKROKdOYOOOyKyyyyyyyyYYc....",
    "..........KdWRRRRRRRRRRRRRRdKRRROKRRRRrWYKyOOOydKKRYKOOOOYYdyyyyyyyrYROK....",
    "..........KORRRRRRRRRRYYYYYYKRRRYydRRycWWWKKdKdWKKRyKyYOOORyyyyyyyyyydOKC...",
    ".........KyRRRRRRRRRYYYYOOOYRKRRRYKRRYOYYryOOOyrKKdRKyyYOYYyyyyyyyyyydYdc...",
    ".........dYRRRRRRRRRROOOYRRRRKRRRYKRRRROOOOOOOOYKKdRKyyYOdyyyyyyyyyyyrYdc...",
    "........KORRRRRRRRRRRRRRRRRRRKdRRRdrRRRROOOOOOYYKKKddyyYORyyyyyyyyyyyrYdc...",
    "........KYRRRRRRRRRRRRRRRRRRRKKRRRRKrRRROOOYYYYyKKKKdyyYORyyyyyyyyyyyrYdc...",
    ".......ccdKKKdRRRRRRRRRRRRyOOdKRRRRKKdRRYORRydYdKKdddyyYOdyyyyyyyyyyyrYdc...",
    ".......ccKKddddKdRRRRRRRYOYOOrKRRRRRRKKdRYRYYYdKKRRKryyYOdyyyyyyyryyyryRc...",
    "......cdyyyydKrRddRRRRRROyrYOdKRRRORRRRdKKddddKKRRdKRyyYOdyyyyyyyryyyryRc...",
    ".....KrYRRRRYYKyyrKRRRROYYrYOdKRRrYyRRRRRRRRRRRRRdRKRyyYORyyyyyyyyyyyryRc...",
    ".....rYRRRRRYYKrrrrKddOOOyYYOKKRRdydRRRRRRRRRRRRRdKKRyyYORYyyyyyyyyyyryRc...",
    "....KYRRRRRRYYdrrrrdKROOOYYOdKdRRydyRRRRRYOYyRRRRddKRyyYORYyyyyyyyyyyrYdc...",
    "...KyRRRRRRYRRdryrdyyddYOYrKKddRRRRRRRRROCCCCcRRRddKRyyYORYYyyyrYYyyyrYdc...",
    "...KRRRRRRRYRRRKdryYYyrrdKKKKddRRRRRRRROCCWWCCcRRddKdyRYORYYyyyYOcCrrrYdc...",
    "...yRRRRRRYRRRRRydyOYYyyYKKKKddRRRRRRRROCWWWWWCyRddKdyyYOdYYYyyCCCCcyrYdc...",
    "..KyRRRRRRRRRRdRYYRdOYyydKcKddRRRRRRRRYCWWWWWWCyRddKdyRYYdRRYOYCWWCCrrYKc...",
    "..KRRddddRRRRdRYRdRRKYKKKKKdddRRRRRRRRYCWWWWWWCyRddKdyRYYRRRYYCCWWCCrrYKc...",
    "..crRRddRRRRRRROOOYddKKKddddddRRRRRRRRRCCWWWWWCyRddKdyRYYRRRRyCCWWCCrrYKc...",
    "..crRdddRRRRKRRYOOOYdKKddddddKRRRRRRRRRYCWWWWCCyRddKdyyYYRRRRyCCWWWCrrYKc...",
    "..crdddRRRRRRRRYOOYOOKKddddddKdRRRRRRRRRCCCCCCyRdddKdyyYYdRRRyCCWWWCrrYKc...",
    "..KrddRRRRddYRRYOYYOOKKKKKKdRyYYrdRRRRRRRCCCCYRRddKKdyyYYRRRRyCCWWCCrrYK....",
    "..KKdRRddKdRRRRROYYOOKKKddKdrRRyYOrRRRRRRRRRRRRRddKKdyyYORRRRyCCWWCCrryK....",
    "...KcKKKdRRRRRRROOOOOKKKdKKKKKdyyYyKRRRRRRRRRRRRddKKdyyYORRRRyCCWWCcyryK....",
    "...KcrRRRRRRRRRRYRRyYKcKKddKKdKKddKdKRRRRRRRRRRddKKKdyyYYRRRyycCWCCryryc....",
    "...KcrRRRRRRRRRRKryydKccKKKKKdRRKKdddKRRRddddKRdKKc.dyyYYRRyyrrCCCCryryc....",
    "...cccdRRRRRRRRKYOOOOYrKKKdddKKdRddddddRdRRRRdKKKKC.dyyYYdyyRyyrcyrrryRc....",
    "....KccRRRRRRRKYOOOOOOOKKrddddRKKyKdKddKRRRRRdyKKK..dyyYYdRyRyrryyrrrydc....",
    "....KccrRRRRdKROOOOOYYOOKdrdyyRYRKddRRRKRRRRRdrKrdK.KyyYYdRRyRrryYrrrydc....",
    "....cKccddKKKdyYOOOYddYYKcryyyRYRdKddRRdRRRRdRKdddrKKyyYYdRyyyrrrrrrdYKc....",
    "......KdKKdRyyRYOOOyddYYKcryyyYYRRKdddRRRRRRddKrdddKKrRYYdRRyyrrrrrrdYKc....",
    "......KyyRRRyYRRYOOyKKyKKdryyRyYRRdKdddRRRRRRKrrrdKKKrrYYdRRyyrrrrrrdYKc....",
    "......KyyyyyYORRYYOYYyKKcdcrdRyRRRRKddKRRRRRKcccddRRKdrrYrrRRrrrrrrrryc.....",
    ".......KyyyyyYYYydRYYYKccdddRRRYyRRKKdKRRddKcKdrdRddKdrryyryRrrrrrrryrc.....",
    ".......cKyyyyyyrryyyYdKrccKyRRRRRRdccKKKKKKcccKKRdccKKrdyYdyRrrrrrrrYKc.....",
    "........cKryrddrryyrKKKKcKrRRRRRRYKC.cccCKKcccKdRrccKKrryYdRrrrrrrrdYKc.....",
    ".........ccKKdyrddKKKdrdddyRRRRRRRKc.....KKKKKddRKrddKrryYrrrrrrrrrdYKc.....",
    "...........ccKKKKccKddddRRRdRRRRRRKccccccKddddddRRdddKdrRYydrrrrrrryRc......",
    "..............CCCC.KrddddRRRRRRRddKc....cKdddddddRRrKcKrryYdrrrrrrdYKc......",
    "..................KcrdddRRRRRRRRRKcc....cKKdddddddKdKcKrryYrrrrrrrRyKc......",
    "..................KrrdddRRRRRRRRdKc.....cKrKdddddRRRRdKdrrYydrrrrKYdc.......",
    ".................cKrddddRRdddRRRKcc.....cdddddKdRRRRRRdKrryYdrrrdyYKcc......",
    ".................KddddddKRRRYRdKcc......cKdRRrKRRRRRRRdKdrrYyKdrYYyKcc......",
    ".................KRdddddRRRRYRRKcc.......KKKddKRRdRRRRdKKrryYyYYYYKcccc.....",
    ".................KddddKRRRRRORRRKc........cccKKddddddKKKKdrryYYYyKcccc......",
    ".................KKdRddRRRRRRRRRKc...........cccccKKcccccKrryYyKKccccc......",
    "..................cKKKdRRRRRRRRRKc.......................cKKKKKccccc........",
    "...................cccKKKKKdKKKKcc........................cccccc............",
    ".....................cccccccccccc...........................................",
    "............................................................................",
    "............................................................................",
    "............................................................................",
    "............................................................................",
};

// =========================== COLLECTIBLE: energy core coin ==================
static const char* kCoin[] = {
    "....PPPP....",
    "..PPCCCCPP..",
    ".PCLDCCDLCP.",
    ".PLBBBBBBLP.",
    "PCDBWWWWBDCP",
    "PLCDBWWBDCLP",
    "PCLCDBBDCLCP",
    "PLCDCDDCDCLP",
    ".PCLCDDCLCP.",
    ".PDCLDDLCDP.",
    "..PPCDDCPP..",
    "....PPPP....",
};
