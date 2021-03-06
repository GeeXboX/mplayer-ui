/*
 * parse.c
 * Copyright (C) 2000-2001 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of a52dec, a free ATSC A-52 stream decoder.
 * See http://liba52.sourceforge.net/ for updates.
 *
 * a52dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * a52dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "a52.h"
#include "a52_internal.h"
#include "bitstream.h"
#include "tables.h"
#include "mm_accel.h"

#ifdef HAVE_MEMALIGN
/* some systems have memalign() but no declaration for it */
void * memalign (size_t align, size_t size);
#endif

typedef struct {
    sample_t q1[2];
    sample_t q2[2];
    sample_t q4;
    int q1_ptr;
    int q2_ptr;
    int q4_ptr;
} quantizer_t;

static uint8_t halfrate[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3};

sample_t * a52_init (uint32_t mm_accel)
{
    sample_t * samples;
    int i;

    samples = memalign (16, 256 * 12 * sizeof (sample_t));
#if defined(__MINGW32__) && defined(HAVE_SSE) 
    for(i=0;i<10;i++){
      if((int)samples%16){
        sample_t* samplestmp=malloc(256 * 12 * sizeof (sample_t));   
        free(samples);
        samples = samplestmp;    
      }
      else break;
    }
#endif
    if(((int)samples%16) && (mm_accel&MM_ACCEL_X86_SSE)){
      mm_accel &=~MM_ACCEL_X86_SSE;
      printf("liba52: unable to get 16 byte aligned memory disabling usage of SSE instructions\n");
    }   
    
    if (samples == NULL)
	return NULL;    
    
    imdct_init (mm_accel);
    downmix_accel_init(mm_accel);
    
    for (i = 0; i < 256 * 12; i++)
	samples[i] = 0;

    return samples;
}

int a52_syncinfo (uint8_t * buf, int * flags,
		  int * sample_rate, int * bit_rate)
{
    static int rate[] = { 32,  40,  48,  56,  64,  80,  96, 112,
			 128, 160, 192, 224, 256, 320, 384, 448,
			 512, 576, 640};
    static uint8_t lfeon[8] = {0x10, 0x10, 0x04, 0x04, 0x04, 0x01, 0x04, 0x01};
    int frmsizecod;
    int bitrate;
    int half;
    int acmod;

    if ((buf[0] != 0x0b) || (buf[1] != 0x77))	/* syncword */
	return 0;

    if (buf[5] >= 0x60)		/* bsid >= 12 */
	return 0;
    half = halfrate[buf[5] >> 3];

    /* acmod, dsurmod and lfeon */
    acmod = buf[6] >> 5;
    *flags = ((((buf[6] & 0xf8) == 0x50) ? A52_DOLBY : acmod) |
	      ((buf[6] & lfeon[acmod]) ? A52_LFE : 0));

    frmsizecod = buf[4] & 63;
    if (frmsizecod >= 38)
	return 0;
    bitrate = rate [frmsizecod >> 1];
    *bit_rate = (bitrate * 1000) >> half;

    switch (buf[4] & 0xc0) {
    case 0:	/* 48 KHz */
	*sample_rate = 48000 >> half;
	return 4 * bitrate;
    case 0x40:
	*sample_rate = 44100 >> half;
	return 2 * (320 * bitrate / 147 + (frmsizecod & 1));
    case 0x80:
	*sample_rate = 32000 >> half;
	return 6 * bitrate;
    default:
	return 0;
    }
}

int a52_frame (a52_state_t * state, uint8_t * buf, int * flags,
	       sample_t * level, sample_t bias)
{
    static sample_t clev[4] = {LEVEL_3DB, LEVEL_45DB, LEVEL_6DB, LEVEL_45DB};
    static sample_t slev[4] = {LEVEL_3DB, LEVEL_6DB, 0, LEVEL_6DB};
    int chaninfo;
    int acmod;

    state->fscod = buf[4] >> 6;
    state->halfrate = halfrate[buf[5] >> 3];
    state->acmod = acmod = buf[6] >> 5;

    bitstream_set_ptr (buf + 6);
    bitstream_skip (3);	/* skip acmod we already parsed */

    if ((acmod == 2) && (bitstream_get (2) == 2))	/* dsurmod */
	acmod = A52_DOLBY;

    if ((acmod & 1) && (acmod != 1))
	state->clev = clev[bitstream_get (2)];	/* cmixlev */

    if (acmod & 4)
	state->slev = slev[bitstream_get (2)];	/* surmixlev */

    state->lfeon = bitstream_get (1);

    state->output = downmix_init (acmod, *flags, level,
				  state->clev, state->slev);
    if (state->output < 0)
	return 1;
    if (state->lfeon && (*flags & A52_LFE))
	state->output |= A52_LFE;
    *flags = state->output;
    /* the 2* compensates for differences in imdct */
    state->dynrng = state->level = 2 * *level;
    state->bias = bias;
    state->dynrnge = 1;
    state->dynrngcall = NULL;

    chaninfo = !acmod;
    do {
	bitstream_skip (5);	/* dialnorm */
	if (bitstream_get (1))	/* compre */
	    bitstream_skip (8);	/* compr */
	if (bitstream_get (1))	/* langcode */
	    bitstream_skip (8);	/* langcod */
	if (bitstream_get (1))	/* audprodie */
	    bitstream_skip (7);	/* mixlevel + roomtyp */
    } while (chaninfo--);

    bitstream_skip (2);		/* copyrightb + origbs */

    if (bitstream_get (1))	/* timecod1e */
	bitstream_skip (14);	/* timecod1 */
    if (bitstream_get (1))	/* timecod2e */
	bitstream_skip (14);	/* timecod2 */

    if (bitstream_get (1)) {	/* addbsie */
	int addbsil;

	addbsil = bitstream_get (6);
	do {
	    bitstream_skip (8);	/* addbsi */
	} while (addbsil--);
    }

    return 0;
}

void a52_dynrng (a52_state_t * state,
		 sample_t (* call) (sample_t, void *), void * data)
{
    state->dynrnge = 0;
    if (call) {
	state->dynrnge = 1;
	state->dynrngcall = call;
	state->dynrngdata = data;
    }
}

static int parse_exponents (int expstr, int ngrps, uint8_t exponent,
			    uint8_t * dest)
{
    int exps;

    while (ngrps--) {
	exps = bitstream_get (7);

	exponent += exp_1[exps];
	if (exponent > 24)
	    return 1;

	switch (expstr) {
	case EXP_D45:
	    *(dest++) = exponent;
	    *(dest++) = exponent;
	case EXP_D25:
	    *(dest++) = exponent;
	case EXP_D15:
	    *(dest++) = exponent;
	}

	exponent += exp_2[exps];
	if (exponent > 24)
	    return 1;

	switch (expstr) {
	case EXP_D45:
	    *(dest++) = exponent;
	    *(dest++) = exponent;
	case EXP_D25:
	    *(dest++) = exponent;
	case EXP_D15:
	    *(dest++) = exponent;
	}

	exponent += exp_3[exps];
	if (exponent > 24)
	    return 1;

	switch (expstr) {
	case EXP_D45:
	    *(dest++) = exponent;
	    *(dest++) = exponent;
	case EXP_D25:
	    *(dest++) = exponent;
	case EXP_D15:
	    *(dest++) = exponent;
	}
    }	

    return 0;
}

static int parse_deltba (int8_t * deltba)
{
    int deltnseg, deltlen, delta, j;

    memset (deltba, 0, 50);

    deltnseg = bitstream_get (3);
    j = 0;
    do {
	j += bitstream_get (5);
	deltlen = bitstream_get (4);
	delta = bitstream_get (3);
	delta -= (delta >= 4) ? 3 : 4;
	if (!deltlen)
	    continue;
	if (j + deltlen >= 50)
	    return 1;
	while (deltlen--)
	    deltba[j++] = delta;
    } while (deltnseg--);

    return 0;
}

static inline int zero_snr_offsets (int nfchans, a52_state_t * state)
{
    int i;

    if ((state->csnroffst) || (state->cplinu && state->cplba.fsnroffst) ||
	(state->lfeon && state->lfeba.fsnroffst))
	return 0;
    for (i = 0; i < nfchans; i++)
	if (state->ba[i].fsnroffst)
	    return 0;
    return 1;
}

static inline int16_t dither_gen (void)
{
    static uint16_t lfsr_state = 1;
    int16_t state;

    state = dither_lut[lfsr_state >> 8] ^ (lfsr_state << 8);
	
    lfsr_state = (uint16_t) state;

    return state;
}

static void coeff_get (sample_t * coeff, uint8_t * exp, int8_t * bap,
		       quantizer_t * quantizer, sample_t level,
		       int dither, int end)
{
    int i;
    sample_t factor[25];

    for (i = 0; i <= 24; i++)
	factor[i] = scale_factor[i] * level;

    for (i = 0; i < end; i++) {
	int bapi;

	bapi = bap[i];
	switch (bapi) {
	case 0:
	    if (dither) {
		coeff[i] = dither_gen() * LEVEL_3DB * factor[exp[i]];
		continue;
	    } else {
		coeff[i] = 0;
		continue;
	    }

	case -1:
	    if (quantizer->q1_ptr >= 0) {
		coeff[i] = quantizer->q1[quantizer->q1_ptr--] * factor[exp[i]];
		continue;
	    } else {
		int code;

		code = bitstream_get (5);

		quantizer->q1_ptr = 1;
		quantizer->q1[0] = q_1_2[code];
		quantizer->q1[1] = q_1_1[code];
		coeff[i] = q_1_0[code] * factor[exp[i]];
		continue;
	    }

	case -2:
	    if (quantizer->q2_ptr >= 0) {
		coeff[i] = quantizer->q2[quantizer->q2_ptr--] * factor[exp[i]];
		continue;
	    } else {
		int code;

		code = bitstream_get (7);

		quantizer->q2_ptr = 1;
		quantizer->q2[0] = q_2_2[code];
		quantizer->q2[1] = q_2_1[code];
		coeff[i] = q_2_0[code] * factor[exp[i]];
		continue;
	    }

	case 3:
	    coeff[i] = q_3[bitstream_get (3)] * factor[exp[i]];
	    continue;

	case -3:
	    if (quantizer->q4_ptr == 0) {
		quantizer->q4_ptr = -1;
		coeff[i] = quantizer->q4 * factor[exp[i]];
		continue;
	    } else {
		int code;

		code = bitstream_get (7);

		quantizer->q4_ptr = 0;
		quantizer->q4 = q_4_1[code];
		coeff[i] = q_4_0[code] * factor[exp[i]];
		continue;
	    }

	case 4:
	    coeff[i] = q_5[bitstream_get (4)] * factor[exp[i]];
	    continue;

	default:
	    coeff[i] = ((bitstream_get_2 (bapi) << (16 - bapi)) *
			  factor[exp[i]]);
	}
    }
}

static void coeff_get_coupling (a52_state_t * state, int nfchans,
				sample_t * coeff, sample_t (* samples)[256],
				quantizer_t * quantizer, uint8_t dithflag[5])
{
    int sub_bnd, bnd, i, i_end, ch;
    int8_t * bap;
    uint8_t * exp;
    sample_t cplco[5];

    bap = state->cpl_bap;
    exp = state->cpl_exp;
    sub_bnd = bnd = 0;
    i = state->cplstrtmant;
    while (i < state->cplendmant) {
	i_end = i + 12;
	while (state->cplbndstrc[sub_bnd++])
	    i_end += 12;
	for (ch = 0; ch < nfchans; ch++)
	    cplco[ch] = state->cplco[ch][bnd] * coeff[ch];
	bnd++;

	while (i < i_end) {
	    sample_t cplcoeff;
	    int bapi;

	    bapi = bap[i];
	    switch (bapi) {
	    case 0:
		cplcoeff = LEVEL_3DB * scale_factor[exp[i]];
		for (ch = 0; ch < nfchans; ch++)
		    if (state->chincpl[ch]) {
			if (dithflag[ch])
			    samples[ch][i] = (cplcoeff * cplco[ch] *
					      dither_gen ());
			else
			    samples[ch][i] = 0;
		    }
		i++;
		continue;

	    case -1:
		if (quantizer->q1_ptr >= 0) {
		    cplcoeff = quantizer->q1[quantizer->q1_ptr--];
		    break;
		} else {
		    int code;

		    code = bitstream_get (5);

		    quantizer->q1_ptr = 1;
		    quantizer->q1[0] = q_1_2[code];
		    quantizer->q1[1] = q_1_1[code];
		    cplcoeff = q_1_0[code];
		    break;
		}

	    case -2:
		if (quantizer->q2_ptr >= 0) {
		    cplcoeff = quantizer->q2[quantizer->q2_ptr--];
		    break;
		} else {
		    int code;

		    code = bitstream_get (7);

		    quantizer->q2_ptr = 1;
		    quantizer->q2[0] = q_2_2[code];
		    quantizer->q2[1] = q_2_1[code];
		    cplcoeff = q_2_0[code];
		    break;
		}

	    case 3:
		cplcoeff = q_3[bitstream_get (3)];
		break;

	    case -3:
		if (quantizer->q4_ptr == 0) {
		    quantizer->q4_ptr = -1;
		    cplcoeff = quantizer->q4;
		    break;
		} else {
		    int code;

		    code = bitstream_get (7);

		    quantizer->q4_ptr = 0;
		    quantizer->q4 = q_4_1[code];
		    cplcoeff = q_4_0[code];
		    break;
		}

	    case 4:
		cplcoeff = q_5[bitstream_get (4)];
		break;

	    default:
		cplcoeff = bitstream_get_2 (bapi) << (16 - bapi);
	    }

	    cplcoeff *= scale_factor[exp[i]];
	    for (ch = 0; ch < nfchans; ch++)
		if (state->chincpl[ch])
		    samples[ch][i] = cplcoeff * cplco[ch];
	    i++;
	}
    }
}

int a52_block (a52_state_t * state, sample_t * samples)
{
    static const uint8_t nfchans_tbl[] = {2, 1, 2, 3, 3, 4, 4, 5, 1, 1, 2};
    static int rematrix_band[4] = {25, 37, 61, 253};
    int i, nfchans, chaninfo;
    uint8_t cplexpstr, chexpstr[5], lfeexpstr, do_bit_alloc, done_cpl;
    uint8_t blksw[5], dithflag[5];
    sample_t coeff[5];
    int chanbias;
    quantizer_t quantizer;

    nfchans = nfchans_tbl[state->acmod];

    for (i = 0; i < nfchans; i++)
	blksw[i] = bitstream_get (1);

    for (i = 0; i < nfchans; i++)
	dithflag[i] = bitstream_get (1);

    chaninfo = !(state->acmod);
    do {
	if (bitstream_get (1)) {	/* dynrnge */
	    int dynrng;

	    dynrng = bitstream_get_2 (8);
	    if (state->dynrnge) {
		sample_t range;

		range = ((((dynrng & 0x1f) | 0x20) << 13) *
			 scale_factor[3 - (dynrng >> 5)]);
		if (state->dynrngcall)
		    range = state->dynrngcall (range, state->dynrngdata);
		state->dynrng = state->level * range;
	    }
	}
    } while (chaninfo--);

    if (bitstream_get (1)) {	/* cplstre */
	state->cplinu = bitstream_get (1);
	if (state->cplinu) {
	    static int bndtab[16] = {31, 35, 37, 39, 41, 42, 43, 44,
				     45, 45, 46, 46, 47, 47, 48, 48};
	    int cplbegf;
	    int cplendf;
	    int ncplsubnd;

	    for (i = 0; i < nfchans; i++)
		state->chincpl[i] = bitstream_get (1);
	    switch (state->acmod) {
	    case 0: case 1:
		return 1;
	    case 2:
		state->phsflginu = bitstream_get (1);
	    }
	    cplbegf = bitstream_get (4);
	    cplendf = bitstream_get (4);

	    if (cplendf + 3 - cplbegf < 0)
		return 1;
	    state->ncplbnd = ncplsubnd = cplendf + 3 - cplbegf;
	    state->cplstrtbnd = bndtab[cplbegf];
	    state->cplstrtmant = cplbegf * 12 + 37;
	    state->cplendmant = cplendf * 12 + 73;

	    for (i = 0; i < ncplsubnd - 1; i++) {
		state->cplbndstrc[i] = bitstream_get (1);
		state->ncplbnd -= state->cplbndstrc[i];
	    }
	    state->cplbndstrc[i] = 0;	/* last value is a sentinel */
	}
    }

    if (state->cplinu) {
	int j, cplcoe;

	cplcoe = 0;
	for (i = 0; i < nfchans; i++)
	    if (state->chincpl[i])
		if (bitstream_get (1)) {	/* cplcoe */
		    int mstrcplco, cplcoexp, cplcomant;

		    cplcoe = 1;
		    mstrcplco = 3 * bitstream_get (2);
		    for (j = 0; j < state->ncplbnd; j++) {
			cplcoexp = bitstream_get (4);
			cplcomant = bitstream_get (4);
			if (cplcoexp == 15)
			    cplcomant <<= 14;
			else
			    cplcomant = (cplcomant | 0x10) << 13;
			state->cplco[i][j] =
			    cplcomant * scale_factor[cplcoexp + mstrcplco];
		    }
		}
	if ((state->acmod == 2) && state->phsflginu && cplcoe)
	    for (j = 0; j < state->ncplbnd; j++)
		if (bitstream_get (1))	/* phsflg */
		    state->cplco[1][j] = -state->cplco[1][j];
    }

    if ((state->acmod == 2) && (bitstream_get (1))) {	/* rematstr */
	int end;

	end = (state->cplinu) ? state->cplstrtmant : 253;
	i = 0;
	do
	    state->rematflg[i] = bitstream_get (1);
	while (rematrix_band[i++] < end);
    }

    cplexpstr = EXP_REUSE;
    lfeexpstr = EXP_REUSE;
    if (state->cplinu)
	cplexpstr = bitstream_get (2);
    for (i = 0; i < nfchans; i++)
	chexpstr[i] = bitstream_get (2);
    if (state->lfeon) 
	lfeexpstr = bitstream_get (1);

    for (i = 0; i < nfchans; i++)
	if (chexpstr[i] != EXP_REUSE) {
	    if (state->cplinu && state->chincpl[i])
		state->endmant[i] = state->cplstrtmant;
	    else {
		int chbwcod;

		chbwcod = bitstream_get (6);
		if (chbwcod > 60)
		    return 1;
		state->endmant[i] = chbwcod * 3 + 73;
	    }
	}

    do_bit_alloc = 0;

    if (cplexpstr != EXP_REUSE) {
	int cplabsexp, ncplgrps;

	do_bit_alloc = 64;
	ncplgrps = ((state->cplendmant - state->cplstrtmant) /
		    (3 << (cplexpstr - 1)));
	cplabsexp = bitstream_get (4) << 1;
	if (parse_exponents (cplexpstr, ncplgrps, cplabsexp,
			     state->cpl_exp + state->cplstrtmant))
	    return 1;
    }
    for (i = 0; i < nfchans; i++)
	if (chexpstr[i] != EXP_REUSE) {
	    int grp_size, nchgrps;

	    do_bit_alloc |= 1 << i;
	    grp_size = 3 << (chexpstr[i] - 1);
	    nchgrps = (state->endmant[i] + grp_size - 4) / grp_size;
	    state->fbw_exp[i][0] = bitstream_get (4);
	    if (parse_exponents (chexpstr[i], nchgrps, state->fbw_exp[i][0],
				 state->fbw_exp[i] + 1))
		return 1;
	    bitstream_skip (2);	/* gainrng */
	}
    if (lfeexpstr != EXP_REUSE) {
	do_bit_alloc |= 32;
	state->lfe_exp[0] = bitstream_get (4);
	if (parse_exponents (lfeexpstr, 2, state->lfe_exp[0],
			     state->lfe_exp + 1))
	    return 1;
    }

    if (bitstream_get (1)) {	/* baie */
	do_bit_alloc = -1;
	state->sdcycod = bitstream_get (2);
	state->fdcycod = bitstream_get (2);
	state->sgaincod = bitstream_get (2);
	state->dbpbcod = bitstream_get (2);
	state->floorcod = bitstream_get (3);
    }
    if (bitstream_get (1)) {	/* snroffste */
	do_bit_alloc = -1;
	state->csnroffst = bitstream_get (6);
	if (state->cplinu) {
	    state->cplba.fsnroffst = bitstream_get (4);
	    state->cplba.fgaincod = bitstream_get (3);
	}
	for (i = 0; i < nfchans; i++) {
	    state->ba[i].fsnroffst = bitstream_get (4);
	    state->ba[i].fgaincod = bitstream_get (3);
	}
	if (state->lfeon) {
	    state->lfeba.fsnroffst = bitstream_get (4);
	    state->lfeba.fgaincod = bitstream_get (3);
	}
    }
    if ((state->cplinu) && (bitstream_get (1))) {	/* cplleake */
	do_bit_alloc |= 64;
	state->cplfleak = 2304 - (bitstream_get (3) << 8);
	state->cplsleak = 2304 - (bitstream_get (3) << 8);
    }

    if (bitstream_get (1)) {	/* deltbaie */
	do_bit_alloc = -1;
	if (state->cplinu)
	    state->cplba.deltbae = bitstream_get (2);
	for (i = 0; i < nfchans; i++)
	    state->ba[i].deltbae = bitstream_get (2);
	if (state->cplinu && (state->cplba.deltbae == DELTA_BIT_NEW) &&
	    parse_deltba (state->cplba.deltba))
	    return 1;
	for (i = 0; i < nfchans; i++)
	    if ((state->ba[i].deltbae == DELTA_BIT_NEW) &&
		parse_deltba (state->ba[i].deltba))
		return 1;
    }

    if (do_bit_alloc) {
	if (zero_snr_offsets (nfchans, state)) {
	    memset (state->cpl_bap, 0, sizeof (state->cpl_bap));
	    memset (state->fbw_bap, 0, sizeof (state->fbw_bap));
	    memset (state->lfe_bap, 0, sizeof (state->lfe_bap));
	} else {
	    if (state->cplinu && (do_bit_alloc & 64))
		bit_allocate (state, &state->cplba, state->cplstrtbnd,
			      state->cplstrtmant, state->cplendmant,
			      state->cplfleak, state->cplsleak,
			      state->cpl_exp, state->cpl_bap);
	    for (i = 0; i < nfchans; i++)
		if (do_bit_alloc & (1 << i))
		    bit_allocate (state, state->ba + i, 0, 0,
				  state->endmant[i], 0, 0, state->fbw_exp[i],
				  state->fbw_bap[i]);
	    if (state->lfeon && (do_bit_alloc & 32)) {
		state->lfeba.deltbae = DELTA_BIT_NONE;
		bit_allocate (state, &state->lfeba, 0, 0, 7, 0, 0,
			      state->lfe_exp, state->lfe_bap);
	    }
	}
    }

    if (bitstream_get (1)) {	/* skiple */
	i = bitstream_get (9);	/* skipl */
	while (i--)
	    bitstream_skip (8);
    }

    if (state->output & A52_LFE)
	samples += 256;	/* shift for LFE channel */

    chanbias = downmix_coeff (coeff, state->acmod, state->output,
			      state->dynrng, state->clev, state->slev);

    quantizer.q1_ptr = quantizer.q2_ptr = quantizer.q4_ptr = -1;
    done_cpl = 0;

    for (i = 0; i < nfchans; i++) {
	int j;

	coeff_get (samples + 256 * i, state->fbw_exp[i], state->fbw_bap[i],
		   &quantizer, coeff[i], dithflag[i], state->endmant[i]);

	if (state->cplinu && state->chincpl[i]) {
	    if (!done_cpl) {
		done_cpl = 1;
		coeff_get_coupling (state, nfchans, coeff,
				    (sample_t (*)[256])samples, &quantizer,
				    dithflag);
	    }
	    j = state->cplendmant;
	} else
	    j = state->endmant[i];
	do
	    (samples + 256 * i)[j] = 0;
	while (++j < 256);
    }

    if (state->acmod == 2) {
	int j, end, band;

	end = ((state->endmant[0] < state->endmant[1]) ?
	       state->endmant[0] : state->endmant[1]);

	i = 0;
	j = 13;
	do {
	    if (!state->rematflg[i]) {
		j = rematrix_band[i++];
		continue;
	    }
	    band = rematrix_band[i++];
	    if (band > end)
		band = end;
	    do {
		sample_t tmp0, tmp1;

		tmp0 = samples[j];
		tmp1 = (samples+256)[j];
		samples[j] = tmp0 + tmp1;
		(samples+256)[j] = tmp0 - tmp1;
	    } while (++j < band);
	} while (j < end);
    }

    if (state->lfeon) {
	if (state->output & A52_LFE) {
	    coeff_get (samples - 256, state->lfe_exp, state->lfe_bap,
		       &quantizer, state->dynrng, 0, 7);
	    for (i = 7; i < 256; i++)
		(samples-256)[i] = 0;
	    imdct_512 (samples - 256, samples + 1536 - 256, state->bias);
	} else {
	    /* just skip the LFE coefficients */
	    coeff_get (samples + 1280, state->lfe_exp, state->lfe_bap,
		       &quantizer, 0, 0, 7);
	}
    }

    i = 0;
    if (nfchans_tbl[state->output & A52_CHANNEL_MASK] < nfchans)
	for (i = 1; i < nfchans; i++)
	    if (blksw[i] != blksw[0])
		break;

    if (i < nfchans) {
	if (samples[2 * 1536 - 1] == (sample_t)0x776b6e21) {
	    samples[2 * 1536 - 1] = 0;
	    upmix (samples + 1536, state->acmod, state->output);
	}

	for (i = 0; i < nfchans; i++) {
	    sample_t bias;

	    bias = 0;
	    if (!(chanbias & (1 << i)))
		bias = state->bias;

	    if (coeff[i]) {
		if (blksw[i])
		    imdct_256 (samples + 256 * i, samples + 1536 + 256 * i,
			       bias);
		else 
		    imdct_512 (samples + 256 * i, samples + 1536 + 256 * i,
			       bias);
	    } else {
		int j;

		for (j = 0; j < 256; j++)
		    (samples + 256 * i)[j] = bias;
	    }
	}

	downmix (samples, state->acmod, state->output, state->bias,
		 state->clev, state->slev);
    } else {
	nfchans = nfchans_tbl[state->output & A52_CHANNEL_MASK];

	downmix (samples, state->acmod, state->output, 0,
		 state->clev, state->slev);

	if (samples[2 * 1536 - 1] != (sample_t)0x776b6e21) {
	    downmix (samples + 1536, state->acmod, state->output, 0,
		     state->clev, state->slev);
	    samples[2 * 1536 - 1] = (sample_t)0x776b6e21;
	}

	if (blksw[0])
	    for (i = 0; i < nfchans; i++)
		imdct_256 (samples + 256 * i, samples + 1536 + 256 * i,
			   state->bias);
	else 
	    for (i = 0; i < nfchans; i++)
		imdct_512 (samples + 256 * i, samples + 1536 + 256 * i,
			   state->bias);
    }

    return 0;
}
