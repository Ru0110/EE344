# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/opt/pico/pico-sdk/tools/pioasm"
  "/home/shob/Desktop/acads/EE344/Software/RP2040_code/build/pioasm"
  "/home/shob/Desktop/acads/EE344/Software/RP2040_code/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm"
  "/home/shob/Desktop/acads/EE344/Software/RP2040_code/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/tmp"
  "/home/shob/Desktop/acads/EE344/Software/RP2040_code/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/PioasmBuild-stamp"
  "/home/shob/Desktop/acads/EE344/Software/RP2040_code/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src"
  "/home/shob/Desktop/acads/EE344/Software/RP2040_code/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/PioasmBuild-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/shob/Desktop/acads/EE344/Software/RP2040_code/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/PioasmBuild-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/shob/Desktop/acads/EE344/Software/RP2040_code/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/PioasmBuild-stamp${cfgdir}") # cfgdir has leading slash
endif()
