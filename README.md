# QB SDK - Decoder
---
### Important change announcement for Qristal users:

>
> **Qristal is moving to GitHub**
>
> [https://github.com/qbrilliance/qristal](https://github.com/qbrilliance/qristal)
>
> We are making this change to engage more effectively with the developer community.
>
>Please read the following information, as you will need to take action as soon as possible to minimise any potential impact to your development workflow.
>
> **When is the change happening?**
>
> Qristal is available [from GitHub](https://github.com/qbrilliance/) **now**.
>
> On **Friday, 20 October 2023**,  public access will be removed from the existing GitLab repository.
>

#### If I have cloned the Qristal Decoder, how do I set the new GitHub repository as the `remote`?
```
git remote remove origin
git remote add origin https://github.com/qbrilliance/qristal-decoder.git
git fetch
git branch --set-upstream-to=origin/main main
```

#### If I have used the Qristal Decoder as a git submodule of my project, how do I update this to use the new GitHub repository?
```
git rm your/project/path/to/Qristal-Decoder
git submodule add https://github.com/qbrilliance/qristal-decoder.git your/project/path/to/Qristal-Decoder
git submodule update --init --recursive
```

#### What will happen to my local repository if I don’t take any action?

Your local repository will no longer be in sync with newer releases of Qristal.

#### Who can I contact if I have questions about the change to GitHub?

Please raise any questions with:

Simon Yin (Developer Relations, <simon.y@quantum-brilliance.com>)



---


## Description
These are control scripts for the quantum decoder and its variants.
The function of the decoder is to take a probability table, indicating the probability of each symbol in an alphabet at each of a set of timesteps, and find the most likely intended output.
The probability table is provided and comes from a classical convolutional neural network as part of a compound application, where our focus has been on a speech-to-text application. The intended outputs are not the strings naively found from the probability table but require a two-step process:
1. Contract all repetitions within the string down to one symbol,
2. Remove all _null_ characters.
Strings which return the same output after this process may be deemed equivalent and form equivalence classes which we call _beams_.
### The full decoder
The full decoder sorts the input strings into their respective beams before returning the most probable.
To find the probabilities of each beam, the decoder finds the probabilities of each string in the beam and adds them. It then uses repeated Grover searches to magnify the highest probability beam as much as possible, so that it is returned upon quantum measurement.
This is a complicated algorithm with multiple parts, which we summarize as follows:
1. At each timestep the decoder transcribes the probability data to the appropriate quantum register and then clears the input register for the next timestep.
2. The probabilities are encoded as logarithms so that multiplying them can be effected by addition, which is easier. The decoder finds the logarithm of the probability of each string and uses the exponent module to find the actual probability for later summation.
3. Strings are sorted into their respective beams by the _decoder kernel_. Since a quantum algorithm cannot remove symbols from a string it relocates them to the end of the string and flags those symbols that have been moved as being either repetitions of the previous symbol, or _null_. The original positions of those symbols are flagged separately.
4. The next step is to sum the probability values of each string within each beam class. The complications of this step include determining the relative size of each beam class and adjusting the outcome accordingly.
5. The final step is an exponential search, which is implemented in a module of the SDK core. This amplifies the amplitude of those beams whose probability is greater than the previous measurement, or zero the first time. Measuring then gives a new value to compare to until the maximum possible value is found or a set number of loops have been run.
### Simplified decoder
In order to reduce the scaling of the gate depth and to have a relevant application demonstrable to clients, we have also put together a simplified version of the decoder. This does not identify the beams but simply encodes the strings with probability amplitudes representative of the input probability table. The probability of any given string being returned upon measurement matches that expected from the probability table. Similarly for the probability of the returned string belonging to a given beam. The beam to which the returned string belongs is determined classically post-measurement. This simplified approach does not attempt to return the highest probability string or beam with certainty, _i.e._ there is no amplitude amplification of the highest probability string/beam.

## Tests
CI tests are included for both decoders and for the quantum kernel. However, the user is warned that those for the full decoder and the decoder kernel can take an excessive amount of time to run, depending on the hardware being used. 

## License
[Apache 2.0](LICENSE)
