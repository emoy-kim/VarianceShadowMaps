# Variance Shadow Maps

  This code is about variance shadow maps from GPU Gems 3.
  
  Please refer to [this](https://jeesunkim.com/projects/gpu-gems/variance_shadow_maps/) to see the details.
  
![result_merged](https://github.com/emoy-kim/VarianceShadowMaps/assets/17864157/555c473d-79ec-4742-844d-4987f891ed0b)

## Keyboard Commands
  * **1 key**: select Percentage-Closer Filtering(PCF)
  * **2 key**: select Variance Shadow Map(VSM)
  * **3 key**: select Parallel-Split Variance Shadow Map(PSVSM)
  * **4 key**: select Summed Area Table Variance Shadow Map(SATVSM)
  * **l key**: toggle light effects
  * **c key**: capture the current frame
  * **d key**: capture the depth maps when _PSVSM is selected_
  * **s key**: capture the summed area table when _SATVSM is selected_
  * **SPACE key**: pause rendering
  * **q/ESC key**: exit
