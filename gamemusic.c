#include "gamemusic.h"
#include <c64/sid.h>

// Memory section for the music data from 0x9000 to 0xbfff (12k)
#pragma section( music, 0)

#pragma region( music, 0x9000, 0xc000, , , {music} )

#pragma data(music)

__export char music[] = {
	#embed 0x3000 0x7e "BattleTanks-7.2.sid" 
};

#pragma data(data)

// Throttle counter to slow music down by 1/6th in case of NTSC, keeps
// counting from 0..5 and skips a beat when wrapping
char		music_throttle;

// Init the SID code in the included music with the given tune
void music_init(Tune tune)
{
	__asm
	{
		lda		tune
		jsr		$9000
	}
}

void music_queue(Tune tune)
{
}

// Fade the music slowly out
void music_fade(void)
{
	for(char i=0; i<15; i+=5)
	{
		sid.fmodevol = 15 - i;
		vic_waitFrame();
	}

}

// Play the music called each interrupt (50 or 60 times per second)
void music_play(void)
{
	// Throttle music by one sixth in NTSC systems
	if (system_ntsc)
	{
		music_throttle++;
		if (music_throttle == 6)
		{
			music_throttle = 0;
			return;
		}
	}

	// Call the playback routine
	if (music_active)
	{
		__asm
		{
			jsr		$9003
		}
	}
}

// Patch the SID to either two or three voices for intro or in game music
void music_patch_voice3(bool enable)
{
	*(char *)0x916a = enable ? 0x20 : 0x4c;
}

void music_toggle(void)
{
}
