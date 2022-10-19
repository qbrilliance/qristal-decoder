# decoder


## Description
These are control scripts for the quantum decoder and its variants. 
The function of the decoder is to take a probability table, indicating the probability of each symbol in an alphabet at each of a set of timesteps, and find the most likely intended output. 
The probability table is provided and comes from a classical convolutional neural network as part of a compound application, where our focus has been on a speech-to-text application. The intended outputs are not the strings naively found from the probability table but require a two-step process:
1. Contract all repetitions within the string down to one symbol,
2. Remove all _null_ characters.
Strings which return the same output after this process may be deemed equivalent and form equivalence classes which we call _beams_.
### The full decoder
The full decoder sorts the input strings into their respective beams before returning the most probable.
To find the probabilities of each beam, the decoder finds the probabilities of each string in the beam and adds them. It then uses repeated Grover searches to magnify the highest probability beam as high as possible so that it is returned upon quantum measurement
### Simplified decoder
In order to reduce the scaling of the gate depth and to have a relevant application demonstrable to clients we have also put together a simplified version of the decoder which does not identify the beams but simply encodes the strings with probability amplitudes representative of the input probability table. The probability of any given string being returned upon measurement matches that expected from the probability table. Similarly for the probability of the returned string belonging to a given beam. The beam to which the returned string belongs is determined classically post-measurement. This simplified approach does not attempt to return the highest probability string or beam with certainty, _i.e._ there is no amplitude amplification of the highest probability string/beam.

## Tests
Test information may be added here

## Project status
This is a branch of the SDK-core

***
