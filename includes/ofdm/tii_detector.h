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

#ifndef	__TII_DETECTOR__
#define	__TII_DETECTOR__

#include	<cstdint>
#include	"dab-params.h"
#include	"fft-handler.h"
#include	<QList>
#include	<vector>

class	TII_Detector {
public:
		TII_Detector	(uint8_t dabMode, int16_t);
		~TII_Detector();
	void	reset();
	void	addBuffer	(std::vector<std::complex<double>>);
	std::vector<int>	processNULL();

private:
	void			collapse	(std::complex<double> *,
	                                         double *);
	int16_t			depth;
	uint8_t			invTable [256];
	dabParams		params;
	fftHandler		my_fftHandler;
	int16_t			T_u;
	int16_t			carriers;
	bool			ind;
	std::complex<double>	*fft_buffer;
	std::vector<complex<double> >	theBuffer;
	std::vector<double>	window;
	int16_t		fillCount;
};

#endif

