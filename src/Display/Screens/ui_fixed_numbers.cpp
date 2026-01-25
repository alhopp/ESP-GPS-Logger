#include "Fonts.h"
#include "Display/E_paper.h"

// ============================================================================
// Fixed-width numeric renderer
//
// Goal:
// - Render proportional Body font numbers as if they were monospaced
// - All values formatted as "000.00"
// - Digits aligned vertically across rows
// - Decimal point rendered in Mono font for visual stability
//
// Approach:
// - Precompute a fixed cell width for digits (widest Body glyph)
// - Precompute a tighter cell width for decimal (Mono glyph)
// - Render right→left, centering each glyph inside its cell
// ============================================================================

// Cached layout metrics (computed once)
static int  DIGIT_W     = 0;   // widest Body digit xAdvance
static int  DOT_W       = 0;   // Mono decimal xAdvance (tightened)
static bool DIGITS_INIT = false;


// -----------------------------------------------------------------------------
// Measure glyph metrics once at runtime
// -----------------------------------------------------------------------------
static void initDigitCells()
{
  const GFXfont *body = Fonts::Body12, *mono = Fonts::Mono12;

  // Widest numeric glyph in Body font → fixed digit cell
  for(char c='0'; c<='9'; ++c){
    if(c < body->first || c > body->last) continue;
    DIGIT_W = max(DIGIT_W, (int)body->glyph[c - body->first].xAdvance);
  }

  // Decimal point from Mono font (optically tighter)
  if('.' >= mono->first && '.' <= mono->last){
    DOT_W = mono->glyph['.' - mono->first].xAdvance - 4;
    if(DOT_W < 4) DOT_W = 4; // safety clamp
  }

  DIGITS_INIT = true;
}


// -----------------------------------------------------------------------------
// Draw fixed-width number (right-aligned)
// -----------------------------------------------------------------------------
void drawFixedNumber(int xRight, int yBaseline, float value)
{
  if(!DIGITS_INIT) initDigitCells();

  // Fixed format: "000.00"
  char buf[8];
  dtostrf(value, 6, 2, buf);

  int x = xRight;

  // Render from right → left so alignment is trivial
  for(int i = strlen(buf)-1; i >= 0; --i){

    const char c = buf[i];
    const bool dot = (c == '.');

    const int cellW = dot ? DOT_W : DIGIT_W;
    const GFXfont *font = dot ? Fonts::Mono12 : Fonts::Body12;

    x -= cellW;                 // advance to this cell
    display.setFont(font);

    if(c < font->first || c > font->last) continue;
    const GFXglyph &g = font->glyph[c - font->first];

    // Center glyph within its fixed cell
    display.setCursor(x + (cellW - g.xAdvance)/2, yBaseline);
    display.write(c);
  }
}
