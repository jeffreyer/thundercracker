#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

// Constants
#define NUM_CUBES 			3
#define LOAD_ASSETS			1
#define NUM_ICONS 			4
#define NUM_TIPS			3
#define ELEMENT_PADDING 	24.f
#define ICON_WIDTH      	80.f
#define MAX_NORMAL_SPEED 	40.f
#define ACCEL_SCALING_FACTOR -0.25f

// computed/hardware constants
#define NUM_TILES_X			18
#define NUM_VISIBLE_TILES_X (NUM_TILES_X - 2)
#define NUM_TILES_Y			18
#define NUM_VISIBLE_TILES_Y (NUM_TILES_Y - 2)
#define PIXELS_PER_ICON		(ELEMENT_PADDING + ICON_WIDTH)
#define COLUMNS_PER_ICON	((int)(PIXELS_PER_ICON / 8)) // columns taken up by the icon plus padding
#define ONE_G				fabs(64 * ACCEL_SCALING_FACTOR)

//typedef VidMode_BG0 Canvas;
typedef VidMode_BG0 Canvas;

// Static Globals
static Cube gCubes[NUM_CUBES];
static const AssetImage* gIcons[NUM_ICONS] = { &IconChroma, &IconSandwich, &IconPeano, &IconBuddy };
static const AssetImage* gTips[NUM_TIPS] = { &Tip0, &Tip1, &Tip2 };
static const AssetImage* gLabels[NUM_ICONS] = { &LabelChroma, &LabelSandwich, &LabelPeano, &LabelBuddy };

// Modulo operator that always returns an unsigned result
inline unsigned UnsignedMod(int x, unsigned y) { const int z = x % (int)y; return z < 0 ? z+y : z; }
inline float Lerp(float min, float max, float u) { return min + u * (max - min); }
inline int ComputeSelected(float u) { int s = (u + (PIXELS_PER_ICON / 2.f))/(ICON_WIDTH + ELEMENT_PADDING); return clamp(s, 0, NUM_ICONS-1); }
inline float StoppingPositionFor(int selected) { return (ICON_WIDTH + ELEMENT_PADDING) * (selected); }

// positions are in pixel units
// columns are in tile-units
// each icon is 80px/10tl wide with a 16px/2tl spacing
static void DrawColumn(Cube* pCube, int x) {
	// x is the column in "global" space
    x -= 3; // because the first icon is 24px inset
    uint16_t addr = UnsignedMod(x, NUM_TILES_X);
	const uint16_t local_x = UnsignedMod(x, COLUMNS_PER_ICON);
	const int iconId = x < 0 ? -1 : x / COLUMNS_PER_ICON;
	// icon or blank column?
	if (local_x < 10 && iconId >= 0 && iconId < NUM_ICONS) {
		// drawing an icon column
		const AssetImage* pImg = gIcons[x / COLUMNS_PER_ICON];
		const uint16_t *src = pImg->tiles + UnsignedMod(local_x, pImg->width);
		addr += 2*NUM_TILES_X;
		for(int row=0; row<10; ++row) {
	        _SYS_vbuf_writei(&pCube->vbuf.sys, addr, src, 0, 1);
    	    addr += NUM_TILES_X;
        	src += pImg->width;

		}
	} else {
		// drawing a blank column
		Canvas g(pCube->vbuf);
		for(int row=0; row<10; ++row) {
			g.BG0_drawAsset(Vec2(addr, row+2), BgTile);
		}

	}
}

// wrapper for paint() that updates the footer
static int gCurrentTip = 0;
static float gPrevTime;
static void Paint(Cube *pCube) {
	float time = System::clock();
	float dt = time - gPrevTime;
	if (dt > 4.f) {
		gPrevTime = time - fmodf(dt, 4.f);
		const AssetImage& tip = *gTips[gCurrentTip];
        _SYS_vbuf_writei(
        	&pCube->vbuf.sys, 
        	offsetof(_SYSVideoRAM, bg1_tiles) / 2 + LabelEmpty.width * LabelEmpty.height,
            tip.tiles, 
            0, 
            tip.width * tip.height
        );
		gCurrentTip = (gCurrentTip+1) % NUM_TIPS;
	}
	System::paint();
}

// retrieve the acceleration of the cube due to tilting
const float kAccelThresholdOn = 1.15f;
const float kAccelThresholdOff = 0.85f;
static float GetAccel(Cube *pCube) {
	return ACCEL_SCALING_FACTOR * pCube->virtualAccel().x;
}

// entry point
void siftmain() {
	// enable cube slots
	for (Cube *p = gCubes; p!=gCubes+NUM_CUBES; ++p) { p->enable(p-gCubes); }
  	// load assets
	#if LOAD_ASSETS
		for(Cube *p = gCubes; p!=gCubes+NUM_CUBES; ++p) {
			p->loadAssets(BetterflowAssets);
			VidMode_BG0_ROM rom(p->vbuf);
			rom.init();
			rom.BG0_text(Vec2(1,1), "Loading...");
		}
		bool done = false;
		while(!done) {
			done = true;
			for(Cube *p = gCubes; p!=gCubes+NUM_CUBES; ++p) {
				VidMode_BG0_ROM rom(p->vbuf);
				rom.BG0_progressBar(Vec2(0,7), p->assetProgress(BetterflowAssets, VidMode_BG0::LCD_width), 2);
				done &= p->assetDone(BetterflowAssets);
			}
			System::paint();
		}
	#endif
	// blank screens
	for(Cube* p=gCubes; p!=gCubes+NUM_CUBES; ++p) {
		Canvas g(p->vbuf);
		g.set();
		g.clear();
		for(unsigned r=0; r<18; ++r)
		for(unsigned c=0; c<18; ++c) {
			g.BG0_drawAsset(Vec2(c,r), BgTile);
		}

	}
	// sync up
	System::paintSync();
	for(Cube* p=gCubes; p!=gCubes+NUM_CUBES; ++p) { p->vbuf.touch(); }
	System::paintSync();
	for(Cube* p=gCubes; p!=gCubes+NUM_CUBES; ++p) { p->vbuf.touch(); }
	System::paintSync();
	// initialize view
	Cube *pCube = gCubes;
	_SYS_vbuf_pokeb(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_BG1);
	Canvas canvas(pCube->vbuf);
	for(;;) {
	// HACK ALERT: Relies on the fact that vram is the same for both modes
	canvas.clear();
	VidMode_BG0_SPR_BG1(pCube->vbuf).BG1_setPanning(Vec2(0, 0));
	BG1Helper(*pCube).Flush();
    // Allocate tiles for the static upper label, and draw it.
    {
    	const AssetImage& label = *gLabels[0];
    	_SYS_vbuf_fill(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2, ((1 << label.width) - 1), label.height);
    	_SYS_vbuf_writei(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2, label.tiles, 0, label.width * label.height);
	}
	// Allocate tiles for the footer, and draw it.
    _SYS_vbuf_fill(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2 + (NUM_VISIBLE_TILES_Y - Tip0.height), ((1 << Tip0.width) - 1), Tip0.height);
    _SYS_vbuf_writei(
    	&pCube->vbuf.sys, 
    	offsetof(_SYSVideoRAM, bg1_tiles) / 2 + LabelEmpty.width * LabelEmpty.height,
        Tip0.tiles, 
        0, 
        Tip0.width * Tip0.height
    );
    for (int x = -1; x < NUM_TILES_X - 1; x++) { DrawColumn(pCube, x); }
    Paint(pCube);
    // initialize physics
    float position = 0;
	int prev_ut = 0;
	int prev_ui = 0;
	gPrevTime = System::clock();
	for(;;) {
		// wait for a tilt or touch
		bool prevTouch = pCube->touching();
		while(fabs(GetAccel(pCube)) < kAccelThresholdOn) {
			Paint(pCube);
			bool touch = pCube->touching();
			// when touching any icon but the last one
			if (ComputeSelected(position) != NUM_ICONS-1 && touch && !prevTouch) {
				goto Selected;
			} else {
				prevTouch = touch;
			}

		}
		// hide label
	    _SYS_vbuf_writei(
	    	&pCube->vbuf.sys, 
	    	offsetof(_SYSVideoRAM, bg1_tiles) / 2, 
	    	LabelEmpty.tiles, 0, 
	    	LabelEmpty.width * 
	    	LabelEmpty.height
	    );
		bool doneTilting = false;
		float velocity = 0;
		float lastPaint = System::clock();
		position = StoppingPositionFor(ComputeSelected(position));
		while(!doneTilting) {
			float now = System::clock();
			// update physics
			const float accel = GetAccel(pCube);
			const bool isTilting = fabs(accel) > kAccelThresholdOff;
			float dt = (now - lastPaint) * 13.1f;
			const bool isLefty = position < 0.f - 0.05f;
			const bool isRighty = position > PIXELS_PER_ICON*(NUM_ICONS-1) + 0.05f;
			if (isTilting && !isLefty && !isRighty) {
				velocity += accel * dt;
				
				// clamp maximum velocity based on cube angle
				if(fabs(velocity) > MAX_NORMAL_SPEED * (fabs(accel) / ONE_G)) {
					velocity = (velocity < 0 ? 0 - MAX_NORMAL_SPEED : MAX_NORMAL_SPEED) * (fabs(accel) / ONE_G);
				}
				position += velocity * dt;
			} else {
				const float stiffness = 0.333f;
				const int selected = ComputeSelected(position);
				const float stopping_position = StoppingPositionFor(selected);
				velocity += stiffness * (stopping_position - position) * dt;
				velocity *= 0.875f;
				position += velocity * dt;
				position = Lerp(position, stopping_position, 0.15f);
				doneTilting = fabs(velocity) < 1.0f && fabs(stopping_position - position) < 0.5f;
			}
			const float pad = 24.f;
			// update view
			int ui = position + 0.5f;
			int ut = position / 8;
			// TODO: Sometimes, on really fast accelerations, there's a column
			// 		of garbage tiles on the side of the screen; is this a firmware
			//		bug or a logic bug here?
			// numist: It's an application logic bug triggered by not being aware
			//         of the underlying organization of the hardware.
			//         To detect, count pixels moved since the last paint call, and
			//         if it's larger than a tile, switch to paintSync so the new
			//         column is guaranteed to arrive before the render.
			//         Actual number to check should be just below a tile to avoid
			//         getting caught by the PLL.
			while(prev_ut < ut) {
				DrawColumn(pCube, prev_ut + 17);
				prev_ut++;
			}
			while(prev_ut > ut) {
				DrawColumn(pCube, prev_ut - 2);
				prev_ut--;
			}
			canvas.BG0_setPanning(Vec2(ui, 0));
			if(abs(prev_ui - ui) > 7) {
				LOG(("pixel delta: %d, sync\n", prev_ui - ui));
				System::paintSync();
			} else {
				Paint(pCube);
			}
			prev_ui = ui;
			lastPaint = now;
		}
		{
			// show the title of the game
			const AssetImage& label = *gLabels[ComputeSelected(position)];
		    _SYS_vbuf_writei(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2, label.tiles, 0, label.width * label.height);
		}
	}
	Selected:
	// isolate the selected icon
	canvas.BG0_setPanning(Vec2(0,0));
	// kinda sub-optimal :P
	for(int row=0; row<12; ++row)
	for(int col=0; col<16; ++col) {
		canvas.BG0_drawAsset(Vec2(col, row), BgTile);
	}
	canvas.BG0_drawAsset(Vec2(0, NUM_VISIBLE_TILES_Y - Footer.height), Footer);
	// TODO: Phase-Out BG1Helper code with optimized code
	{
		const AssetImage* icon = gIcons[ComputeSelected(position)];
		BG1Helper overlay(*pCube);
		overlay.DrawAsset(Vec2((NUM_VISIBLE_TILES_X - icon->width) / 2, 2), *icon);
		overlay.Flush();
	}
	System::paintSync();
	// GFX Workaround
	pCube->vbuf.touch();
	System::paintSync();

	// scroll-out icon with a parabolic arc
	const float k = 5.f;
	int i=0;
	int prevBottom = 12;
	int offset = 0;
	while(offset>-128) {
		i++;
		float u = i/33.f;
		u = (1.f-k*u);
		offset = int(12*(1.f-u*u));
		// HACK ALERT: Relies on the fact that vram is the same for both modes
		VidMode_BG0_SPR_BG1(pCube->vbuf).BG1_setPanning(Vec2(0, offset));
		System::paint();
	}
	// TODO: actually choose game
	LOG(("Selected Game: %d\n", ComputeSelected(position)));
	for(int i=0; i<32; ++i) {
		System::paint();
	}
	}
}
