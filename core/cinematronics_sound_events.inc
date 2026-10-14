// license:BSD-3-Clause
// copyright-holders:Aaron Giles
// Adapted from official MAME mame0170/src/mame/audio/cinemat.cpp.
// Source SHA256: 3f2544cc5276adaf5c0406a4c733997889ec69886fe684683e645f284832ab45
// Generated event decoder; synthesis backend is new and approximate.
#define RISING_EDGE(bit, changed, val)  (((changed) & (bit)) && ((val) & (bit)))
#define FALLING_EDGE(bit, changed, val) (((changed) & (bit)) && !((val) & (bit)))
#define SOUNDVAL_RISING_EDGE(bit)       RISING_EDGE(bit, bits_changed, sound_val)
#define SOUNDVAL_FALLING_EDGE(bit)      FALLING_EDGE(bit, bits_changed, sound_val)
#define SHIFTREG_RISING_EDGE(bit)       RISING_EDGE(bit, (m_last_shift ^ m_current_shift), m_current_shift)
#define SHIFTREG_FALLING_EDGE(bit)      FALLING_EDGE(bit, (m_last_shift ^ m_current_shift), m_current_shift)
#define SHIFTREG2_RISING_EDGE(bit)      RISING_EDGE(bit, (m_last_shift2 ^ m_current_shift), m_current_shift)
#define SHIFTREG2_FALLING_EDGE(bit)     FALLING_EDGE(bit, (m_last_shift2 ^ m_current_shift), m_current_shift)
static const char *const spacewar_sample_names[] =
{
	"*spacewar",
	"explode1",
	"fire1",
	"idle",
	"thrust1",
	"thrust2",
	"pop",
	"explode2",
	"fire2",
	nullptr
};
void CinematronicsSound::spacewar_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* Explosion - rising edge */
	if (SOUNDVAL_RISING_EDGE(0x01))
		m_samples->start(0, (random() & 1) ? 0 : 6);

	/* Fire sound - rising edge */
	if (SOUNDVAL_RISING_EDGE(0x02))
		m_samples->start(1, (random() & 1) ? 1 : 7);

	/* Player 1 thrust - 0=on, 1=off */
	if (SOUNDVAL_FALLING_EDGE(0x04))
		m_samples->start(3, 3, true);
	if (SOUNDVAL_RISING_EDGE(0x04))
		m_samples->stop(3);

	/* Player 2 thrust - 0=on, 1-off */
	if (SOUNDVAL_FALLING_EDGE(0x08))
		m_samples->start(4, 4, true);
	if (SOUNDVAL_RISING_EDGE(0x08))
		m_samples->stop(4);

	/* Mute - 0=off, 1=on */
	if (SOUNDVAL_FALLING_EDGE(0x10))
		m_samples->start(2, 2, true); /* play idle sound */
	if (SOUNDVAL_RISING_EDGE(0x10))
	{
		int i;

		/* turn off all but the idle sound */
		for (i = 0; i < 5; i++)
			if (i != 2)
				m_samples->stop(i);

		/* Pop when board is shut off */
		m_samples->start(2, 5);
	}
}
static const char *const barrier_sample_names[] =
{
	"*barrier",
	"playrdie",
	"playmove",
	"enemmove",
	nullptr
};
void CinematronicsSound::barrier_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* Player die - rising edge */
	if (SOUNDVAL_RISING_EDGE(0x01))
		m_samples->start(0, 0);

	/* Player move - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x02))
		m_samples->start(1, 1);

	/* Enemy move - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x04))
		m_samples->start(2, 2);
}
static const char *const speedfrk_sample_names[] =
{
	"*speedfrk",
	"offroad",
	nullptr
};
void CinematronicsSound::speedfrk_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* on the falling edge of bit 0x08, clock the inverse of bit 0x04 into the top of the shiftreg */
	if (SOUNDVAL_FALLING_EDGE(0x08))
	{
		m_current_shift = ((m_current_shift >> 1) & 0x7fff) | ((static_cast<uint32_t>(~sound_val) & 4u) << 13);
		/* high 12 bits control the frequency - counts from value to $FFF, carry triggers */
		/* another counter */

		/* low 4 bits control the volume of the noise output (explosion?) */
	}

	/* off-road - 1=on, 0=off */
	if (SOUNDVAL_RISING_EDGE(0x10))
		m_samples->start(0, 0, true);
	if (SOUNDVAL_FALLING_EDGE(0x10))
		m_samples->stop(0);

	/* start LED is controlled by bit 0x02 */

}
static const char *const starhawk_sample_names[] =
{
	"*starhawk",
	"explode",
	"rlaser",
	"llaser",
	"k",
	"master",
	"kexit",
	nullptr
};
void CinematronicsSound::starhawk_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* explosion - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x01))
		m_samples->start(0, 0);

	/* right laser - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x02))
		m_samples->start(1, 1);

	/* left laser - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x04))
		m_samples->start(2, 2);

	/* K - 0=on, 1=off */
	if (SOUNDVAL_FALLING_EDGE(0x08))
		m_samples->start(3, 3, true);
	if (SOUNDVAL_RISING_EDGE(0x08))
		m_samples->stop(3);

	/* master - 0=on, 1=off */
	if (SOUNDVAL_FALLING_EDGE(0x10))
		m_samples->start(4, 4, true);
	if (SOUNDVAL_RISING_EDGE(0x10))
		m_samples->stop(4);

	/* K exit - 1=on, 0=off */
	if (SOUNDVAL_RISING_EDGE(0x80))
		m_samples->start(3, 5, true);
	if (SOUNDVAL_FALLING_EDGE(0x80))
		m_samples->stop(3);
}
static const char *const sundance_sample_names[] =
{
	"*sundance",
	"bong",
	"whoosh",
	"explsion",
	"ping1",
	"ping2",
	"hatch",
	nullptr
};
void CinematronicsSound::sundance_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* bong - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x01))
		m_samples->start(0, 0);

	/* whoosh - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x02))
		m_samples->start(1, 1);

	/* explosion - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x04))
		m_samples->start(2, 2);

	/* ping - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x08))
		m_samples->start(3, 3);

	/* ping - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x10))
		m_samples->start(4, 4);

	/* hatch - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x80))
		m_samples->start(5, 5);
}
static const char *const tailg_sample_names[] =
{
	"*tailg",
	"sexplode",
	"thrust1",
	"slaser",
	"shield",
	"bounce",
	"hypersp",
	nullptr
};
void CinematronicsSound::tailg_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* the falling edge of bit 0x10 clocks bit 0x08 into the mux selected by bits 0x07 */
	if (SOUNDVAL_FALLING_EDGE(0x10))
	{
		/* update the shift register (actually just a simple mux) */
		m_current_shift = (m_current_shift & ~(1 << (sound_val & 7))) | (((sound_val >> 3) & 1) << (sound_val & 7));

		/* explosion - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x01))
			m_samples->start(0, 0);

		/* rumble - 0=on, 1=off */
		if (SHIFTREG_FALLING_EDGE(0x02))
			m_samples->start(1, 1, true);
		if (SHIFTREG_RISING_EDGE(0x02))
			m_samples->stop(1);

		/* laser - 0=on, 1=off */
		if (SHIFTREG_FALLING_EDGE(0x04))
			m_samples->start(2, 2, true);
		if (SHIFTREG_RISING_EDGE(0x04))
			m_samples->stop(2);

		/* shield - 0=on, 1=off */
		if (SHIFTREG_FALLING_EDGE(0x08))
			m_samples->start(3, 3, true);
		if (SHIFTREG_RISING_EDGE(0x08))
			m_samples->stop(3);

		/* bounce - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x10))
			m_samples->start(4, 4);

		/* hyperspace - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x20))
			m_samples->start(5, 5);

		/* LED */


		/* remember the previous value */
		m_last_shift = m_current_shift;
	}
}
static const char *const armora_sample_names[] =
{
	"*armora",
	"loexp",
	"jeepfire",
	"hiexp",
	"tankfire",
	"tankeng",
	"beep",
	"chopper",
	nullptr
};
void CinematronicsSound::armora_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* on the rising edge of bit 0x10, clock bit 0x80 into the shift register */
	if (SOUNDVAL_RISING_EDGE(0x10))
		m_current_shift = ((m_current_shift >> 1) & 0x7f) | (sound_val & 0x80);

	/* execute on the rising edge of bit 0x01 */
	if (SOUNDVAL_RISING_EDGE(0x01))
	{
		/* bits 0-4 control the tank sound speed */

		/* lo explosion - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x10))
			m_samples->start(0, 0);

		/* jeep fire - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x20))
			m_samples->start(1, 1);

		/* hi explosion - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x40))
			m_samples->start(2, 2);

		/* tank fire - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x80))
			m_samples->start(3, 3);

		/* remember the previous value */
		m_last_shift = m_current_shift;
	}

	/* tank sound - 0=on, 1=off */
	/* still not totally correct - should be multiple speeds based on remaining bits in shift reg */
	if (SOUNDVAL_FALLING_EDGE(0x02))
		m_samples->start(4, 4, true);
	if (SOUNDVAL_RISING_EDGE(0x02))
		m_samples->stop(4);

	/* beep sound - 0=on, 1=off */
	if (SOUNDVAL_FALLING_EDGE(0x04))
		m_samples->start(5, 5, true);
	if (SOUNDVAL_RISING_EDGE(0x04))
		m_samples->stop(5);

	/* chopper sound - 0=on, 1=off */
	if (SOUNDVAL_FALLING_EDGE(0x08))
		m_samples->start(6, 6, true);
	if (SOUNDVAL_RISING_EDGE(0x08))
		m_samples->stop(6);
}
static const char *const ripoff_sample_names[] =
{
	"*ripoff",
	"bonuslvl",
	"eattack",
	"shipfire",
	"efire",
	"explosn",
	"bg1",
	"bg2",
	"bg3",
	"bg4",
	"bg5",
	"bg6",
	"bg7",
	"bg8",
	nullptr
};
void CinematronicsSound::ripoff_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* on the rising edge of bit 0x02, clock bit 0x01 into the shift register */
	if (SOUNDVAL_RISING_EDGE(0x02))
		m_current_shift = ((m_current_shift >> 1) & 0x7f) | ((sound_val << 7) & 0x80);

	/* execute on the rising edge of bit 0x04 */
	if (SOUNDVAL_RISING_EDGE(0x04))
	{
		/* background - 0=on, 1=off, selected by bits 0x38 */
		if ((((m_current_shift ^ m_last_shift) & 0x38) && !(m_current_shift & 0x04)) || SHIFTREG_FALLING_EDGE(0x04))
			m_samples->start(5, 5 + ((m_current_shift >> 5) & 7), true);
		if (SHIFTREG_RISING_EDGE(0x04))
			m_samples->stop(5);

		/* beep - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x02))
			m_samples->start(0, 0);

		/* motor - 0=on, 1=off */
		if (SHIFTREG_FALLING_EDGE(0x01))
			m_samples->start(1, 1, true);
		if (SHIFTREG_RISING_EDGE(0x01))
			m_samples->stop(1);

		/* remember the previous value */
		m_last_shift = m_current_shift;
	}

	/* torpedo - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x08))
		m_samples->start(2, 2);

	/* laser - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x10))
		m_samples->start(3, 3);

	/* explosion - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x80))
		m_samples->start(4, 4);
}
static const char *const starcas_sample_names[] =
{
	"*starcas",
	"cfire",
	"shield",
	"star",
	"thrust",
	"drone",
	"lexplode",
	"sexplode",
	"pfire",
	nullptr
};
void CinematronicsSound::starcas_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	UINT32 target_pitch;

	/* on the rising edge of bit 0x10, clock bit 0x80 into the shift register */
	if (SOUNDVAL_RISING_EDGE(0x10))
		m_current_shift = ((m_current_shift >> 1) & 0x7f) | (sound_val & 0x80);

	/* execute on the rising edge of bit 0x01 */
	if (SOUNDVAL_RISING_EDGE(0x01))
	{
		/* fireball - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x80))
			m_samples->start(0, 0);

		/* shield hit - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x40))
			m_samples->start(1, 1);

		/* star sound - 0=off, 1=on */
		if (SHIFTREG_RISING_EDGE(0x20))
			m_samples->start(2, 2, true);
		if (SHIFTREG_FALLING_EDGE(0x20))
			m_samples->stop(2);

		/* thrust sound - 1=off, 0=on*/
		if (SHIFTREG_FALLING_EDGE(0x10))
			m_samples->start(3, 3, true);
		if (SHIFTREG_RISING_EDGE(0x10))
			m_samples->stop(3);

		/* drone - 1=off, 0=on */
		if (SHIFTREG_FALLING_EDGE(0x08))
			m_samples->start(4, 4, true);
		if (SHIFTREG_RISING_EDGE(0x08))
			m_samples->stop(4);

		/* latch the drone pitch */
		target_pitch = (m_current_shift & 7) + ((m_current_shift & 2) << 2);
		target_pitch = 0x5800 + (target_pitch << 12);

		/* once per frame slide the pitch toward the target */
		if (frame_ > m_last_frame)
		{
			if (m_current_pitch > target_pitch)
				m_current_pitch -= 225;
			if (m_current_pitch < target_pitch)
				m_current_pitch += 150;
			m_samples->set_frequency(4, m_current_pitch);
			m_last_frame = frame_;
		}

		/* remember the previous value */
		m_last_shift = m_current_shift;
	}

	/* loud explosion - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x02))
		m_samples->start(5, 5);

	/* soft explosion - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x04))
		m_samples->start(6, 6);

	/* player fire - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x08))
		m_samples->start(7, 7);
}
static const char *const solarq_sample_names[] =
{
	"*solarq",
	"bigexpl",
	"smexpl",
	"lthrust",
	"slaser",
	"pickup",
	"nuke2",
	"nuke1",
	"music",
	nullptr
};
void CinematronicsSound::solarq_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* on the rising edge of bit 0x10, clock bit 0x80 into the shift register */
	if (SOUNDVAL_RISING_EDGE(0x10))
		m_current_shift = ((m_current_shift >> 1) & 0x7fff) | ((sound_val << 8) & 0x8000);

	/* execute on the rising edge of bit 0x02 */
	if (SOUNDVAL_RISING_EDGE(0x02))
	{
		/* only the upper 8 bits matter */
		m_current_shift >>= 8;

		/* loud explosion - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x80))
			m_samples->start(0, 0);

		/* soft explosion - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x40))
			m_samples->start(1, 1);

		/* thrust - 0=on, 1=off */
		if (SHIFTREG_FALLING_EDGE(0x20))
		{
			m_target_volume = 1.0;
			if (!m_samples->playing(2))
				m_samples->start(2, 2, true);
		}
		if (SHIFTREG_RISING_EDGE(0x20))
			m_target_volume = 0;

		/* ramp the thrust volume */
		if (m_samples->playing(2) && frame_ > m_last_frame)
		{
			if (m_current_volume > m_target_volume)
				m_current_volume -= 0.078f;
			if (m_current_volume < m_target_volume)
				m_current_volume += 0.078f;
			if (m_current_volume > 0)
				m_samples->set_volume(2, m_current_volume);
			else
				m_samples->stop(2);
			m_last_frame = frame_;
		}

		/* fire - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x10))
			m_samples->start(3, 3);

		/* capture - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x08))
			m_samples->start(4, 4);

		/* nuke - 1=on, 0=off */
		if (SHIFTREG_RISING_EDGE(0x04))
			m_samples->start(5, 5, true);
		if (SHIFTREG_FALLING_EDGE(0x04))
			m_samples->stop(5);

		/* photon - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x02))
			m_samples->start(6, 6);

		/* remember the previous value */
		m_last_shift = m_current_shift;
	}

	/* clock music data on the rising edge of bit 0x01 */
	if (SOUNDVAL_RISING_EDGE(0x01))
	{
		int freq, vol;

		/* start/stop the music sample on the high bit */
		if (SHIFTREG2_RISING_EDGE(0x8000))
			m_samples->start(7, 7, true);
		if (SHIFTREG2_FALLING_EDGE(0x8000))
			m_samples->stop(7);

		/* set the frequency */
		freq = static_cast<int>(56818.181818 / (4096 - (m_current_shift & 0xfff)));
		m_samples->set_frequency(7, 44100.0 * freq / 1050.0);

		/* set the volume */
		vol = (~m_current_shift >> 12) & 7;
		m_samples->set_volume(7, vol / 7.0);

		/* remember the previous value */
		m_last_shift2 = m_current_shift;
	}
}
static const char *const boxingb_sample_names[] =
{
	"*boxingb",
	"softexpl",
	"loudexpl",
	"chirp",
	"eggcrack",
	"bugpusha",
	"bugpushb",
	"bugdie",
	"beetle",
	"music",
	"cannon",
	"bounce",
	"bell",
	nullptr
};
void CinematronicsSound::boxingb_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* on the rising edge of bit 0x10, clock bit 0x80 into the shift register */
	if (SOUNDVAL_RISING_EDGE(0x10))
		m_current_shift = ((m_current_shift >> 1) & 0x7fff) | ((sound_val << 8) & 0x8000);

	/* execute on the rising edge of bit 0x02 */
	if (SOUNDVAL_RISING_EDGE(0x02))
	{
		/* only the upper 8 bits matter */
		m_current_shift >>= 8;

		/* soft explosion - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x80))
			m_samples->start(0, 0);

		/* loud explosion - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x40))
			m_samples->start(1, 1);

		/* chirping birds - 0=on, 1=off */
		if (SHIFTREG_FALLING_EDGE(0x20))
			m_samples->start(2, 2);
		if (SHIFTREG_RISING_EDGE(0x20))
			m_samples->stop(2);

		/* egg cracking - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x10))
			m_samples->start(3, 3);

		/* bug pushing A - rising edge */
		if (SHIFTREG_RISING_EDGE(0x08))
			m_samples->start(4, 4);

		/* bug pushing B - rising edge */
		if (SHIFTREG_RISING_EDGE(0x04))
			m_samples->start(5, 5);

		/* bug dying - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x02))
			m_samples->start(6, 6);

		/* beetle on screen - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x01))
			m_samples->start(7, 7);

		/* remember the previous value */
		m_last_shift = m_current_shift;
	}

	/* clock music data on the rising edge of bit 0x01 */
	if (SOUNDVAL_RISING_EDGE(0x01))
	{
		int freq, vol;

		/* start/stop the music sample on the high bit */
		if (SHIFTREG2_RISING_EDGE(0x8000))
			m_samples->start(8, 8, true);
		if (SHIFTREG2_FALLING_EDGE(0x8000))
			m_samples->stop(8);

		/* set the frequency */
		freq = static_cast<int>(56818.181818 / (4096 - (m_current_shift & 0xfff)));
		m_samples->set_frequency(8, 44100.0 * freq / 1050.0);

		/* set the volume */
		vol = (~m_current_shift >> 12) & 3;
		m_samples->set_volume(8, vol / 3.0);

		/* cannon - falling edge */
		if (SHIFTREG2_RISING_EDGE(0x4000))
			m_samples->start(9, 9);

		/* remember the previous value */
		m_last_shift2 = m_current_shift;
	}

	/* bounce - rising edge */
	if (SOUNDVAL_RISING_EDGE(0x04))
		m_samples->start(10, 10);

	/* bell - falling edge */
	if (SOUNDVAL_RISING_EDGE(0x08))
		m_samples->start(11, 11);
}
static const char *const wotw_sample_names[] =
{
	"*wotw",
	"cfire",
	"shield",
	"star",
	"thrust",
	"drone",
	"lexplode",
	"sexplode",
	"pfire",
	nullptr
};
void CinematronicsSound::wotw_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	UINT32 target_pitch;

	/* on the rising edge of bit 0x10, clock bit 0x80 into the shift register */
	if (SOUNDVAL_RISING_EDGE(0x10))
		m_current_shift = ((m_current_shift >> 1) & 0x7f) | (sound_val & 0x80);

	/* execute on the rising edge of bit 0x01 */
	if (SOUNDVAL_RISING_EDGE(0x01))
	{
		/* fireball - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x80))
			m_samples->start(0, 0);

		/* shield hit - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x40))
			m_samples->start(1, 1);

		/* star sound - 0=off, 1=on */
		if (SHIFTREG_RISING_EDGE(0x20))
			m_samples->start(2, 2, true);
		if (SHIFTREG_FALLING_EDGE(0x20))
			m_samples->stop(2);

		/* thrust sound - 1=off, 0=on*/
		if (SHIFTREG_FALLING_EDGE(0x10))
			m_samples->start(3, 3, true);
		if (SHIFTREG_RISING_EDGE(0x10))
			m_samples->stop(3);

		/* drone - 1=off, 0=on */
		if (SHIFTREG_FALLING_EDGE(0x08))
			m_samples->start(4, 4, true);
		if (SHIFTREG_RISING_EDGE(0x08))
			m_samples->stop(4);

		/* latch the drone pitch */
		target_pitch = (m_current_shift & 7) + ((m_current_shift & 2) << 2);
		target_pitch = 0x10000 + (target_pitch << 12);

		/* once per frame slide the pitch toward the target */
		if (frame_ > m_last_frame)
		{
			if (m_current_pitch > target_pitch)
				m_current_pitch -= 300;
			if (m_current_pitch < target_pitch)
				m_current_pitch += 200;
			m_samples->set_frequency(4, m_current_pitch);
			m_last_frame = frame_;
		}

		/* remember the previous value */
		m_last_shift = m_current_shift;
	}

	/* loud explosion - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x02))
		m_samples->start(5, 5);

	/* soft explosion - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x04))
		m_samples->start(6, 6);

	/* player fire - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x08))
		m_samples->start(7, 7);
}
static const char *const *effect_names(Machine machine) { switch(machine) {
case Machine::spacewar: return spacewar_sample_names;
case Machine::barrier: return barrier_sample_names;
case Machine::speedfrk: return speedfrk_sample_names;
case Machine::starhawk: return starhawk_sample_names;
case Machine::sundance: return sundance_sample_names;
case Machine::tailg: return tailg_sample_names;
case Machine::armora: return armora_sample_names;
case Machine::ripoff: return ripoff_sample_names;
case Machine::starcas: return starcas_sample_names;
case Machine::solarq: return solarq_sample_names;
case Machine::boxingb: return boxingb_sample_names;
case Machine::wotw: return wotw_sample_names;
default: return nullptr; }}
