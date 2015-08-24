CFLAGS = -c

OBJS = VGMPlay/vgm2pcm.o

LIB_OBJS = VGMPlay/ChipMapper.o VGMPlay/VGMPlay.o VGMPlay/chips/2151intf.o\
	VGMPlay/chips/2203intf.o VGMPlay/chips/2413intf.o VGMPlay/chips/2608intf.o\
	VGMPlay/chips/2610intf.o VGMPlay/chips/2612intf.o VGMPlay/chips/262intf.o\
	VGMPlay/chips/3526intf.o VGMPlay/chips/3812intf.o VGMPlay/chips/8950intf.o\
	VGMPlay/chips/adlibemu_opl2.o VGMPlay/chips/adlibemu_opl3.o\
	VGMPlay/chips/ay8910.o VGMPlay/chips/ay8910_opl.o VGMPlay/chips/ay_intf.o\
	VGMPlay/chips/c140.o VGMPlay/chips/c352.o VGMPlay/chips/c6280.o\
	VGMPlay/chips/c6280intf.o VGMPlay/chips/dac_control.o VGMPlay/chips/emu2149.o\
	VGMPlay/chips/emu2413.o VGMPlay/chips/es5503.o VGMPlay/chips/es5506.o\
	VGMPlay/chips/fm.o VGMPlay/chips/fm2612.o VGMPlay/chips/fmopl.o\
	VGMPlay/chips/gb.o VGMPlay/chips/iremga20.o VGMPlay/chips/k051649.o\
	VGMPlay/chips/k053260.o VGMPlay/chips/k054539.o VGMPlay/chips/multipcm.o\
	VGMPlay/chips/nes_apu.o VGMPlay/chips/nes_intf.o VGMPlay/chips/np_nes_apu.o\
	VGMPlay/chips/np_nes_dmc.o VGMPlay/chips/np_nes_fds.o VGMPlay/chips/okim6258.o\
	VGMPlay/chips/okim6295.o VGMPlay/chips/Ootake_PSG.o	VGMPlay/chips/panning.o\
	VGMPlay/chips/pokey.o VGMPlay/chips/pwm.o VGMPlay/chips/qsound.o\
	VGMPlay/chips/rf5c68.o VGMPlay/chips/saa1099.o VGMPlay/chips/scd_pcm.o\
	VGMPlay/chips/scsp.o VGMPlay/chips/scspdsp.o VGMPlay/chips/segapcm.o\
	VGMPlay/chips/sn76489.o VGMPlay/chips/sn76496.o VGMPlay/chips/sn76496_opl.o\
	VGMPlay/chips/sn764intf.o VGMPlay/chips/upd7759.o VGMPlay/chips/vsu.o\
	VGMPlay/chips/ws_audio.o VGMPlay/chips/x1_010.o VGMPlay/chips/ym2151.o\
	VGMPlay/chips/ym2413.o VGMPlay/chips/ym2413_opl.o VGMPlay/chips/ym2413hd.o\
	VGMPlay/chips/ym2612.o VGMPlay/chips/ymdeltat.o VGMPlay/chips/ymf262.o\
	VGMPlay/chips/ymf271.o VGMPlay/chips/ymf278b.o VGMPlay/chips/ymz280b.o\
	VGMPlay/resampler.o

OPTS = -O2

all: libvgmplay.a vgm2pcm

vgm2pcm: $(OBJS) libvgmplay.a
	$(CC) $(OPTS) -o $@ $^ -lz

libvgmplay.a : $(LIB_OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(OPTS) -o $@ $^

clean:
	rm -f $(OBJS) $(LIB_OBJS) libvgmplay.a > /dev/null
