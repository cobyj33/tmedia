#include <cstdlib>
#include "audio.h"
#include <cstddef>
#include <vector>

std::vector<float> audio_to_mono(std::vector<float>& samples, int nb_channels) {
  std::vector<float> res;
  for (std::size_t frame = 0; frame < samples.size() / (std::size_t)nb_channels; frame++) {
    float mono = 0.0;
    for (int c = 0; c < nb_channels; c++) {
      mono += samples[frame * nb_channels + c];
    }
    res.push_back(mono);
  }

  audio_normalize(res);
  return res;
}

void audio_normalize(std::vector<float>& samples) {
  float largest = 0.0;
  for (std::size_t i = 0; i < samples.size(); i++) {
    largest = std::max(samples[i], largest);
  }

  if (largest == 0.0)
    return;

  for (std::size_t i = 0; i < samples.size(); i++) {
    samples[i] /= largest;
  }
}

std::vector<float> alter_audio_len(std::vector<float>& original_samples, int nb_channels, size_t target_nb_samples) {
    size_t nb_samples = original_samples.size() / nb_channels;

    return original_samples;
    
    // if (nb_samples == target_nb_samples) {
    //   return original_samples;
    // } else if (nb_samples < target_nb_samples) {
    //   float** finalValues = alloc_samples(linesize, target_nb_samples, nb_channels);
    //   double stretchAmount = (double)target_nb_samples / nb_samples;
    //   float orignalSampleBlocks[nb_samples][nb_channels];
    //   float finalSampleBlocks[target_nb_samples][nb_channels];
    //   for (int i = 0; i < nb_samples * nb_channels; i++) {
    //     orignalSampleBlocks[i / nb_channels][i % nb_channels] = original_samples[0][i];
    //   }

    //   float currentSampleBlock[nb_channels];
    //   float lastSampleBlock[nb_channels];
    //   float currentFinalSampleBlockIndex = 0;

    //   for (int i = 0; i < nb_samples; i++) {
    //     for (int cpy = 0; cpy < nb_channels; cpy++) {
    //       currentSampleBlock[cpy] = orignalSampleBlocks[i][cpy];
    //     }

    //     if (i != 0) {
    //       for (int currentChannel = 0; currentChannel < nb_channels; currentChannel++) {
    //         float sampleStep = (currentSampleBlock[currentChannel] - lastSampleBlock[currentChannel]) / stretchAmount; 
    //         float currentSampleValue = lastSampleBlock[currentChannel];

    //         for (int finalSampleBlockIndex = (int)(currentFinalSampleBlockIndex); finalSampleBlockIndex < (int)(currentFinalSampleBlockIndex + stretchAmount); finalSampleBlockIndex++) {
    //           finalSampleBlocks[finalSampleBlockIndex][currentChannel] = currentSampleValue;
    //           currentSampleValue += sampleStep;
    //         }
    //       }
    //     }

    //     currentFinalSampleBlockIndex += stretchAmount;
    //     for (int cpy = 0; cpy < nb_channels; cpy++) {
    //         lastSampleBlock[cpy] = currentSampleBlock[cpy];
    //     }
    //   }

    //   for (int i = 0; i < target_nb_samples * nb_channels; i++) {
    //       finalValues[0][i] = finalSampleBlocks[i / nb_channels][i % nb_channels];
    //   }
    //   return finalValues;

    // } else if (nb_samples > target_nb_samples) {

    //   float** finalValues = alloc_samples(linesize, target_nb_samples, nb_channels);
    //   double stretchAmount = (double)target_nb_samples / nb_samples;
    //   float stretchStep = 1 / stretchAmount;
    //   float orignalSampleBlocks[nb_samples][nb_channels];
    //   float finalSampleBlocks[target_nb_samples][nb_channels];
    //   for (int i = 0; i < nb_samples * nb_channels; i++) {
    //       orignalSampleBlocks[i / nb_channels][i % nb_channels] = original_samples[0][i];
    //   }

    //   float currentOriginalSampleBlockIndex = 0;

    //   for (int i = 0; i < target_nb_samples; i++) {
    //       float average = 0.0;
    //       for (int ichannel = 0; ichannel < nb_channels; ichannel++) {
    //         average = 0.0;
    //         for (int origi = (int)(currentOriginalSampleBlockIndex); origi < (int)(currentOriginalSampleBlockIndex + stretchStep); origi++) {
    //             average += orignalSampleBlocks[origi][ichannel];
    //         }
    //         average /= stretchStep;
    //         finalSampleBlocks[i][ichannel] = average;
    //       }

    //       currentOriginalSampleBlockIndex += stretchStep;
    //   }

    //   for (int i = 0; i < target_nb_samples * nb_channels; i++) {
    //       finalValues[0][i] = finalSampleBlocks[i / nb_channels][i % nb_channels];
    //   }
    //   return finalValues;

    // }

    // //unreachable
    // return {};
}