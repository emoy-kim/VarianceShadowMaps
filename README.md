# Variance Shadow Maps

  This code is about variance shadow maps from GPU Gems 3.
  
  Please refer to [this](https://jeesunkim.com/projects/gpu-gems/ambient_occlusion/) to see the details.
  
![result_merged](https://github.com/emoy-kim/AmbientOcclusion/assets/17864157/7ddd4432-5cb6-40e5-bb0d-05c21e444977)

## Keyboard Commands
  * **1 key**: select Percentage-Closer Filtering(PCF)
  * **2 key**: select Variance Shadow Map(VSM)
  * **3 key**: select Parallel-Split Variance Shadow Map(PSVSM)
  * **4 key**: select Summed Area Table Variance Shadow Map(SATVSM)
  * **l key**: toggle light effects
  * **c key**: capture the current frame
  * **d key**: capture the depth maps when _PSVSM is selected_
  * **s key**: capture the summed area table when _SATVSM_ is selected_
  * **SPACE key**: pause rendering
  * **q/ESC key**: exit
