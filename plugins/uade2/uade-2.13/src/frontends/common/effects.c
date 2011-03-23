/* Effect module for UADE2 frontends.

   Copyright 2005 (C) Antti S. Lankila <alankila@bel.fi>

   This module is licensed under the GNU LGPL.
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include <compilersupport.h>

#include "effects.h"

/*** old headphone effect ***/
#define UADE_EFFECT_HEADPHONES_DELAY_LENGTH 22
#define UADE_EFFECT_HEADPHONES_DELAY_DIRECT 0.3
#define UADE_EFFECT_HEADPHONES_CROSSMIX_VOL 0.80

static float headphones_ap_l[UADE_EFFECT_HEADPHONES_DELAY_LENGTH];
static float headphones_ap_r[UADE_EFFECT_HEADPHONES_DELAY_LENGTH];
static float headphones_rc_l[4];
static float headphones_rc_r[4];

/*** new headphone effect ***/

/* delay time defines the width of the head. 0.5 ms gives us 15 cm virtual distance
 * between sound arriving to either ear. */
#define HEADPHONE2_DELAY_TIME 0.49e-3
#define HEADPHONE2_DELAY_K 0.15
/* head shadow frequency cutoff */
#define HEADPHONE2_SHADOW_FREQ 8000.0
/* high shelve keeps frequencies below cutoff intact and attenuates
 * the rest in an uniform way. The effect is to make bass more "mono" than "stereo". */
#define HEADPHONE2_SHELVE_FREQ 100.0
#define HEADPHONE2_SHELVE_LEVEL -2.0

#define MAXIMUM_SAMPLING_RATE 96000
#define HEADPHONE2_DELAY_MAX_LENGTH ((int)(MAXIMUM_SAMPLING_RATE*HEADPHONE2_DELAY_TIME+1))
#define DENORMAL_OFFSET 1E-10

#define NORMALISE_RESOLUTION 10	/* in bits */
#define NORMALISE_DEFAULT_GAIN 8.0
#define NORMALISE_MAXIMUM_GAIN 8.0

/* Headphone variables */
typedef struct {
	float b0, b1, b2, a1, a2, x[2], y[2];
} biquad_t;

static float headphone2_ap_l[HEADPHONE2_DELAY_MAX_LENGTH];
static float headphone2_ap_r[HEADPHONE2_DELAY_MAX_LENGTH];
static int headphone2_delay_length;
static biquad_t headphone2_shelve_l;
static biquad_t headphone2_shelve_r;
static biquad_t headphone2_rc_l;
static biquad_t headphone2_rc_r;

/* Normalise variables */
static int normalise_peak_level;
static int normalise_historic_maximum_peak;
static int normalise_oldlevel;

static void gain(int gain_amount, int16_t * sm, int frames);
static void pan(int pan_amount, int16_t * sm, int frames);
static void headphones(int16_t * sm, int frames);
static void headphones2(int16_t * sm, int frames);
static void normalise(int change_level, int16_t * sm, int frames);
static int normalise_compute_gain(int peak);

static inline int sampleclip(int x)
{
	if (unlikely(x > 32767 || x < -32768)) {
		if (x > 32767)
			x = 32767;
		else
			x = -32768;
	}
	return x;
}

/* calculate a high shelve filter */
static void calculate_shelve(double fs, double fc, double g, biquad_t * bq)
{
	float A, omega, sn, cs, beta, b0, b1, b2, a0, a1, a2;

	A = powf(10, g / 40);
	omega = 2 * M_PI * fc / fs;
	omega = tan(omega / 2) * 2;
	sn = sin(omega);
	cs = cos(omega);
	beta = sqrt(A + A);

	b0 = A * ((A + 1) + (A - 1) * cs + beta * sn);
	b1 = -2 * A * ((A - 1) + (A + 1) * cs);
	b2 = A * ((A + 1) + (A - 1) * cs - beta * sn);
	a0 = (A + 1) - (A - 1) * cs + beta * sn;
	a1 = 2 * ((A - 1) - (A + 1) * cs);
	a2 = (A + 1) - (A - 1) * cs - beta * sn;

	bq->b0 = b0 / a0;
	bq->b1 = b1 / a0;
	bq->b2 = b2 / a0;
	bq->a1 = a1 / a0;
	bq->a2 = a2 / a0;
}

/* calculate 1st order lowpass filter */
static void calculate_rc(double fs, double fc, biquad_t * bq)
{
	float omega;

	if (fc >= fs / 2) {
		bq->b0 = 1.0;
		bq->b1 = 0.0;
		bq->b2 = 0.0;
		bq->a1 = 0.0;
		bq->a2 = 0.0;
		return;
	}
	omega = 2 * M_PI * fc / fs;
	omega = tan(omega / 2) * 2;

	bq->b0 = 1 / (1 + 1 / omega);
	bq->b1 = 0;
	bq->b2 = 0;
	bq->a1 = -1 + bq->b0;
	bq->a2 = 0;
}

static inline float evaluate_biquad(float input, biquad_t * bq)
{
	float output = DENORMAL_OFFSET;

	output += input * bq->b0 + bq->x[0] * bq->b1 + bq->x[1] * bq->b2;
	output -= bq->y[0] * bq->a1 + bq->y[1] * bq->a2;

	bq->x[1] = bq->x[0];
	bq->x[0] = input;

	bq->y[1] = bq->y[0];
	bq->y[0] = output;

	return output;
}

static void reset_biquad(biquad_t * bq)
{
	bq->x[0] = bq->x[1] = bq->y[0] = bq->y[1] = 0;
}

/* Reset effects' state variables.
 * Call this method between before starting playback */
void uade_effect_reset_internals(void)
{
	/* old headphones */
	memset(headphones_ap_l, 0, sizeof(headphones_ap_l));
	memset(headphones_ap_r, 0, sizeof(headphones_ap_r));
	memset(headphones_rc_l, 0, sizeof(headphones_rc_l));
	memset(headphones_rc_r, 0, sizeof(headphones_rc_r));

	/* new headphones */
	memset(headphone2_ap_l, 0, sizeof(headphone2_ap_l));
	memset(headphone2_ap_r, 0, sizeof(headphone2_ap_r));
	reset_biquad(&headphone2_shelve_l);
	reset_biquad(&headphone2_shelve_r);
	reset_biquad(&headphone2_rc_l);
	reset_biquad(&headphone2_rc_r);

	normalise_peak_level = 0;
	normalise_historic_maximum_peak = 0;
	normalise_oldlevel = 1 << NORMALISE_RESOLUTION;
}

void uade_effect_disable_all(struct uade_effect *ue)
{
	ue->enabled = 0;
}

void uade_effect_disable(struct uade_effect *ue, uade_effect_t effect)
{
	ue->enabled &= ~(1 << effect);
}

void uade_effect_enable(struct uade_effect *ue, uade_effect_t effect)
{
	ue->enabled |= 1 << effect;
}

/* Returns 1 if effect is enabled, and zero otherwise. Ignores
   UADE_EFFECT_ALLOW. */
int uade_effect_is_enabled(struct uade_effect *ue, uade_effect_t effect)
{
	return (ue->enabled & (1 << effect)) != 0;
}

void uade_effect_run(struct uade_effect *ue, int16_t * samples, int frames)
{
	if (ue->enabled & (1 << UADE_EFFECT_ALLOW)) {
		normalise(ue->enabled & (1 << UADE_EFFECT_NORMALISE), samples,
			  frames);
		if (ue->enabled & (1 << UADE_EFFECT_PAN))
			pan(ue->pan, samples, frames);
		if (ue->enabled & (1 << UADE_EFFECT_HEADPHONES))
			headphones(samples, frames);
		if (ue->enabled & (1 << UADE_EFFECT_HEADPHONES2) && ue->rate)
			headphones2(samples, frames);
		if (ue->enabled & (1 << UADE_EFFECT_GAIN))
			gain(ue->gain, samples, frames);
	}
}

void uade_effect_toggle(struct uade_effect *ue, uade_effect_t effect)
{
	ue->enabled ^= 1 << effect;
}

void uade_effect_set_defaults(struct uade_effect *ue)
{
	memset(ue, 0, sizeof(*ue));
	uade_effect_disable_all(ue);
	uade_effect_enable(ue, UADE_EFFECT_ALLOW);
	uade_effect_gain_set_amount(ue, 1.0);
	uade_effect_pan_set_amount(ue, 0.7);
}

/* Rate of 0 means undefined. Effects that depend on sample rate must
   self-check against this because they can not implemented properly */
void uade_effect_set_sample_rate(struct uade_effect *ue, int rate)
{
	assert(rate >= 0);
	ue->rate = rate;

	if (rate == 0)
		return;

	calculate_shelve(rate, HEADPHONE2_SHELVE_FREQ, HEADPHONE2_SHELVE_LEVEL,
			 &headphone2_shelve_l);
	calculate_shelve(rate, HEADPHONE2_SHELVE_FREQ, HEADPHONE2_SHELVE_LEVEL,
			 &headphone2_shelve_r);
	calculate_rc(rate, HEADPHONE2_SHADOW_FREQ, &headphone2_rc_l);
	calculate_rc(rate, HEADPHONE2_SHADOW_FREQ, &headphone2_rc_r);
	headphone2_delay_length = HEADPHONE2_DELAY_TIME * rate + 0.5;
	if (headphone2_delay_length > HEADPHONE2_DELAY_MAX_LENGTH) {
		fprintf(stderr,	"effects.c: truncating headphone delay line due to samplerate exceeding 96 kHz.\n");
		headphone2_delay_length = HEADPHONE2_DELAY_MAX_LENGTH;
	}
}

void uade_effect_gain_set_amount(struct uade_effect *ue, float amount)
{
	assert(amount >= 0.0 && amount <= 128.0);
	ue->gain = amount * 256.0;
}

void uade_effect_pan_set_amount(struct uade_effect *ue, float amount)
{
	assert(amount >= 0.0 && amount <= 2.0);
	ue->pan = amount * 256.0 / 2.0;
}

static int normalise_compute_gain(int peak)
{
	if (normalise_historic_maximum_peak == 0) {
		/* if the peak is not known, we cap gain in an attempt to avoid
		 * boosting silent intros too much. */
		if (peak < 32768 / NORMALISE_DEFAULT_GAIN)
			return NORMALISE_DEFAULT_GAIN *
			    (1 << NORMALISE_RESOLUTION);
		else
			return (32768 << NORMALISE_RESOLUTION) / peak;
	} else {
		int largerpeak;
		if (peak < normalise_historic_maximum_peak)
			largerpeak = normalise_historic_maximum_peak;
		else
			largerpeak = peak;
		/* if the peak is known, we use the recorded value but adapt
		   if this rendition comes out louder for some reason (for
		   instance, updated UADE) */
		if (largerpeak < 32768 / NORMALISE_MAXIMUM_GAIN)
			return NORMALISE_MAXIMUM_GAIN *
			    (1 << NORMALISE_RESOLUTION);
		else
			return (32768 << NORMALISE_RESOLUTION) / largerpeak;
	}
}

/* We save gain from maximum known level. This is an one-way street,
   the gain can * only decrease with time. If the historic level is
   known and larger, we prefer it. */
void uade_effect_normalise_serialise(char *buf, size_t len)
{
	int peak = normalise_peak_level;

	assert(len > 0);

	if (normalise_historic_maximum_peak > normalise_peak_level)
		peak = normalise_historic_maximum_peak;

	if (snprintf(buf, len, "v=1,p=%d", peak) >= len) {
		fprintf(stderr,	"normalise effect: buffer too short, gain would be truncated. This is a bug in UADE.\n");
		exit(-1);
	}
}

/* similarly, this should only be called if gain has a positive value,
 * but we try to recover from misuse. */
void uade_effect_normalise_unserialise(const char *buf)
{
	int version, readcount;
	float peak;

	normalise_historic_maximum_peak = 0;

	if (buf == NULL)
		return;

	readcount = sscanf(buf, "v=%d,p=%f", &version, &peak);

	if (readcount == 0) {
		fprintf(stderr, "normalise effect: gain string invalid: '%s'\n", buf);
		exit(-1);
	}

	if (version != 1) {
		fprintf(stderr,	"normalise effect: unrecognized gain version: '%s'\n", buf);
		exit(-1);
	}

	if (readcount != 2) {
		fprintf(stderr,	"Could not read peak value for version 1: '%s'\n", buf);
		exit(-1);
	}

	if (peak >= 0.0 && peak <= 1.0) {
		normalise_oldlevel = normalise_historic_maximum_peak =
		    32768 * peak;
	} else {
		fprintf(stderr, "normalise effect: invalid peak level: '%s'\n", buf);
	}
}

static void normalise(int change_level, int16_t * sm, int frames)
{
	int i;

	/* Negative side is mirrored. but positive side gains by 1.
	 * This is to make both semiwaves have same max. */
	for (i = 0; i < 2 * frames; i += 1) {
		int tmp = sm[i];
		tmp = (tmp >= 0) ? tmp + 1 : -tmp;
		if (tmp > normalise_peak_level)
			normalise_peak_level = tmp;
	}

	/* Slight clipping may result in first playback while the system
	 * adjusts.  With a bit of "advance warning" of clipping about to
	 * occur, the level begins to adjust as soon as the buffer
	 * begins. Typical adjustment times are not large -- a few hundred
	 * samples are to be expected -- and the clipping should only
	 * occur on the first rendition of the song, if at all. */
	if (change_level) {
		int newlevel = normalise_compute_gain(normalise_peak_level);

		for (i = 0; i < 2 * frames; i += 1) {
			/* same gain for the frame */
			if ((i & 1) == 0) {
				if (normalise_oldlevel < newlevel)
					normalise_oldlevel += 1;
				if (normalise_oldlevel > newlevel)
					normalise_oldlevel -= 1;
			}
			sm[i] =
			    sampleclip((sm[i] *
					normalise_oldlevel) >>
				       NORMALISE_RESOLUTION);
		}
	}
}

static void gain(int gain_amount, int16_t * sm, int frames)
{
	int i;
	for (i = 0; i < 2 * frames; i += 1)
		sm[i] = sampleclip((sm[i] * gain_amount) >> 8);
}

/* Panning effect. Turns stereo into mono in a specific degree */
static void pan(int pan_amount, int16_t * sm, int frames)
{
	int i, l, r, m;
	for (i = 0; i < frames; i += 1) {
		l = sm[0];
		r = sm[1];
		m = (r - l) * pan_amount;
		sm[0] = ((l << 8) + m) >> 8;
		sm[1] = ((r << 8) - m) >> 8;
		sm += 2;
	}
}

/* All-pass delay. Its purpose is to confuse the phase of the sound a bit
 * and also provide some delay to locate the source outside the head. This
 * seems to work better than a pure delay line. */
static float headphones_allpass_delay(float in, float *state)
{
	int i;
	float tmp, output;

	tmp = in - UADE_EFFECT_HEADPHONES_DELAY_DIRECT * state[0];
	output = state[0] + UADE_EFFECT_HEADPHONES_DELAY_DIRECT * tmp;

	/* FIXME: use modulo and index */
	for (i = 1; i < UADE_EFFECT_HEADPHONES_DELAY_LENGTH; i += 1)
		state[i - 1] = state[i];
	state[UADE_EFFECT_HEADPHONES_DELAY_LENGTH - 1] = tmp;

	return output;
}

static float headphones_lpf(float in, float *state)
{
	float out = in * 0.53;
	out += 0.47 * state[0];
	state[0] = out;

	return out;
}

/* A real implementation would simply perform FIR with recorded HRTF data. */
static void headphones(int16_t * sm, int frames)
{
	int i;
	float ld, rd;
	int l_final, r_final;
	for (i = 0; i < frames; i += 1) {
		ld = headphones_allpass_delay(sm[0], headphones_ap_l);
		rd = headphones_allpass_delay(sm[1], headphones_ap_r);
		ld = headphones_lpf(ld, headphones_rc_l);
		rd = headphones_lpf(rd, headphones_rc_r);

		l_final =
		    (sm[0] + rd * UADE_EFFECT_HEADPHONES_CROSSMIX_VOL) / 2;
		r_final =
		    (sm[1] + ld * UADE_EFFECT_HEADPHONES_CROSSMIX_VOL) / 2;
		sm[0] = sampleclip(l_final);
		sm[1] = sampleclip(r_final);

		sm += 2;
	}
}

static float headphone2_allpass_delay(float in, float *state)
{
	int i;
	float tmp, output;

	tmp = in - HEADPHONE2_DELAY_K * state[0];
	output = state[0] + HEADPHONE2_DELAY_K * tmp;

	/* FIXME: use modulo and index */
	for (i = 1; i < headphone2_delay_length; i += 1)
		state[i - 1] = state[i];
	state[headphone2_delay_length - 1] = tmp;

	return output;
}

static void headphones2(int16_t * sm, int frames)
{
	int i;
	for (i = 0; i < frames; i += 1) {
		float ld, rd;

		ld = headphone2_allpass_delay(sm[0], headphone2_ap_l);
		rd = headphone2_allpass_delay(sm[1], headphone2_ap_r);
		ld = evaluate_biquad(ld, &headphone2_rc_l);
		rd = evaluate_biquad(rd, &headphone2_rc_r);
		ld = evaluate_biquad(ld, &headphone2_shelve_l);
		rd = evaluate_biquad(rd, &headphone2_shelve_r);

		sm[0] = sampleclip((sm[0] + rd) / 2);
		sm[1] = sampleclip((sm[1] + ld) / 2);
		sm += 2;
	}
}
