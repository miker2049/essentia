#!/usr/bin/env python

# Copyright (C) 2006-2021  Music Technology Group - Universitat Pompeu Fabra
#
# This file is part of Essentia
#
# Essentia is free software: you can redistribute it and/or modify it under
# the terms of the GNU Affero General Public License as published by the Free
# Software Foundation (FSF), either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the Affero GNU General Public License
# version 3 along with this program. If not, see http://www.gnu.org/licenses/


from essentia_test import *

class TestCREPE(TestCase):

    def testRegression(self):
        expected_data = numpy.loadtxt(
            join(filedir(), 'crepe', 'vignesh.f0.csv'),
            delimiter=",",
            skiprows=1
        )
        expected_time = expected_data[:, 0]
        expected_frequency = expected_data[:, 1]
        expected_confidence = expected_data[:, 2]

        filename = join(testdata.audio_dir, 'recorded', 'vignesh.wav')
        model = join(testdata.models_dir, 'crepe', 'crepe-tiny-1.pb')

        audio = MonoLoader(filename=filename, sampleRate=16000)()
        # The last value returned by CREPE are the activations that are already
        # tested in test_tensorflowpredictcrepe.
        time, frequency, confidence, _ = CREPE(graphFilename=model)(audio)

        self.assertAlmostEqualVector(time, expected_time, 1e-5)
        self.assertAlmostEqualVector(frequency, expected_frequency, 1e-5)
        self.assertAlmostEqualVector(confidence, expected_confidence, 1e-3)

    def testEmptyModelName(self):
        # With empty or undefined model names the algorithm should skip the configuration
        # without errors.
        CREPE()
        CREPE(graphFilename='')
        CREPE(graphFilename='', input='')
        CREPE(graphFilename='', input='wrong_input')

    def testInvalidParam(self):
        model = join(testdata.models_dir, 'crepe', 'crepe-tiny-1.pb')
        self.assertConfigureFails(CREPE(), {'graphFilename': model,
                                            'input': 'wrong_input_name',
                                            'output': 'model/classifier/Sigmoid',
                                           })  # input do not exist in the model
        self.assertConfigureFails(CREPE(), {'graphFilename': 'wrong_model_name',
                                            'input': 'frames',
                                            'output': 'model/classifier/Sigmoid',
                                           })  # the model does not exist

suite = allTests(TestCREPE)

if __name__ == '__main__':
    TextTestRunner(verbosity=2).run(suite)
