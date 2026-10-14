"""Extract BSD sound event protocols, leaving MAME devices and sample data out."""
from pathlib import Path
import hashlib
import re
root=Path(__file__).resolve().parents[1]
p=root/'deps/mame-reference/src/mame/audio/cinemat.cpp'
s=p.read_text(encoding='utf-8')
names=['spacewar','barrier','speedfrk','starhawk','sundance','tailg','armora','ripoff','starcas','solarq','boxingb','wotw']
out=['// license:BSD-3-Clause', '// copyright-holders:Aaron Giles',
     '// Adapted from official MAME mame0170/src/mame/audio/cinemat.cpp.',
     '// Source SHA256: '+hashlib.sha256(p.read_bytes()).hexdigest(),
     '// Generated event decoder; synthesis backend is new and approximate.']
out+=re.findall(r'^#define (?:RISING_EDGE|FALLING_EDGE|SOUNDVAL_\w+|SHIFTREG\w*)[^\n]*',s,re.M)
for name in names:
    table=re.search(r'static const char \*const '+name+r'_sample_names\[\] =\s*\{.*?\n\};',s,re.S)[0]
    out.append(table)
    fn=re.search(r'void cinemat_state::'+name+r'_sound_w\(.*?\n\}',s,re.S)[0]
    fn=fn.replace('cinemat_state::','CinematronicsSound::').replace('machine().rand()','random()').replace('m_screen->frame_number()','frame_')
    fn=re.sub(r'^\s*output\(\)\.set_led_value[^\n]*','',fn,flags=re.M)
    fn=fn.replace('freq = 56818.181818 / (4096 - (m_current_shift & 0xfff));',
                  'freq = static_cast<int>(56818.181818 / (4096 - (m_current_shift & 0xfff)));')
    fn=fn.replace('44100 * freq / 1050','44100.0 * freq / 1050.0')
    if name=='speedfrk':
        fn=fn.replace('((~sound_val << 13) & 1)','((static_cast<uint32_t>(~sound_val) & 4u) << 13)')
    out.append(fn)
out.append('static const char *const *effect_names(Machine machine) { switch(machine) {')
for name in names: out.append('case Machine::%s: return %s_sample_names;'%(name,name))
out+=['default: return nullptr; }}']
(root/'core/cinematronics_sound_events.inc').write_text('\n'.join(out)+'\n',encoding='utf-8')
print('Generated',len(names),'independent sound protocols; no ROM/sample data.')
