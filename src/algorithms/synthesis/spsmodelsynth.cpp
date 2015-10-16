/*
 * Copyright (C) 2006-2013  Music Technology Group - Universitat Pompeu Fabra
 *
 * This file is part of Essentia
 *
 * Essentia is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by the Free
 * Software Foundation (FSF), either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the Affero GNU General Public License
 * version 3 along with this program.  If not, see http://www.gnu.org/licenses/
 */

#include "spsmodelsynth.h"
#include "essentiamath.h"


using namespace essentia;
using namespace standard;


const char* SpsModelSynth::name = "SpsModelSynth";
const char* SpsModelSynth::description = DOC("This algorithm computes the stochastic model synthesis from stochastic model analysis.");


// configure
void SpsModelSynth::configure()
{
  _sampleRate = parameter("sampleRate").toReal();
  _fftSize = parameter("fftSize").toInt();
  _hopSize = parameter("hopSize").toInt();

  _sineModelSynth->configure( "sampleRate", parameter("sampleRate").toReal(),
                            "fftSize", parameter("fftSize").toInt(),
                            "hopSize", parameter("hopSize").toInt()
                            );

  // resample for stochastic envelope
  int stocSize = int (parameter("fftSize").toInt() * parameter("stocf").toReal());
  _fft->configure( "FFT",
                            "size", stocSize);
  _ifft->configure("IFFT",
                            "size", parameter("fftSize").toInt());

}


void SpsModelSynth::compute() {

  const std::vector<Real>& magnitudes = _magnitudes.get();
  const std::vector<Real>& frequencies = _frequencies.get();
  const std::vector<Real>& phases = _phases.get();
  const std::vector<Real>& stocenv = _stocenv.get();

  std::vector<std::complex<Real> >& outfft = _outfft.get();
  std::vector<std::complex<Real> > fftStoc;

  int outSize = (int)floor(_fftSize/2.0) + 1;
  initializeFFT(outfft, outSize);
  int i = 0;

  _sineModelSynth->input("magnitudes").set(magnitudes);
  _sineModelSynth->input("frequencies").set(frequencies);
  _sineModelSynth->input("phases").set(phases);
  _sineModelSynth->output("fft").set(outfft);

  _sineModelSynth->compute();

  stochasticModelSynth(stocenv, parameter("hopSize").toInt(), outSize, fftStoc);          //# synthesize stochastic residual

  // mix stoachastic and sinusoidal components
  for (i = 0; i < (int)outfft.size(); ++i)
  {
     outfft[i].real( outfft[i].real() + fftStoc[i].real());
     outfft[i].imag( outfft[i].imag() + fftStoc[i].imag());
  }
}

void SpsModelSynth::stochasticModelSynth(const std::vector<Real> stocEnv, const int H, const int N, std::vector<std::complex<Real> > &fftStoc)
{
//	"""
//	Stochastic synthesis of a sound
//	stocEnv: stochastic envelope; H: hop size; N: fft size (postive??)
//	returns y: output FFT magitude
//	"""


	int hN = (N/2.)+1;                                            		//# positive size of fft
	//hN = N; // if FFT size is only the positive size. Check!

  // init stochastic FFT
  fftStoc.resize(N);

// orignial python code
//		mY = resample(stocEnv[:], hN)                        # interpolate to original size
//		pY = 2* PI *np.random.rand(hN)                        # generate phase random values
//		Y = np.zeros(N, dtype = complex)                       # initialize synthesis spectrum
//		Y[:hN] = 10.f^(mY/20.) * np.exp(1j*pY)                   # generate positive freq.


// New c++ code: WIP
  Real magdB;
  Real phase;

  for (int i = 0; i < hN; ++i)
  {
    phase = 2 * M_PI *  Real(rand()/Real(RAND_MAX));
    magdB = 1; //resample(stocEnv, x);

    // positive spectrums
    fftStoc[i].real( powf(10.f, (magdB / 20.f)) * cos(phase) ) ;
    fftStoc[i].imag( powf(10.f, (magdB / 20.f)) * sin(phase) ) ;
    // negative spectrums
    fftStoc[N-i-1].real( powf(10.f, (magdB / 20.f)) * cos(phase) ) ;
    fftStoc[N-i-1].imag( powf(10.f, (magdB / 20.f)) * sin(phase) ) ;

  }

}

void SpsModelSynth::initializeFFT(std::vector<std::complex<Real> >&fft, int sizeFFT)
{
  fft.resize(sizeFFT);
  for (int i=0; i < sizeFFT; ++i){
    fft[i].real(0);
    fft[i].imag(0);
  }
}

// function to resample based on the FFT
// Use the same function than in python code
// http://docs.scipy.org/doc/scipy/reference/generated/scipy.signal.resample.html
void SpsModelSynth::resample(const std::vector<float> in, std::vector<float> &out, const int sizeOut)
{
  std::vector<std::complex<Real> >&fftin; // temp vectors
  std::vector<std::complex<Real> >&fftout; // temp vectors

  fft->input("frame").set(in);
  fft->output("fft").set(fftin);
  int sizeIn = (int) fftin.size();

  initializeFFT(fftout, sizeOut);

  int hN = (sizeIn/2.)+1;
  for (int i = 0; i < hN; ++i)
  {
    // positive spectrums
    fftout[i].real( fftin[i].real());
    fftout[i].imag( fftin[i].imag());
    // negative spectrums
    fftout[sizeOut-i-1].real(fftin[i].real());
    fftout[sizeOut-i-1].imag(fftin[i].imag());
  }

  ifft->input("fft").set(fftout); // taking SpsModelSynth output
  ifft->output("frame").set(out);

  fft->compute();
  ifft->compute();


}
