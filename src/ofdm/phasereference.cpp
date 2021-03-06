#
/*
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the dab-scanner
 *    dab-scanner is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    dab-scanner is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dab-scanner; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	"phasereference.h" 
#include	"string.h"
#include	"radio.h"
/**
  *	\class phaseReference
  *	Implements the correlation that is used to identify
  *	the "first" element (following the cyclic prefix) of
  *	the first non-null block of a frame
  *	The class inherits from the phaseTable.
  */

	phaseReference::phaseReference (RadioInterface *mr,
	                                uint8_t		dabMode,
	                                int16_t		threshold) :
	                                     phaseTable (dabMode),
	                                     params (dabMode),
	                                     my_fftHandler (dabMode) {
int32_t	i;
double	Phi_k;

	(void)mr;
	this	-> threshold	= threshold;
	this	-> diff_length	= 40;
	this	-> T_u		= params. get_T_u ();
	this	-> carriers	= params. get_carriers ();

	refTable.		resize (T_u);
	phaseDifferences.	resize (diff_length);
	fft_buffer		= my_fftHandler. getVector ();

	framesperSecond		= 2048000 / params. get_T_F ();
	displayCounter		= 0;

	for (i = 0; i < T_u; i ++)
	   refTable [i] = std::complex<double> (0, 0);

	for (i = 1; i <= params. get_carriers () / 2; i ++) {
	   Phi_k =  get_Phi (i);
	   refTable [i] = std::complex<double> (cos (Phi_k), sin (Phi_k));
	   Phi_k = get_Phi (-i);
	   refTable [T_u - i] = std::complex<double> (cos (Phi_k), sin (Phi_k));
	}
//
//	prepare a table for the coarse frequency synchronization
//	can be a static one, actually, we are only interested in
//	the ones with a null
	shiftFactor	= this -> diff_length / 4;
	for (i = 0; i < diff_length; i ++) {
	   phaseDifferences [i] = abs (arg (refTable [(T_u - shiftFactor + i) % T_u] *
                                       conj (refTable [(T_u - shiftFactor + i + 1) % T_u])));
	   phaseDifferences [i] *= phaseDifferences [i];
	}
}

	phaseReference::~phaseReference (void) {
}

/**
  *	\brief findIndex
  *	the vector v contains "T_u" samples that are believed to
  *	belong to the first non-null block of a DAB frame.
  *	We correlate the data in this vector with the predefined
  *	data, and if the maximum exceeds a threshold value,
  *	we believe that that indicates the first sample we were
  *	looking for.
  */

int32_t	phaseReference::findIndex (std::vector <std::complex<double>> v,
	                           int threshold) {
int32_t	i;
int32_t	maxIndex	= -1;
double	sum		= 0;
double	Max		= -1000;
double	lbuf [T_u];

	for (i = 0; i < T_u; i ++)
	   fft_buffer [i] = v [i];

	my_fftHandler. do_FFT ();
//
//	into the frequency domain, now correlate
	for (i = 0; i < T_u; i ++) 
	   fft_buffer [i] *= conj (refTable [i]);
//	and, again, back into the time domain
	my_fftHandler. do_IFFT ();
/**
  *	We compute the average and the max signal values
  */
	for (i = 0; i < T_u; i ++) {
	   lbuf [i] = abs (fft_buffer [i]);
	   sum	+= lbuf [i];
	   if (lbuf [i] > Max) {
	      maxIndex = i;
	      Max = lbuf [i];
	   }
	}

/**
  *	that gives us a basis for defining the actual threshold value
  */
	if (Max < threshold * sum / T_u)
	   return  - abs (Max * T_u / sum) - 1;
	else {
	   return maxIndex;	
	}
}
//
//	we look at the phasediferences of subsequent carriers,
//	these should match with those in the reference tablw
//	Since the row to match contains 0, PI/2 PI ..
//	we do two measurements
//	a. we look at the differences of the squared values
//	b. since there are a lot of 0 values in the refTable
//	   we look at the corresponding values
//	if both measurements give the same result, we are
//	confident that we have found the right index
#define	SEARCH_RANGE	(2 * 35)
int16_t	phaseReference::estimate_CarrierOffset (std::vector<std::complex<double>> v) {
int16_t	i, j, index_1 = 100, index_2 = 100;
double	computedDiffs [SEARCH_RANGE + diff_length + 1];

	for (i = 0; i < T_u; i ++)
	   fft_buffer [i] = v [i];
	my_fftHandler. do_FFT();

	for (i = T_u - SEARCH_RANGE / 2;
	     i < T_u + SEARCH_RANGE / 2 + diff_length; i ++) 
	   computedDiffs [i - (T_u - SEARCH_RANGE / 2)] =
	      arg (fft_buffer [(i - shiftFactor) % T_u] *
	                  conj (fft_buffer [(i - shiftFactor + 1) % T_u]));

	for (i = 0; i < SEARCH_RANGE + diff_length; i ++)
	   computedDiffs [i] *= computedDiffs [i];

	float	Mmin_1 = 10000;
	float	Mmin_2 = 10000;
	for (i = T_u - SEARCH_RANGE / 2;
	     i < T_u + SEARCH_RANGE / 2; i ++) {
	   int sum_1 = 0;
	   int sum_2 = 0;
	   for (j = 0; j < diff_length; j ++) {
	      if (phaseDifferences [j] < 0.05)
	         sum_1 += computedDiffs [i - (T_u - SEARCH_RANGE / 2) + j];
	      sum_2 += abs (computedDiffs [i - (T_u - SEARCH_RANGE / 2) + j] -
	                                           phaseDifferences [j]); 
	   }                            
	   if (sum_1 < Mmin_1) {
	      Mmin_1 = sum_1;
	      index_1 = i;
	   }
	   if (sum_2 < Mmin_2) {
	      Mmin_2 = sum_2;
	      index_2 = i;
	   }
	}

	return index_1 == index_2 ? index_1 - T_u : 100;
}
//	NOT USED, just for some tests
//	An alternative way to compute the small frequency offset
//	is to look at the phase offset of subsequent carriers
//	in block 0, compared to the values as computed from the
//	reference block.
//	The values are reasonably close to the values computed
//	on the fly
#define	LLENGTH	100
double	phaseReference::estimate_FrequencyOffset (std::vector <std::complex<double>> v) {
int16_t	i;
double pd	= 0;

	for (i = - LLENGTH / 2 ; i < LLENGTH / 2; i ++) {
	   std::complex<double> a1 = refTable [(T_u + i) % T_u] * conj (refTable [(T_u + i + 1) % T_u]);
	   std::complex<double> a2 = fft_buffer [(T_u + i) % T_u] * conj (fft_buffer [(T_u + i + 1) % T_u]);
	   pd += arg (a2) - arg (a1);
	}
	return pd / LLENGTH;
}

