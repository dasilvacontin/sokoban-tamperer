#include <stdio.h>
#include <tonc.h>
#include <ctype.h>
#include "all_gfx.h"

#define MAP_WIDTH (SCREEN_WIDTH / 8)
#define MAP_HEIGHT (SCREEN_HEIGHT / 8)

#define IN_FLOOR 36
#define OUTFLOOR 37
#define WALL_TOP 38
#define WALLSIDE 39
#define GOALTILE 43
#define INITTILE 44

#define CONSOLE_BG 40
#define CONSOLE_BG_DARK 41
#define CONSOLE_HIGHLIGHT 42

#define EMPTY 45

SCR_ENTRY bg3[32][32];

#define SBperCB 8
#define lower4BitsMask 0xF

inline int tile_id_for_char(char c) {
  return 10 + (toupper(c) - 'A');
}

int hold_delay = 0;
bool key_effective_held(u32 key) {
  return hold_delay >= 16 && key_held(key);
}

// 0x 1010 1111 0100 0001
// 0x 10.A 15.F 4    1
void render_data(SCR_ENTRY *pos, SCR_ENTRY data) {
  pos[0] = (data >> 12) & lower4BitsMask;
  pos[1] = (data >> 8) & lower4BitsMask;
  pos[2] = (data >> 4) & lower4BitsMask;
  pos[3] = data & lower4BitsMask;
}

void render_hex(SCR_ENTRY *pos, SCR_ENTRY data) {
  pos[0] = 0;
  pos[1] = tile_id_for_char('X');
  render_data(pos + 2, data);
}

int main()
{
	// Init interrupts and VBlank irq.
	irq_init(NULL);
	irq_add(II_VBLANK, NULL);

  for (int i = 0; i < MAP_HEIGHT; ++i) {
    for (int j = 0; j < MAP_WIDTH; ++j) {
      se_mem[0 * SBperCB + 0][j + 32 * i] = (MAP_WIDTH - j <= 9)
      ? (i == 0 ? CONSOLE_BG : CONSOLE_BG_DARK)
      : OUTFLOOR;
    }
  }

  memset(&se_mem[3 * SBperCB + 2][0], EMPTY, sizeof(bg3));

	// Video mode 0, enable bg 0.
  // bitmap BG2 MODE(3),4,5 (3: colors 16bits (5.5.5), 4: colors 8bits con pageflipping, 5: pageflipping)
  // tilemade BG0123 MODE(0),1,2 (0: 4 layers/bgs, 1: 2 normales y 1 affines, 2: 2 afines)
  // 16 * 2^5 * 2^5 * 8
  // DCNT_BG# actives BG#
	REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG3;
  REG_BG0CNT = BG_SBB(0 * SBperCB + 0) | BG_CBB(3) | BG_8BPP | BG_PRIO(3);
  REG_BG1CNT = BG_8BPP | BG_PRIO(1);
  REG_BG2CNT = BG_8BPP | BG_PRIO(2);
  REG_BG3CNT = BG_SBB(3 * SBperCB + 2) | BG_CBB(3) | BG_8BPP | BG_PRIO(0); // text layer

  // tile8_mem[CB][]
  // se_mem[SB][]
  // pal_bg_mem / pal_obj_mem
  memcpy(&tile8_mem[3][0], fontTiles, fontTilesLen);
  memcpy(pal_bg_mem, fontPal, fontPalLen);
  // memcpy(&se_mem[3 * SBperCB + 2][0], bg3, sizeof(bg3));

  SCR_ENTRY *base_address = &se_mem[0][0];
  SCR_ENTRY *selection_address = base_address;

	while(1) {
		VBlankIntrWait();
    key_poll();

    bool held_is_effective = hold_delay >= 16;

    if (key_held(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_L | KEY_R | KEY_A | KEY_B)) ++hold_delay;
    if (key_hit(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_L | KEY_R | KEY_A | KEY_B)) hold_delay = 0;

    if (key_hit(KEY_DOWN) || key_effective_held(KEY_DOWN)) {
      selection_address++;
      if (selection_address - base_address == 16) base_address = selection_address;
    } else if (key_hit(KEY_UP) || key_effective_held(KEY_UP)) {
      selection_address--;
      if (selection_address < base_address) base_address -= 16;
    }
    if (key_hit(KEY_LEFT) || key_effective_held(KEY_LEFT)) {
      base_address -= 16;
      selection_address -= 16;
    } else if (key_hit(KEY_RIGHT) || key_effective_held(KEY_RIGHT)) {
      base_address += 16;
      selection_address += 16;
    }

    if (key_hit(KEY_L) || key_effective_held(KEY_L)) {
      base_address -= 16 * 16;
      selection_address -= 16 * 16;
    } else if (key_hit(KEY_R) || key_effective_held(KEY_R)) {
      base_address += 16 * 16;
      selection_address += 16 * 16;
    }

    if (key_hit(KEY_A) || key_effective_held(KEY_A)) *selection_address += 1;
    else if (key_hit(KEY_B) || key_effective_held(KEY_B)) *selection_address -= 1;

    SCR_ENTRY *pos = &se_mem[3 * SBperCB + 2][MAP_WIDTH - 9];
    render_hex(pos + 3, ((uint) base_address) >> 16);

    for (int i = 0; i < 16; ++i) {
      pos += 32;
      SCR_ENTRY *current_address = base_address + i;
      memset16(
        &se_mem[0 * SBperCB + 0][(MAP_WIDTH - 9) + 32 * (i + 1)],
        ((current_address == selection_address) ? CONSOLE_HIGHLIGHT : CONSOLE_BG_DARK),
        9
      );
      render_data(pos, (uint) *current_address);
      render_data(pos + 5, (uint) current_address);
    }
	}

	return 0;
}
