// CA53Y Comms Display: screens, on-screen keyboard and touch handling (240x320 portrait).
// Everything is drawn into a 240x40 strip sprite and pushed band by band, so redraws don't flicker
// and we never need a full 150 KB frame buffer.
#pragma once

// ---------- theme ----------
static constexpr uint32_t COL_LAC   = 0x0F2A30;   // deep lacquer teal
static constexpr uint32_t COL_LAC2  = 0x17393F;
static constexpr uint32_t COL_LINE  = 0x2D545A;
static constexpr uint32_t COL_GOLD  = 0xCFA84E;
static constexpr uint32_t COL_DGOLD = 0xA07A2C;   // gold that reads on cream
static constexpr uint32_t COL_CHAMP = 0xEAD9A8;
static constexpr uint32_t COL_IVORY = 0xF2EBD9;
static constexpr uint32_t COL_CREAM = 0xF4EDDC;
static constexpr uint32_t COL_MIST  = 0xA3B8B2;
static constexpr uint32_t COL_VERD  = 0x86C2AE;
static constexpr uint32_t COL_ROSE  = 0xE39098;
static constexpr uint32_t COL_INK   = 0x1A2A2C;

#define F_BODY ((const lgfx::IFont *)&lgfx::fonts::FreeSans9pt7b)
#define F_SMALL ((const lgfx::IFont *)&lgfx::fonts::Font2)
#define F_TITLE ((const lgfx::IFont *)&lgfx::fonts::FreeSerif12pt7b)
#define F_SERIF9 ((const lgfx::IFont *)&lgfx::fonts::FreeSerif9pt7b)
#define F_TITLEB ((const lgfx::IFont *)&lgfx::fonts::FreeSerifBold12pt7b)
#define F_LOGO ((const lgfx::IFont *)&lgfx::fonts::FreeSerifBold24pt7b)

using lgfx::textdatum_t;
#define D_TL textdatum_t::top_left
#define D_TC textdatum_t::top_center
#define D_TR textdatum_t::top_right
#define D_ML textdatum_t::middle_left
#define D_MC textdatum_t::middle_center
#define D_MR textdatum_t::middle_right

// ---------- drawing (all coordinates are screen coordinates; OY is the current strip's top) ----------
static LGFX lcd;
static LGFX_Sprite strips[2] = {LGFX_Sprite(&lcd), LGFX_Sprite(&lcd)};
static int nStrips = 0;
static LovyanGFX *G = &lcd;
static int OY = 0, SH = 320;
static bool useStrip = false;
#define STRIP_H 40

static inline bool vis(int y, int h) { return y < OY + SH && y + h > OY; }
static void fillR(int x, int y, int w, int h, uint32_t c) { if (vis(y, h)) G->fillRect(x, y - OY, w, h, c); }
static void rfill(int x, int y, int w, int h, int r, uint32_t c) { if (vis(y, h)) G->fillRoundRect(x, y - OY, w, h, r, c); }
static void rframe(int x, int y, int w, int h, int r, uint32_t c) { if (vis(y, h)) G->drawRoundRect(x, y - OY, w, h, r, c); }
static void hline(int x, int y, int w, uint32_t c) { if (vis(y, 1)) G->drawFastHLine(x, y - OY, w, c); }
static void vline(int x, int y, int h, uint32_t c) { if (vis(y, h)) G->drawFastVLine(x, y - OY, h, c); }
static void circ(int x, int y, int r, uint32_t c) { if (vis(y - r, 2 * r + 1)) G->fillCircle(x, y - OY, r, c); }
static void ring(int x, int y, int r, uint32_t c) { if (vis(y - r, 2 * r + 1)) G->drawCircle(x, y - OY, r, c); }
static void tri(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t c) {
  int lo = min(y0, min(y1, y2)), hi = max(y0, max(y1, y2));
  if (vis(lo, hi - lo + 1)) G->fillTriangle(x0, y0 - OY, x1, y1 - OY, x2, y2 - OY, c);
}
static void txt(const char *s, int x, int y, const lgfx::IFont *f, uint32_t c, textdatum_t d) {
  G->setFont(f);
  int h = G->fontHeight();
  int v = (int)d & 0x0C;
  int top = v == 4 ? y - h / 2 : v == 8 ? y - h : y;
  if (!vis(top - 2, h + 4)) return;
  G->setTextDatum(d);
  G->setTextColor(c);
  G->drawString(s, x, y - OY);
}

// ---------- text measuring and wrapping (GFX fonts here are ASCII only) ----------
static uint8_t cwBody[128], cwSmall[128];
static void initCharWidths() {
  char s[2] = {0, 0};
  for (int c = 0; c < 128; c++) {
    s[0] = (c < 32 || c > 126) ? '?' : c;
    lcd.setFont(F_BODY);  cwBody[c] = lcd.textWidth(s);
    lcd.setFont(F_SMALL); cwSmall[c] = lcd.textWidth(s);
  }
}
static inline int charW(const lgfx::IFont *f, char c) {
  uint8_t u = (uint8_t)c & 0x7F;
  return f == F_SMALL ? cwSmall[u] : cwBody[u];
}
static int strW(const char *s, const lgfx::IFont *f) { int w = 0; for (; *s; s++) w += charW(f, *s); return w; }

// UTF-8 -> what the fonts can draw. Curly quotes become straight, anything else unknown becomes '?'.
static void asciiize(char *dst, const char *src, size_t cap, bool flatten) {
  size_t j = 0;
  const uint8_t *p = (const uint8_t *)src;
  while (*p && j < cap - 1) {
    uint8_t c = *p;
    if (c < 0x80) { dst[j++] = (c == '\n' && flatten) ? ' ' : (char)c; p++; continue; }
    int n = (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3 : (c & 0xF8) == 0xF0 ? 4 : 1;
    char r = '?';
    if (n == 3 && p[0] == 0xE2 && p[1] == 0x80) {
      if (p[2] == 0x98 || p[2] == 0x99) r = '\'';
      else if (p[2] == 0x9C || p[2] == 0x9D) r = '"';
      else if (p[2] == 0x93 || p[2] == 0x94) r = '-';
    }
    dst[j++] = r;
    for (int k = 0; k < n && *p; k++) p++;
  }
  dst[j] = 0;
}

// cut to fit maxW, adding "..." if it had to cut
static String fitText(const char *s, int maxW, const lgfx::IFont *f) {
  char a[200];
  asciiize(a, s, sizeof a, true);
  if (strW(a, f) <= maxW) return String(a);
  int dots = charW(f, '.') * 3, w = 0, i = 0;
  for (; a[i]; i++) { int c = charW(f, a[i]); if (w + c + dots > maxW) break; w += c; }
  a[i] = 0;
  return String(a) + "...";
}

// greedy word wrap of an ASCII string; fills line starts/lengths, returns line count
static int wrapText(const char *s, int maxW, const lgfx::IFont *f, uint16_t *ls, uint8_t *ll, int maxL) {
  int len = strlen(s), n = 0, i = 0;
  while (i < len && n < maxL) {
    int start = i, lastSpace = -1, w = 0, j = i;
    while (j < len && s[j] != '\n') {
      int c = charW(f, s[j]);
      if (w + c > maxW) break;
      if (s[j] == ' ') lastSpace = j;
      w += c;
      j++;
    }
    if (j >= len || s[j] == '\n') { ls[n] = start; ll[n] = j - start; n++; i = j + 1; continue; }
    if (lastSpace > start) { ls[n] = start; ll[n] = lastSpace - start; i = lastSpace + 1; }
    else { int e = j > start ? j : start + 1; ls[n] = start; ll[n] = e - start; i = e; }
    n++;
  }
  if (n == 0) { ls[0] = 0; ll[0] = 0; n = 1; }
  return n;
}

static void drawLines(const char *s, const uint16_t *ls, const uint8_t *ll, int n, int x, int y, int step,
                      const lgfx::IFont *f, uint32_t c) {
  char line[200];
  for (int i = 0; i < n; i++) {
    int k = min((int)ll[i], (int)sizeof line - 1);
    memcpy(line, s + ls[i], k);
    line[k] = 0;
    txt(line, x, y + i * step, f, c, D_TL);
  }
}

static void paragraph(const char *s, int x, int y, int w, const lgfx::IFont *f, uint32_t c, int step) {
  char a[300];
  asciiize(a, s, sizeof a, false);
  uint16_t ls[10]; uint8_t ll[10];
  int n = wrapText(a, w, f, ls, ll, 10);
  drawLines(a, ls, ll, n, x, y, step, f, c);
}

// ---------- small ornaments ----------
static void lockIcon(int x, int y, uint32_t c) {   // 10 x 12, top-left at x,y
  rframe(x + 2, y, 6, 8, 3, c);
  rframe(x + 3, y + 1, 4, 7, 2, c);
  fillR(x, y + 5, 10, 7, c);
}
static void badge(int xr, int y, int n) {          // right-aligned pill, vertically centered on y
  char b[6];
  snprintf(b, sizeof b, "%d", n > 99 ? 99 : n);
  int w = max(20, strW(b, F_SMALL) + 12);
  rfill(xr - w, y - 10, w, 20, 10, COL_GOLD);
  txt(b, xr - w / 2, y, F_SMALL, COL_INK, D_MC);
}
static void signalBars(int x, int y, int rssi) {   // bottom-left at x,y
  int bars = rssi > -55 ? 4 : rssi > -67 ? 3 : rssi > -78 ? 2 : 1;
  for (int i = 0; i < 4; i++) fillR(x + i * 5, y - 4 - i * 3, 3, 4 + i * 3, i < bars ? COL_VERD : COL_LINE);
}
static void decoRule(int y) {                       // gold double rule with a centre diamond
  hline(0, y, 240, COL_GOLD);
  hline(0, y + 2, 240, COL_LINE);
  tri(120, y - 3, 124, y + 1, 116, y + 1, COL_GOLD);
  tri(116, y + 1, 124, y + 1, 120, y + 5, COL_GOLD);
}
static void plate(const char *s, int xr, int y, uint32_t c) {   // art-deco ID plate, right-aligned, centered on y
  int w = strW(s, F_SMALL) + 18;
  int x = xr - w;
  rframe(x, y - 11, w, 22, 0, c);
  fillR(x, y - 11, 3, 22, c);
  fillR(x + w - 3, y - 11, 3, 22, c);
  txt(s, x + w / 2, y, F_SMALL, c, D_MC);
}
static void button(int x, int y, int w, int h, const char *label, bool solid, uint32_t c) {
  if (solid) rfill(x, y, w, h, 3, c);
  else rframe(x, y, w, h, 3, c);
  txt(label, x + w / 2, y + h / 2, F_SMALL, solid ? COL_INK : c, D_MC);
}

// The CA53Y IND sunburst mark. Drawn, not stored, so it costs no flash.
// To use your own artwork instead, run tools/make_logo.py; it writes logo.h next to this file.
#if __has_include("logo.h")
#include "logo.h"
#define HAS_LOGO 1
#else
#define HAS_LOGO 0
#endif

static void sunburst(int cx, int cy, int r0, int r1, uint32_t c) {
  for (int a = 0; a < 360; a += 10) {
    float t0 = (a - 2.2f) * DEG_TO_RAD, t1 = (a + 2.2f) * DEG_TO_RAD, tm = a * DEG_TO_RAD;
    tri(cx + cosf(t0) * r0, cy + sinf(t0) * r0, cx + cosf(t1) * r0, cy + sinf(t1) * r0,
        cx + cosf(tm) * r1, cy + sinf(tm) * r1, c);
  }
}

static void drawSplash(const char *status) {
  fillR(0, 0, 240, 320, COL_CREAM);
#if HAS_LOGO
  if (vis(150 - CA53Y_LOGO_H / 2, CA53Y_LOGO_H))
    G->pushImage((240 - CA53Y_LOGO_W) / 2, 150 - CA53Y_LOGO_H / 2 - OY, CA53Y_LOGO_W, CA53Y_LOGO_H,
                 (const lgfx::swap565_t *)ca53y_logo);
#else
  const int cx = 120, cy = 140;
  sunburst(cx, cy, 76, 112, COL_GOLD);
  circ(cx, cy, 76, COL_CREAM);
  ring(cx, cy, 76, COL_DGOLD);
  ring(cx, cy, 72, COL_DGOLD);
  ring(cx, cy, 71, COL_DGOLD);
  txt("CA53Y", cx + 1, cy - 4, F_LOGO, COL_GOLD, D_MC);   // gold with a darker edge, like stamped foil
  txt("CA53Y", cx, cy - 5, F_LOGO, COL_DGOLD, D_MC);
  int w = 44;
  fillR(cx - w / 2, cy + 22, w, 18, COL_DGOLD);
  txt("IND", cx, cy + 31, F_SMALL, COL_CREAM, D_MC);
  hline(cx - 60, cy + 31, 30, COL_DGOLD);
  hline(cx + 30, cy + 31, 30, COL_DGOLD);
#endif
  txt("Comms", 120, 268, F_TITLE, COL_INK, D_MC);
  txt(status, 120, 298, F_SMALL, COL_DGOLD, D_MC);
}

// ---------- UI state ----------
enum : uint8_t { SCR_SPLASH, SCR_HOME, SCR_THREAD, SCR_KEYS, SCR_CONTACT, SCR_CHAN, SCR_CONFIRM };
enum : uint8_t { TAB_CHATS, TAB_CHANS, TAB_NEAR, TAB_SET };
enum : uint8_t { ED_MSG, ED_MYNAME, ED_C_ID, ED_C_NICK, ED_C_PIN, ED_CH_NAME, ED_CH_PASS };
enum : uint8_t { CONF_ERASE, CONF_DEL_CONTACT, CONF_LEAVE_CHAN };

#define HEAD_H  44
#define TAB_Y   276
#define LIST_Y  44
#define LST_H  (TAB_Y - LIST_Y)
#define ROW_H   48
#define TH_TOP  56
#define TH_BOT  276

uint8_t scr = SCR_SPLASH, tab = TAB_CHATS;
int listScroll[4] = {0, 0, 0, 0};
bool needDraw = true;
uint32_t lastDraw = 0;

// thread
uint8_t thKind = 0;          // 0 direct, 1 channel
char thPeer[20];
int thScroll = 0;
bool thStick = true;          // follow new messages
bool layoutDirty = true;
struct TItem { uint32_t uid; int y, h, bw; uint8_t nl; uint16_t ls[10]; uint8_t ll[10]; bool label, meta; };
TItem titems[MAX_MSGS];
int tcount = 0, tContentH = 0;

// keyboard
struct Editor {
  uint8_t target, layer, ret;
  bool shift, mask;
  int maxLen;
  char buf[200];
  char title[32];
  char hint[120];
} ed;

// contact / channel forms
struct { bool editing, hadPin; uint8_t pinMode; char id[20], nick[24], pin[33]; } cf;   // pinMode 0 keep, 1 set, 2 clear
struct { int idx; char name[24], pass[33]; } chf;
uint8_t confirmAction = 0;
int confirmIdx = -1;

// toast (incoming message or a short note)
struct { bool on, tappable; uint32_t until; uint8_t kind; char peer[20]; char title[40]; char text[64]; uint32_t color; } toast;

// screen power
bool asleep = false;
uint32_t lastInput = 0;
static const uint8_t BRIGHT[4] = {40, 90, 160, 255};
static const uint32_t SLEEP_MS[4] = {30000, 60000, 300000, 0};
static const char *SLEEP_LBL[4] = {"30 seconds", "1 minute", "5 minutes", "Never"};

// sound
uint8_t beepStage = 0;
uint32_t beepAt = 0;

// ---------- names ----------
static const char *contactName(int ci) { return contacts[ci].nick[0] ? contacts[ci].nick : contacts[ci].id + PREFIX_LEN; }
static String senderName(const Msg *m) {
  if (m->dir == 1) return String("You");
  int ci = findContact(m->from);
  if (ci >= 0 && contacts[ci].nick[0]) return String(contacts[ci].nick);
  if (m->fromNick[0]) return String(m->fromNick);
  return String(m->from + PREFIX_LEN);
}
static Msg *lastMsgFor(const char *peer) {
  for (int i = msgCount - 1; i >= 0; i--) {
    Msg *m = msgAt(i);
    if (m->st != 99 && !strcasecmp(m->peer, peer)) return m;
  }
  return nullptr;
}
static bool anyUnread(uint8_t which) {
  if (which == 0) { for (int i = 0; i < nContacts; i++) if (crt[i].unread) return true; }
  else { for (int i = 0; i < nChans; i++) if (chrt[i].unread) return true; }
  return false;
}

// sorted views (most recent first)
static int sortedContacts(int *idx) {
  for (int i = 0; i < nContacts; i++) idx[i] = i;
  for (int i = 1; i < nContacts; i++)
    for (int j = i; j > 0 && crt[idx[j]].lastAct > crt[idx[j - 1]].lastAct; j--) std::swap(idx[j], idx[j - 1]);
  return nContacts;
}
static int sortedChans(int *idx) {
  for (int i = 0; i < nChans; i++) idx[i] = i;
  for (int i = 1; i < nChans; i++)
    for (int j = i; j > 0 && chrt[idx[j]].lastAct > chrt[idx[j - 1]].lastAct; j--) std::swap(idx[j], idx[j - 1]);
  return nChans;
}
static int nearbyList(Nearby **out) {
  int n = 0;
  uint32_t now = millis();
  for (auto &nb : nearby) if (nb.used && now - nb.seen <= NEARBY_TTL_MS) out[n++] = &nb;
  for (int i = 1; i < n; i++)
    for (int j = i; j > 0 && out[j]->rssi > out[j - 1]->rssi; j--) std::swap(out[j], out[j - 1]);
  return n;
}

static void showNote(const char *text, uint32_t color) {
  toast.on = true;
  toast.tappable = false;
  toast.until = millis() + 3000;
  toast.color = color;
  toast.title[0] = 0;
  strlcpy(toast.text, text, sizeof toast.text);
  needDraw = true;
}

// ---------- screen: home ----------
static int homeRows() {
  if (tab == TAB_CHATS) return 1 + nContacts;
  if (tab == TAB_CHANS) return 1 + nChans;
  if (tab == TAB_NEAR) { Nearby *nb[MAX_NEARBY]; return nearbyList(nb); }
  return 9;
}

static void row(int y, const char *title, const String &sub, uint32_t titleCol, int unread, bool lock, const char *right) {
  if (!vis(y, ROW_H)) return;
  int maxW = 216 - (unread ? 30 : 0) - (right ? strW(right, F_SMALL) + 10 : 0) - (lock ? 16 : 0);
  String t = fitText(title, maxW, F_BODY);
  txt(t.c_str(), 14, y + 16, F_BODY, titleCol, D_ML);
  if (lock) lockIcon(14 + strW(t.c_str(), F_BODY) + 6, y + 9, COL_GOLD);
  int subW = 212 - (unread ? 30 : 0) - (right && !unread ? strW(right, F_SMALL) + 10 : 0);
  if (sub.length()) txt(fitText(sub.c_str(), subW, F_SMALL).c_str(), 14, y + 35, F_SMALL, COL_MIST, D_ML);
  if (unread) badge(228, y + 24, unread);
  else if (right) txt(right, 228, y + 24, F_SMALL, COL_GOLD, D_MR);
  hline(14, y + ROW_H - 1, 212, COL_LINE);
}

static void drawHomeList() {
  int y0 = LIST_Y - listScroll[tab];
  if (tab == TAB_CHATS) {
    row(y0, "+  Add contact", String("By device ID, or pick from Nearby"), COL_GOLD, 0, false, nullptr);
    int idx[MAX_CONTACTS];
    int n = sortedContacts(idx);
    for (int k = 0; k < n; k++) {
      int i = idx[k];
      Msg *m = lastMsgFor(contacts[i].id);
      String sub;
      if (crt[i].isNew && !m) sub = "New: sent you a message";
      else if (m) sub = m->st == 11 ? String("Locked message") : String(m->dir ? "You: " : "") + m->text;
      else sub = contacts[i].nick[0] ? String(contacts[i].id) : String("No messages yet");
      row(y0 + (k + 1) * ROW_H, contactName(i), sub, crt[i].isNew ? COL_CHAMP : COL_IVORY, crt[i].unread, contacts[i].pin[0], nullptr);
    }
    if (!n) paragraph("No contacts yet. Pick a device from Nearby, or tap Add contact and type its ID.",
                      14, y0 + ROW_H + 14, 212, F_SMALL, COL_MIST, 18);
  } else if (tab == TAB_CHANS) {
    row(y0, "+  Join a channel", String("Open, or private with a passphrase"), COL_GOLD, 0, false, nullptr);
    int idx[MAX_CHANNELS];
    int n = sortedChans(idx);
    for (int k = 0; k < n; k++) {
      int i = idx[k];
      Msg *m = lastMsgFor(chrt[i].addr);
      String sub = m ? senderName(m) + ": " + m->text : String(chrt[i].hasKey ? "Private channel" : "Open channel");
      String title = String("# ") + chans[i].name;
      row(y0 + (k + 1) * ROW_H, title.c_str(), sub, COL_IVORY, chrt[i].unread, chrt[i].hasKey, nullptr);
    }
    if (!n) paragraph("Channels are group chats. Everyone who joins the same name (and passphrase, if it has one) sees the messages.",
                      14, y0 + ROW_H + 14, 212, F_SMALL, COL_MIST, 18);
  } else if (tab == TAB_NEAR) {
    Nearby *nb[MAX_NEARBY];
    int n = nearbyList(nb);
    for (int k = 0; k < n; k++) {
      int y = y0 + k * ROW_H;
      int ci = findContact(nb[k]->id);
      bool isC = ci >= 0;
      if (!vis(y, ROW_H)) continue;
      const char *nm = isC && contacts[ci].nick[0] ? contacts[ci].nick : nb[k]->nick[0] ? nb[k]->nick : nb[k]->id + PREFIX_LEN;
      row(y, nm, String(nb[k]->id), COL_IVORY, 0, false, isC ? "Contact" : "Add");
      signalBars(176 - (isC ? 20 : 0), y + 31, nb[k]->rssi);
    }
    if (!n) paragraph("No CA53Y devices heard yet. Devices show up here while they're switched on and visible. Messages can still reach contacts through repeaters.",
                      14, y0 + 14, 212, F_SMALL, COL_MIST, 18);
  } else {
    char b[64];
    row(y0, "Your name", String(myNick[0] ? myNick : "Not set"), COL_IVORY, 0, false, "Edit");
    row(y0 + ROW_H, "Visible to others", String(discoverable ? "On: you show up in Nearby lists" : "Off: people add you by ID"), COL_IVORY, 0, false, discoverable ? "On" : "Off");
    snprintf(b, sizeof b, relayOn ? "On: %lu passed along for others" : "Off: only your own messages", (unsigned long)relayedCount);
    row(y0 + ROW_H * 2, "Repeat messages", String(b), COL_IVORY, 0, false, relayOn ? "On" : "Off");
    row(y0 + ROW_H * 3, "Sound", String("Chime on new messages"), COL_IVORY, 0, false, soundOn ? "On" : "Off");
    snprintf(b, sizeof b, "%d%%", (brightIdx + 1) * 25);
    row(y0 + ROW_H * 4, "Brightness", String("Tap to change"), COL_IVORY, 0, false, b);
    row(y0 + ROW_H * 5, "Screen sleep", String("Tap the screen to wake it"), COL_IVORY, 0, false, SLEEP_LBL[sleepIdx]);
    row(y0 + ROW_H * 6, "Calibrate touch", String("If taps land in the wrong place"), COL_IVORY, 0, false, nullptr);
    snprintf(b, sizeof b, "%s  v%s", myId, FW_VERSION);
    row(y0 + ROW_H * 7, "About", String(b), COL_IVORY, 0, false, nullptr);
    row(y0 + ROW_H * 8, "Erase everything", String("Name, contacts, channels, settings"), COL_ROSE, 0, false, nullptr);
  }
}

static void drawHeader() {
  fillR(0, 0, 240, HEAD_H, COL_LAC);
  txt("CA53Y", 12, 20, F_TITLEB, COL_GOLD, D_ML);
  G->setFont(F_TITLEB);
  txt("Comms", 12 + G->textWidth("CA53Y") + 5, 22, F_SERIF9, COL_CHAMP, D_ML);
  plate(myId + PREFIX_LEN, 230, 20, COL_GOLD);
  decoRule(39);
}

static void drawTabs() {
  static const char *names[4] = {"Chats", "Channels", "Nearby", "Settings"};
  fillR(0, TAB_Y, 240, 320 - TAB_Y, COL_LAC2);
  hline(0, TAB_Y, 240, COL_LINE);
  for (int i = 0; i < 4; i++) {
    int x = i * 60;
    bool sel = tab == i;
    if (sel) fillR(x + 12, TAB_Y + 1, 36, 3, COL_GOLD);
    txt(names[i], x + 30, TAB_Y + 22, F_SMALL, sel ? COL_GOLD : COL_MIST, D_MC);
    if ((i == 0 || i == 1) && anyUnread(i)) circ(x + 55, TAB_Y + 8, 3, COL_ROSE);
  }
}

static void drawHome() {
  fillR(0, 0, 240, 320, COL_LAC);
  drawHomeList();
  drawHeader();
  drawTabs();
}

// ---------- screen: thread ----------
static bool threadValid() {
  return thKind == 0 ? findContact(thPeer) >= 0 : findChanByAddr(thPeer) >= 0;
}

static void layoutThread() {
  tcount = 0;
  int y = 8;
  char a[MAX_TEXT + 8];
  for (int i = 0; i < msgCount; i++) {
    Msg *m = msgAt(i);
    if (m->st == 99 || strcasecmp(m->peer, thPeer)) continue;
    TItem &t = titems[tcount++];
    t.uid = m->uid;
    t.label = thKind == 1 && m->dir == 0;
    t.meta = m->dir == 1 || m->st == 11;
    if (m->st == 11) strlcpy(a, "Locked message. Your PINs for each other don't match.", sizeof a);
    else asciiize(a, m->text, sizeof a, false);
    t.nl = wrapText(a, 172, F_BODY, t.ls, t.ll, 10);
    int bw = 0;
    for (int k = 0; k < t.nl; k++) {
      int w = 0;
      for (int c = 0; c < t.ll[k]; c++) w += charW(F_BODY, a[t.ls[k] + c]);
      bw = max(bw, w);
    }
    t.bw = bw + 20;
    t.y = y;
    t.h = (t.label ? 18 : 0) + t.nl * 20 + 12 + (t.meta ? 18 : 0);
    y += t.h + 6;
  }
  tContentH = y + 4;
  layoutDirty = false;
}

static int threadMaxScroll() { return max(0, tContentH - (TH_BOT - TH_TOP)); }

static void drawThread() {
  if (layoutDirty) {
    layoutThread();
    if (thStick) thScroll = threadMaxScroll();
  }
  fillR(0, 0, 240, 320, COL_LAC);
  char a[MAX_TEXT + 8];
  for (int k = 0; k < tcount; k++) {
    TItem &t = titems[k];
    int y = TH_TOP + t.y - thScroll;
    if (y > TH_BOT || y + t.h < TH_TOP - 4 || !vis(y, t.h)) continue;
    Msg *m = findMsg(t.uid);
    if (!m) continue;
    bool out = m->dir == 1;
    if (t.label) { txt(fitText(senderName(m).c_str(), 200, F_SMALL).c_str(), 12, y + 8, F_SMALL, COL_GOLD, D_ML); y += 18; }
    int bh = t.nl * 20 + 12;
    int bx = out ? 232 - t.bw : 8;
    if (m->st == 11) strlcpy(a, "Locked message. Your PINs for each other don't match.", sizeof a);
    else asciiize(a, m->text, sizeof a, false);
    if (out) rfill(bx, y, t.bw, bh, 8, COL_CHAMP);
    else { rfill(bx, y, t.bw, bh, 8, COL_LAC2); rframe(bx, y, t.bw, bh, 8, COL_LINE); }
    drawLines(a, t.ls, t.ll, t.nl, bx + 10, y + 5, 20, F_BODY, out ? COL_INK : (m->st == 11 ? COL_MIST : COL_IVORY));
    if (t.meta) {
      const char *st = "";
      uint32_t c = COL_MIST;
      switch (m->st) {
        case 0: st = "Sending"; break;
        case 1: st = "Delivered"; c = COL_VERD; break;
        case 2: st = "Not delivered. Tap to resend"; c = COL_ROSE; break;
        case 3: st = "Delivered, but their PIN doesn't match"; c = COL_ROSE; break;
        case 4: st = "Sent"; break;
        case 11: st = "Ask them which PIN they set for you"; c = COL_ROSE; break;
      }
      txt(st, out ? 232 : 10, y + bh + 9, F_SMALL, c, out ? D_MR : D_ML);
    }
  }
  if (!tcount) txt(thKind ? "No messages in this channel yet." : "No messages yet. Say hello.", 120, 160, F_SMALL, COL_MIST, D_MC);

  // header
  fillR(0, 0, 240, TH_TOP, COL_LAC);
  txt("<", 12, 17, F_TITLE, COL_GOLD, D_ML);
  const char *title = thPeer;
  bool priv = false;
  String tname;
  if (thKind == 0) {
    int ci = findContact(thPeer);
    if (ci >= 0) { tname = contactName(ci); priv = crt[ci].hasKey; }
  } else {
    int chi = findChanByAddr(thPeer);
    if (chi >= 0) { tname = String("# ") + chans[chi].name; priv = chrt[chi].hasKey; }
  }
  title = tname.c_str();
  txt(fitText(title, 148, F_TITLE).c_str(), 30, 18, F_TITLE, COL_CHAMP, D_ML);
  button(186, 6, 46, 24, "Edit", false, COL_GOLD);
  if (priv) {
    lockIcon(12, 36, COL_GOLD);
    txt(thKind ? "Private: passphrase encrypted" : "Private: encrypted with your PIN", 28, 42, F_SMALL, COL_GOLD, D_ML);
  } else {
    txt(thKind ? "Open: any CA53Y device can read it" : "Open: tap Edit to add a PIN", 12, 42, F_SMALL, COL_MIST, D_ML);
  }
  hline(0, TH_TOP - 1, 240, COL_LINE);

  // compose bar
  fillR(0, TH_BOT, 240, 320 - TH_BOT, COL_LAC);
  hline(0, TH_BOT, 240, COL_LINE);
  rfill(8, TH_BOT + 8, 224, 30, 4, COL_LAC2);
  rframe(8, TH_BOT + 8, 224, 30, 4, COL_LINE);
  txt(thKind ? "Write to the channel" : "Write a message", 18, TH_BOT + 23, F_SMALL, COL_MIST, D_ML);
}

static void openThread(uint8_t kind, const char *peer) {
  thKind = kind;
  strlcpy(thPeer, peer, sizeof thPeer);
  thStick = true;
  layoutDirty = true;
  if (kind == 0) { int ci = findContact(peer); if (ci >= 0) { crt[ci].unread = 0; crt[ci].isNew = false; } }
  else { int chi = findChanByAddr(peer); if (chi >= 0) chrt[chi].unread = 0; }
  scr = SCR_THREAD;
  needDraw = true;
}

// ---------- screen: keyboard ----------
#define KB_Y 146
#define KB_H 42
static const char *KROWS[3][3] = {
  {"qwertyuiop", "asdfghjkl", "zxcvbnm"},
  {"1234567890", "-/:;()&@\"", ".,?!'_#"},
  {"[]{}%^*+=~", "\\|<>`$_%#", ".,?!;:/"},
};
enum { K_SHIFT = 1, K_DEL, K_LAYER, K_LAYER2, K_OK };
struct Key { int x, w, code; char label[8]; };

static int kbRow(int r, Key *k) {
  int n = 0;
  if (r <= 1) {
    const char *s = KROWS[ed.layer][r];
    int cnt = strlen(s), x = (240 - cnt * 24) / 2;
    for (int i = 0; i < cnt; i++) {
      char c = s[i];
      if (ed.layer == 0 && ed.shift) c = toupper(c);
      k[n].x = x + i * 24; k[n].w = 24; k[n].code = c; k[n].label[0] = c; k[n].label[1] = 0; n++;
    }
  } else if (r == 2) {
    k[n] = {0, 36, ed.layer == 0 ? K_SHIFT : K_LAYER2, ""};
    strlcpy(k[n].label, ed.layer == 0 ? "Shift" : ed.layer == 1 ? "#+=" : "123", 8);
    n++;
    const char *s = KROWS[ed.layer][2];
    for (int i = 0; i < 7; i++) {
      char c = s[i];
      if (ed.layer == 0 && ed.shift) c = toupper(c);
      k[n].x = 36 + i * 24; k[n].w = 24; k[n].code = c; k[n].label[0] = c; k[n].label[1] = 0; n++;
    }
    k[n] = {204, 36, K_DEL, "Del"}; n++;
  } else {
    k[n] = {0, 48, K_LAYER, ""};
    strlcpy(k[n].label, ed.layer == 0 ? "123" : "ABC", 8);
    n++;
    k[n] = {48, 24, ',', ","}; n++;
    k[n] = {72, 96, ' ', "space"}; n++;
    k[n] = {168, 24, '.', "."}; n++;
    k[n] = {192, 48, K_OK, ""};
    strlcpy(k[n].label, ed.target == ED_MSG ? "Send" : "Done", 8);
    n++;
  }
  return n;
}

static void drawKeys() {
  fillR(0, 0, 240, 320, COL_LAC);
  // top bar
  txt("Cancel", 8, 14, F_SMALL, COL_GOLD, D_ML);
  txt(ed.title, 120, 14, F_SMALL, COL_CHAMP, D_MC);
  char cnt[16];
  snprintf(cnt, sizeof cnt, "%d/%d", (int)strlen(ed.buf), ed.maxLen);
  txt(cnt, 232, 14, F_SMALL, (int)strlen(ed.buf) >= ed.maxLen ? COL_ROSE : COL_MIST, D_MR);
  // text box
  rfill(6, 28, 228, 114, 4, COL_LAC2);
  rframe(6, 28, 228, 114, 4, COL_LINE);
  char a[220];
  if (ed.mask) { size_t n = strlen(ed.buf); memset(a, '*', n); a[n] = 0; }
  else asciiize(a, ed.buf, sizeof a, false);
  uint16_t ls[14]; uint8_t ll[14];
  int n = wrapText(a, 210, F_BODY, ls, ll, 14);
  int first = max(0, n - 5);
  if (!a[0] && ed.hint[0]) paragraph(ed.hint, 14, 36, 210, F_SMALL, COL_MIST, 18);
  drawLines(a, ls + first, ll + first, n - first, 14, 34, 20, F_BODY, COL_IVORY);
  int last = n - 1, cw = 0;
  for (int c = 0; c < ll[last]; c++) cw += charW(F_BODY, a[ls[last] + c]);
  if (a[0] || !ed.hint[0]) vline(14 + cw + 1, 36 + (last - first) * 20, 15, COL_GOLD);
  // keys
  Key k[12];
  for (int r = 0; r < 4; r++) {
    int y = KB_Y + r * KB_H;
    int cnt2 = kbRow(r, k);
    for (int i = 0; i < cnt2; i++) {
      bool special = k[i].code < 32;
      bool ok = k[i].code == K_OK;
      bool active = k[i].code == K_SHIFT && ed.shift;
      rfill(k[i].x + 1, y + 1, k[i].w - 2, KB_H - 3, 4, ok ? COL_GOLD : active ? COL_DGOLD : special ? COL_LINE : COL_LAC2);
      if (!ok && !special) rframe(k[i].x + 1, y + 1, k[i].w - 2, KB_H - 3, 4, COL_LINE);
      bool small = special || k[i].code == ' ';
      txt(k[i].label, k[i].x + k[i].w / 2, y + KB_H / 2, small ? F_SMALL : F_BODY, ok ? COL_INK : COL_IVORY, D_MC);
    }
  }
}

static void openEditor(uint8_t target, const char *title, const char *initial, int maxLen, bool mask, const char *hint) {
  ed.target = target;
  ed.ret = scr;
  ed.layer = 0;
  ed.mask = mask;
  ed.maxLen = min(maxLen, (int)sizeof ed.buf - 1);
  strlcpy(ed.title, title, sizeof ed.title);
  strlcpy(ed.buf, initial ? initial : "", ed.maxLen + 1);
  strlcpy(ed.hint, hint ? hint : "", sizeof ed.hint);
  ed.shift = !ed.buf[0] && (target == ED_MSG || target == ED_MYNAME || target == ED_C_NICK);
  scr = SCR_KEYS;
  needDraw = true;
}

void finishSetup();   // .ino

static void editorDone() {
  String v = ed.buf;
  if (ed.target != ED_C_PIN && ed.target != ED_CH_PASS) v.trim();
  scr = ed.ret;
  switch (ed.target) {
    case ED_MSG: {
      if (!v.length()) break;
      const char *err;
      if (thKind == 0) err = sendDirect(findContact(thPeer), v.c_str());
      else err = sendChannel(findChanByAddr(thPeer), v.c_str());
      if (err) { showNote(err, COL_ROSE); scr = SCR_KEYS; return; }
      thStick = true;
      layoutDirty = true;
      break;
    }
    case ED_MYNAME:
      cleanText(myNick, sizeof myNick, v.c_str(), v.length(), false);
      prefs.putString("nick", myNick);
      if (!isSetup) finishSetup();
      break;
    case ED_C_ID: {
      char id[20];
      if (!normalizeId(v, id)) { showNote("IDs look like 1234abC (4 numbers, then 3 letters)", COL_ROSE); scr = SCR_KEYS; return; }
      if (!strcasecmp(id, myId)) { showNote("That's this device's own ID", COL_ROSE); scr = SCR_KEYS; return; }
      strlcpy(cf.id, id, sizeof cf.id);
      Nearby *nb = findNearby(id);
      if (!cf.nick[0] && nb && nb->nick[0]) strlcpy(cf.nick, nb->nick, sizeof cf.nick);
      break;
    }
    case ED_C_NICK: cleanText(cf.nick, sizeof cf.nick, v.c_str(), v.length(), false); break;
    case ED_C_PIN:
      if (!v.length()) break;
      if (v.length() < 4) { showNote("PINs are 4 to 32 characters", COL_ROSE); scr = SCR_KEYS; return; }
      strlcpy(cf.pin, v.c_str(), sizeof cf.pin);
      cf.pinMode = 1;
      break;
    case ED_CH_NAME: cleanText(chf.name, sizeof chf.name, v.c_str(), v.length(), false); break;
    case ED_CH_PASS:
      if (v.length() && v.length() < 4) { showNote("Passphrases are 4 to 32 characters", COL_ROSE); scr = SCR_KEYS; return; }
      strlcpy(chf.pass, v.c_str(), sizeof chf.pass);
      break;
  }
  needDraw = true;
}

static void keyTap(int x, int y) {
  if (y < 28) {
    if (x < 70) { scr = ed.ret; needDraw = true; if (!isSetup) finishSetup(); }
    return;
  }
  if (y < KB_Y) return;
  int r = (y - KB_Y) / KB_H;
  if (r > 3) return;
  Key k[12];
  int n = kbRow(r, k);
  for (int i = 0; i < n; i++) {
    if (x < k[i].x || x >= k[i].x + k[i].w) continue;
    int c = k[i].code;
    size_t len = strlen(ed.buf);
    if (c == K_SHIFT) ed.shift = !ed.shift;
    else if (c == K_LAYER) { ed.layer = ed.layer == 0 ? 1 : 0; ed.shift = false; }
    else if (c == K_LAYER2) ed.layer = ed.layer == 1 ? 2 : 1;
    else if (c == K_DEL) {
      while (len > 0 && ((uint8_t)ed.buf[len - 1] & 0xC0) == 0x80) len--;   // whole UTF-8 character
      if (len > 0) len--;
      ed.buf[len] = 0;
    } else if (c == K_OK) { editorDone(); return; }
    else if ((int)len < ed.maxLen) {
      ed.buf[len] = (char)c;
      ed.buf[len + 1] = 0;
      if (ed.layer == 0) ed.shift = false;
      if (ed.layer != 0 && c == ' ') ed.layer = 0;
    }
    needDraw = true;
    return;
  }
}

// ---------- screen: contact form ----------
static void field(const char *label, const char *value, const char *placeholder, int y, bool enabled) {
  txt(label, 12, y, F_SMALL, COL_CHAMP, D_TL);
  rfill(8, y + 16, 224, 30, 3, enabled ? COL_LAC2 : COL_LAC);
  rframe(8, y + 16, 224, 30, 3, COL_LINE);
  if (value && value[0]) txt(fitText(value, 206, F_BODY).c_str(), 16, y + 31, F_BODY, enabled ? COL_IVORY : COL_MIST, D_ML);
  else txt(placeholder, 16, y + 31, F_SMALL, COL_MIST, D_ML);
}

static void formHeader(const char *title) {
  txt(title, 12, 22, F_TITLE, COL_CHAMP, D_ML);
  txt("Cancel", 230, 22, F_SMALL, COL_GOLD, D_MR);
  decoRule(42);
}

static void drawContactForm() {
  fillR(0, 0, 240, 320, COL_LAC);
  formHeader(cf.editing ? "Edit contact" : "Add contact");
  field("Device ID", cf.id, "Tap to type it, e.g. 1234abC", 52, !cf.editing);
  field("Name", cf.nick, "Optional", 104, true);
  char masked[40];
  const char *pv = nullptr;
  if (cf.pinMode == 1) { size_t n = strlen(cf.pin); memset(masked, '*', n); masked[n] = 0; pv = masked; }
  else if (cf.pinMode == 0 && cf.hadPin) pv = "Set. Tap to change it";
  field("Shared PIN", pv, "None. Tap to add one", 156, true);
  bool priv = cf.pinMode == 1 || (cf.pinMode == 0 && cf.hadPin);
  paragraph(priv ? "Private: encrypted. They must set this same PIN for you on their device."
                 : "Open: nearby devices could read these. Set the same PIN on both devices to encrypt.",
            12, 206, 216, F_SMALL, priv ? COL_GOLD : COL_MIST, 16);
  button(8, 260, priv && cf.editing ? 108 : 224, 26, cf.editing ? "Save changes" : "Save contact", true, COL_GOLD);
  if (priv && cf.editing) button(124, 260, 108, 26, "Remove PIN", false, COL_MIST);
  if (cf.editing) button(8, 292, 224, 24, "Delete contact", false, COL_ROSE);
}

static void openContactForm(int ci, const char *prefId, const char *prefNick) {
  memset(&cf, 0, sizeof cf);
  if (ci >= 0) {
    cf.editing = true;
    strlcpy(cf.id, contacts[ci].id, sizeof cf.id);
    strlcpy(cf.nick, contacts[ci].nick, sizeof cf.nick);
    cf.hadPin = contacts[ci].pin[0];
  } else {
    if (prefId) strlcpy(cf.id, prefId, sizeof cf.id);
    if (prefNick) strlcpy(cf.nick, prefNick, sizeof cf.nick);
  }
  scr = SCR_CONTACT;
  needDraw = true;
}

static void saveContactForm() {
  if (!cf.id[0]) { showNote("Enter the device ID first", COL_ROSE); return; }
  int ci = findContact(cf.id);
  if (ci < 0) {
    ci = addContact(cf.id, cf.nick);
    if (ci < 0) { showNote("Contact list is full (16). Delete one first.", COL_ROSE); return; }
  }
  strlcpy(contacts[ci].nick, cf.nick, sizeof contacts[ci].nick);
  if (cf.pinMode == 1) strlcpy(contacts[ci].pin, cf.pin, sizeof contacts[ci].pin);
  if (cf.pinMode == 2) contacts[ci].pin[0] = 0;
  crt[ci].isNew = false;
  deriveKey(ci);
  saveContacts();
  layoutDirty = true;
  if (cf.editing) { scr = SCR_THREAD; showNote("Changes saved", COL_VERD); }
  else openThread(0, contacts[ci].id);
}

static void contactTap(int x, int y) {
  if (y < 42) { if (x > 160) { scr = cf.editing ? SCR_THREAD : SCR_HOME; needDraw = true; } return; }
  if (y >= 68 && y < 98 && !cf.editing) openEditor(ED_C_ID, "Device ID", cf.id + (cf.id[0] ? PREFIX_LEN : 0), 18, false, "The 7 characters after CA53Ycomms-, e.g. 1234abC");
  else if (y >= 120 && y < 150) openEditor(ED_C_NICK, "Name", cf.nick, 23, false, "What you call them");
  else if (y >= 172 && y < 202) openEditor(ED_C_PIN, "Shared PIN", "", 32, true, "4 to 32 characters. A few words are much stronger than 4 digits.");
  else if (y >= 258 && y < 288) {
    bool priv = cf.pinMode == 1 || (cf.pinMode == 0 && cf.hadPin);
    if (priv && cf.editing && x >= 124) { cf.pinMode = 2; cf.pin[0] = 0; saveContactForm(); }
    else saveContactForm();
  } else if (y >= 290 && cf.editing) {
    confirmAction = CONF_DEL_CONTACT;
    confirmIdx = findContact(cf.id);
    scr = SCR_CONFIRM;
  }
  needDraw = true;
}

// ---------- screen: channel form ----------
static void drawChanForm() {
  fillR(0, 0, 240, 320, COL_LAC);
  formHeader(chf.idx >= 0 ? "Channel" : "Join a channel");
  field("Channel name", chf.name, "e.g. Family, Hike, Public", 52, true);
  char masked[40];
  size_t n = strlen(chf.pass);
  memset(masked, '*', n);
  masked[n] = 0;
  field("Passphrase", n ? masked : nullptr, "None (open channel)", 104, true);
  paragraph(n ? "Private: encrypted. Only devices with this exact name and passphrase can read it. Repeaters pass it on without reading it."
              : "Open: any CA53Y device that joins this name can read it, and nearby devices could listen in.",
            12, 160, 216, F_SMALL, n ? COL_GOLD : COL_MIST, 16);
  button(8, 260, 224, 26, chf.idx >= 0 ? "Save changes" : "Join channel", true, COL_GOLD);
  if (chf.idx >= 0) button(8, 292, 224, 24, "Leave channel", false, COL_ROSE);
}

static void openChanForm(int chi) {
  memset(&chf, 0, sizeof chf);
  chf.idx = chi;
  if (chi >= 0) { strlcpy(chf.name, chans[chi].name, sizeof chf.name); strlcpy(chf.pass, chans[chi].pass, sizeof chf.pass); }
  scr = SCR_CHAN;
  needDraw = true;
}

static void saveChanForm() {
  String nm = chf.name;
  nm.trim();
  if (!nm.length()) { showNote("Give the channel a name", COL_ROSE); return; }
  int dup = findChanByName(nm.c_str(), chf.pass);
  if (dup >= 0 && dup != chf.idx) { openThread(1, chrt[dup].addr); return; }
  int chi = chf.idx;
  if (chi < 0) {
    chi = addChannel(nm.c_str(), chf.pass);
    if (chi < 0) { showNote("You're in 8 channels. Leave one first.", COL_ROSE); return; }
  } else {
    strlcpy(chans[chi].name, nm.c_str(), sizeof chans[chi].name);
    strlcpy(chans[chi].pass, chf.pass, sizeof chans[chi].pass);
    deriveChan(chi);
  }
  saveChannels();
  openThread(1, chrt[chi].addr);
}

static void chanTap(int x, int y) {
  if (y < 42) { if (x > 160) { scr = chf.idx >= 0 ? SCR_THREAD : SCR_HOME; needDraw = true; } return; }
  if (y >= 68 && y < 98) openEditor(ED_CH_NAME, "Channel name", chf.name, 23, false, "Everyone joining must use the same name");
  else if (y >= 120 && y < 150) openEditor(ED_CH_PASS, "Passphrase", chf.pass, 32, true, "Leave empty for an open channel. Everyone must use the same one.");
  else if (y >= 258 && y < 288) saveChanForm();
  else if (y >= 290 && chf.idx >= 0) { confirmAction = CONF_LEAVE_CHAN; confirmIdx = chf.idx; scr = SCR_CONFIRM; }
  needDraw = true;
}

// ---------- screen: confirm ----------
static void drawConfirm() {
  fillR(0, 0, 240, 320, COL_LAC);
  rfill(12, 70, 216, 180, 4, COL_LAC2);
  rframe(12, 70, 216, 180, 4, COL_GOLD);
  hline(12, 74, 216, COL_GOLD);
  const char *t = "", *b = "", *ok = "";
  if (confirmAction == CONF_ERASE) { t = "Erase everything?"; b = "Your name, contacts, PINs, channels and settings are deleted. This device keeps its ID."; ok = "Erase"; }
  if (confirmAction == CONF_DEL_CONTACT) { t = "Delete contact?"; b = "Their PIN is deleted too. Messages from them will add them back as a new contact."; ok = "Delete"; }
  if (confirmAction == CONF_LEAVE_CHAN) { t = "Leave channel?"; b = "You stop receiving its messages. You can join again with the same name and passphrase."; ok = "Leave"; }
  txt(t, 120, 98, F_TITLE, COL_CHAMP, D_MC);
  paragraph(b, 26, 120, 188, F_SMALL, COL_IVORY, 17);
  button(24, 204, 92, 30, "Cancel", false, COL_MIST);
  button(124, 204, 92, 30, ok, true, COL_ROSE);
}

static void confirmTap(int x, int y) {
  if (y < 204 || y > 236) return;
  if (x >= 124 && x < 216) {
    if (confirmAction == CONF_ERASE) {
      String id = prefs.getString("id", "");
      prefs.clear();
      prefs.putString("id", id);
      lcd.fillScreen(COL_LAC);
      delay(200);
      ESP.restart();
    }
    if (confirmAction == CONF_DEL_CONTACT) { deleteContact(confirmIdx); scr = SCR_HOME; tab = TAB_CHATS; showNote("Contact deleted", COL_VERD); return; }
    if (confirmAction == CONF_LEAVE_CHAN) { deleteChannel(confirmIdx); scr = SCR_HOME; tab = TAB_CHANS; showNote("Left the channel", COL_VERD); return; }
  } else if (x >= 24 && x < 116) {
    scr = confirmAction == CONF_ERASE ? SCR_HOME : confirmAction == CONF_DEL_CONTACT ? SCR_CONTACT : SCR_CHAN;
  }
  needDraw = true;
}

// ---------- toast ----------
static void drawToast() {
  if (!toast.on || scr == SCR_SPLASH) return;
  bool two = toast.title[0];
  int h = two ? 42 : 30;
  rfill(4, 4, 232, h, 4, COL_LAC2);
  rframe(4, 4, 232, h, 4, toast.color);
  fillR(4, 4, 4, h, toast.color);
  if (two) {
    txt(fitText(toast.title, 210, F_SMALL).c_str(), 16, 15, F_SMALL, COL_GOLD, D_ML);
    txt(fitText(toast.text, 210, F_SMALL).c_str(), 16, 33, F_SMALL, COL_IVORY, D_ML);
  } else {
    txt(fitText(toast.text, 210, F_SMALL).c_str(), 16, 19, F_SMALL, COL_IVORY, D_ML);
  }
}

// ---------- rendering ----------
static void drawScreen() {
  switch (scr) {
    case SCR_SPLASH:  drawSplash(myId); break;
    case SCR_HOME:    drawHome(); break;
    case SCR_THREAD:  drawThread(); break;
    case SCR_KEYS:    drawKeys(); break;
    case SCR_CONTACT: drawContactForm(); break;
    case SCR_CHAN:    drawChanForm(); break;
    case SCR_CONFIRM: drawConfirm(); break;
  }
  drawToast();
}

static void render() {
  if (scr == SCR_THREAD && !threadValid()) scr = SCR_HOME;
  if (scr == SCR_THREAD && layoutDirty) { layoutThread(); if (thStick) thScroll = threadMaxScroll(); }
  if (scr == SCR_HOME) {
    int maxS = max(0, homeRows() * ROW_H + 60 - LST_H);
    listScroll[tab] = constrain(listScroll[tab], 0, maxS);
  }
  if (!useStrip) { G = &lcd; OY = 0; SH = 320; lcd.startWrite(); drawScreen(); lcd.endWrite(); return; }
  // pushSprite sends the band by DMA in the background, so never draw into a band that is still
  // being sent: alternate two buffers and wait for the previous transfer before starting the next.
  lcd.startWrite();
  for (int y = 0, i = 0; y < 320; y += STRIP_H, i++) {
    LGFX_Sprite &s = strips[nStrips == 2 ? (i & 1) : 0];
    if (nStrips == 1) lcd.waitDMA();
    G = &s;
    OY = y;
    SH = STRIP_H;
    drawScreen();
    lcd.waitDMA();
    s.pushSprite(0, y);
  }
  lcd.waitDMA();
  lcd.endWrite();
  G = &lcd;
}

// ---------- incoming messages ----------
void startBeep() {
  if (!soundOn) return;
  digitalWrite(PIN_AMP_EN, LOW);
  ledcWriteTone(PIN_SPEAKER, 1568);
  beepStage = 1;
  beepAt = millis() + 70;
}

void wake() {
  if (asleep) { asleep = false; lcd.setBrightness(BRIGHT[brightIdx]); }
  lastInput = millis();
}

void onIncoming(Msg *m) {
  bool viewing = scr == SCR_THREAD && !strcasecmp(thPeer, m->peer) && !asleep;
  if (viewing) {
    if (m->kind == 0) { int ci = findContact(m->peer); if (ci >= 0) { crt[ci].unread = 0; crt[ci].isNew = false; } }
    else { int chi = findChanByAddr(m->peer); if (chi >= 0) chrt[chi].unread = 0; }
    layoutDirty = true;
    startBeep();
    return;
  }
  if (scr == SCR_THREAD) layoutDirty = true;
  toast.on = true;
  toast.tappable = true;
  toast.until = millis() + 4000;
  toast.kind = m->kind;
  toast.color = COL_GOLD;
  strlcpy(toast.peer, m->peer, sizeof toast.peer);
  if (m->kind == 0) {
    int ci = findContact(m->peer);
    snprintf(toast.title, sizeof toast.title, "%s", ci >= 0 ? contactName(ci) : m->peer);
  } else {
    int chi = findChanByAddr(m->peer);
    snprintf(toast.title, sizeof toast.title, "# %s  %s", chi >= 0 ? chans[chi].name : "", senderName(m).c_str());
  }
  strlcpy(toast.text, m->st == 11 ? "Locked message (PINs don't match)" : m->text, sizeof toast.text);
  startBeep();
  wake();
  needDraw = true;
}

// ---------- touch ----------
struct { bool down; int sx, sy, x, y, lastY; bool drag; uint8_t upCount; bool ignore; } T;

static void homeTap(int x, int y) {
  if (y >= TAB_Y) { uint8_t t = x / 60; if (t > 3) t = 3; if (t == tab) listScroll[tab] = 0; tab = t; needDraw = true; return; }
  if (y < LIST_Y) return;
  int i = (y - LIST_Y + listScroll[tab]) / ROW_H;
  if (tab == TAB_CHATS) {
    if (i == 0) { openContactForm(-1, nullptr, nullptr); return; }
    int idx[MAX_CONTACTS];
    int n = sortedContacts(idx);
    if (i - 1 < n) openThread(0, contacts[idx[i - 1]].id);
  } else if (tab == TAB_CHANS) {
    if (i == 0) { openChanForm(-1); return; }
    int idx[MAX_CHANNELS];
    int n = sortedChans(idx);
    if (i - 1 < n) openThread(1, chrt[idx[i - 1]].addr);
  } else if (tab == TAB_NEAR) {
    Nearby *nb[MAX_NEARBY];
    int n = nearbyList(nb);
    if (i < n) {
      if (findContact(nb[i]->id) >= 0) openThread(0, nb[i]->id);
      else openContactForm(-1, nb[i]->id, nb[i]->nick);
    }
  } else {
    switch (i) {
      case 0: openEditor(ED_MYNAME, "Your name", myNick, 23, false, "Shown to people you message"); break;
      case 1: discoverable = !discoverable; prefs.putBool("disc", discoverable); break;
      case 2: relayOn = !relayOn; prefs.putBool("relay", relayOn); break;
      case 3: soundOn = !soundOn; prefs.putBool("snd", soundOn); if (soundOn) startBeep(); break;
      case 4: brightIdx = (brightIdx + 1) % 4; prefs.putUChar("bri", brightIdx); lcd.setBrightness(BRIGHT[brightIdx]); break;
      case 5: sleepIdx = (sleepIdx + 1) % 4; prefs.putUChar("slp", sleepIdx); break;
      case 6: {
        uint16_t cal[8];
        lcd.fillScreen(COL_LAC);
        lcd.setFont(F_SMALL);
        lcd.setTextDatum(D_MC);
        lcd.setTextColor(COL_CHAMP);
        lcd.drawString("Tap each arrow tip with the stylus", 120, 160);
        lcd.calibrateTouch(cal, COL_GOLD, COL_LAC, 14);
        prefs.putBytes("cal", cal, sizeof cal);
        T.ignore = true;
        showNote("Touch calibrated", COL_VERD);
        break;
      }
      case 8: confirmAction = CONF_ERASE; scr = SCR_CONFIRM; break;
    }
    needDraw = true;
  }
}

static void threadTap(int x, int y) {
  if (y < TH_TOP) {
    if (x < 70 && y < 34) { scr = SCR_HOME; tab = thKind ? TAB_CHANS : TAB_CHATS; needDraw = true; }
    else if (x > 180 && y < 34) {
      if (thKind == 0) openContactForm(findContact(thPeer), nullptr, nullptr);
      else openChanForm(findChanByAddr(thPeer));
    }
    return;
  }
  if (y >= TH_BOT) {
    String to = thKind == 0 ? String("To ") + contactName(findContact(thPeer)) : String("To # ") + chans[findChanByAddr(thPeer)].name;
    openEditor(ED_MSG, to.c_str(), "", thKind ? MAX_CHAN_TEXT : MAX_TEXT, false, nullptr);
    return;
  }
  int cy = y - TH_TOP + thScroll;   // tap on a failed message resends it
  for (int k = 0; k < tcount; k++) {
    if (cy < titems[k].y || cy > titems[k].y + titems[k].h) continue;
    Msg *m = findMsg(titems[k].uid);
    if (m && m->dir == 1 && m->st == 2) { resendFailed(m->uid); thStick = true; layoutDirty = true; needDraw = true; }
    return;
  }
}

static void onTap(int x, int y) {
  if (toast.on && toast.tappable && y < 48 && scr != SCR_KEYS) {
    toast.on = false;
    openThread(toast.kind, toast.peer);
    return;
  }
  switch (scr) {
    case SCR_HOME:    homeTap(x, y); break;
    case SCR_THREAD:  threadTap(x, y); break;
    case SCR_KEYS:    keyTap(x, y); break;
    case SCR_CONTACT: contactTap(x, y); break;
    case SCR_CHAN:    chanTap(x, y); break;
    case SCR_CONFIRM: confirmTap(x, y); break;
  }
}

static void onDrag(int dy) {
  if (scr == SCR_HOME && T.sy >= LIST_Y && T.sy < TAB_Y) {
    listScroll[tab] -= dy;
    int maxS = max(0, homeRows() * ROW_H + 60 - LST_H);
    listScroll[tab] = constrain(listScroll[tab], 0, maxS);
    needDraw = true;
  } else if (scr == SCR_THREAD && T.sy >= TH_TOP && T.sy < TH_BOT) {
    thScroll = constrain(thScroll - dy, 0, threadMaxScroll());
    thStick = thScroll >= threadMaxScroll();
    needDraw = true;
  }
}

void touchLoop() {
  int32_t x, y;
  bool touched = lcd.getTouch(&x, &y);
  if (touched) {
    T.upCount = 0;
    if (!T.down) {
      T.down = true;
      T.sx = T.x = x; T.sy = T.y = y; T.lastY = y;
      T.drag = false;
      T.ignore = asleep;      // the touch that wakes the screen doesn't press anything
      wake();
      return;
    }
    T.x = x; T.y = y;
    lastInput = millis();
    if (T.ignore) return;
    if (!T.drag && abs(y - T.sy) > 10) T.drag = true;
    if (T.drag && abs(y - T.lastY) >= 4) { onDrag(y - T.lastY); T.lastY = y; }
  } else if (T.down) {
    if (++T.upCount < 3) return;   // resistive panels drop out for a moment mid-press
    T.down = false;
    if (!T.ignore && !T.drag) onTap(T.sx, T.sy);
    T.ignore = false;
  }
}

// ---------- LED, sound, sleep ----------
static void setLed(int pin, bool on) {
  static uint8_t state = 0xFF;
  uint8_t bit = pin == PIN_LED_R ? 1 : pin == PIN_LED_G ? 2 : 4;
  bool cur = (state & bit) == 0;
  if (state != 0xFF && cur == on) return;
  if (state == 0xFF) state = 0x07;
  digitalWrite(pin, on ? LOW : HIGH);   // common anode
  if (on) state &= ~bit; else state |= bit;
}

void outputsLoop() {
  uint32_t now = millis();
  bool unread = anyUnread(0) || anyUnread(1);
  setLed(PIN_LED_G, lastRelayAt && now - lastRelayAt < 80);
  setLed(PIN_LED_B, unread && (now % 2000) < 120);
  if (beepStage && (int32_t)(now - beepAt) >= 0) {
    if (beepStage == 1) { ledcWriteTone(PIN_SPEAKER, 2093); beepStage = 2; beepAt = now + 90; }
    else { ledcWriteTone(PIN_SPEAKER, 0); digitalWrite(PIN_AMP_EN, HIGH); beepStage = 0; }
  }
  if (!asleep && SLEEP_MS[sleepIdx] && now - lastInput > SLEEP_MS[sleepIdx] && scr != SCR_SPLASH) {
    asleep = true;
    lcd.setBrightness(0);
  }
}

void uiLoop() {
  touchLoop();
  uint32_t now = millis();
  if (toast.on && (int32_t)(now - toast.until) > 0) { toast.on = false; needDraw = true; }
  if (netChanged) {
    netChanged = false;
    if (scr == SCR_THREAD) layoutDirty = true;
    if (scr == SCR_HOME || scr == SCR_THREAD) needDraw = true;
  }
  static uint32_t lastTick = 0;   // refresh Nearby / relay counts now and then
  if (scr == SCR_HOME && (tab == TAB_NEAR || tab == TAB_SET) && now - lastTick > 3000) { lastTick = now; needDraw = true; }
  if (needDraw && !asleep && now - lastDraw > 30) {
    needDraw = false;
    lastDraw = now;
    render();
  }
  outputsLoop();
}

void uiSetup() {
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  setLed(PIN_LED_R, false);
  setLed(PIN_LED_G, false);
  setLed(PIN_LED_B, false);
  pinMode(PIN_AMP_EN, OUTPUT);
  digitalWrite(PIN_AMP_EN, HIGH);
  ledcAttach(PIN_SPEAKER, 2000, 8);
  ledcWriteTone(PIN_SPEAKER, 0);

  lcd.init();
  lcd.setRotation(DISPLAY_ROTATION);
  lcd.setBrightness(BRIGHT[brightIdx]);
  initCharWidths();
  for (auto &s : strips) {
    s.setColorDepth(16);
    if (s.createSprite(240, STRIP_H)) nStrips++;
    else break;
  }
  useStrip = nStrips > 0;

  uint16_t cal[8];
  if (prefs.getBytesLength("cal") == sizeof cal) {
    prefs.getBytes("cal", cal, sizeof cal);
    lcd.setTouchCalibrate(cal);
  } else {
    lcd.fillScreen(COL_LAC);
    lcd.setFont(F_TITLE);
    lcd.setTextDatum(D_MC);
    lcd.setTextColor(COL_CHAMP);
    lcd.drawString("Welcome", 120, 130);
    lcd.setFont(F_SMALL);
    lcd.setTextColor(COL_MIST);
    lcd.drawString("Tap each arrow tip with the stylus", 120, 165);
    lcd.drawString("to set up the touch screen", 120, 183);
    lcd.calibrateTouch(cal, COL_GOLD, COL_LAC, 14);
    prefs.putBytes("cal", cal, sizeof cal);
  }

  scr = SCR_SPLASH;
  render();
  lastInput = millis();
}
