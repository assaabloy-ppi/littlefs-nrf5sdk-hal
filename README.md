# littlefs nRF5 SDK HAL

This repository includes an implementation for the hardware abstracted parts of littlefs. The implementation is for use with the nRF5 SDK, and uses the fstorage API for low level flash operations. The fstorage API is available in the nRF5 SDK.

## Features

This section discusses the features and limitations of this implementation.
### Memory
The implementation supports both dynamic and static memory allocation. The default behavior is using dynamic memory. Defining `LFS_NO_MALLOC` in your project ensures that only static memory is used. For dynamic memory, the implementation uses the memory manger in the nRF5 SDK. For static memory, the implementation provides static buffers to the littlefs API. Note, the user must provide their own static buffer in the `lfs_file_t` struct (the buffer in `lfs_file_config`) when opening files using littlefs. An example of this is available in this repository. The size of the buffer must be `LFS_NRF52_CACHE_SIZE`.

### SoftDevice
The implementation supports running with the SoftDevice. This is automatically handled in the implementation. The user should just define `SOFTDEVICE_PRESENT` in their project if there is a SoftDevice present.

### Limitations
- Logging output from littlefs can only be achieved with RTT, not UART.
- This implementation has currently only been tested on the nRF52840-DK, with the SoftDevice S140 present, on nRF5 SDK v17.0.2.
- Implementation can be considered as still a work in progress
- Currently only supports the internal flash memory of the nRF SoC:s.

## Usage
This section discusses how to configure this API for your project. The repository should be cloned recursively for inclusion of littlefs as a submodule to this repository. This HAL implementation has only been tested with the version of littlefs which is set as a submodule.

### Linking and including
`lfs_nrf52_hal.c` and `littlefs/lfs.c` must be linked to your project. The root directory of this repository and the littlefs directory must be included in your project. Furthermore, there are dependencies of the nRF5 SDK. They are currently not listed in this README, but it should be straight forward link and include the correct files and directories.

The direct dependencies from the nRF5 SDK are fstorage, mem_manager (when not using static memory), app_error for asserts, and Segger RTT for logging (which is only enabled if `NRF_LOG_ENABLED` and `NRF_LOG_BACKEND_RTT_ENABLED` are defined in your project).

### Defines
One **mandatory** define in your project is `LFS_CONFIG`, and it must be set to `lfs_nrf52_config.h`. To clarify, `-DLFS_CONFIG=lfs_nrf52_config.h`.

These defines are used to configure the littlefs API.
| Define                        | Default                |
| -----------                   | --------               |
| LFS_NRF52_CACHE_SIZE          | 16                     |
| LFS_NRF52_START_ADDR          | 0x3e000                |
| LFS_NRF52_END_ADDR            | 0x3ffff                |
| LFS_NRF52_READ_SIZE           | 16                     |
| LFS_NRF52_PROG_SIZE           | 16                     |
| LFS_NRF52_BLOCK_SIZE          | 4096                   |
| LFS_NRF52_BLOCK_COUNT         | 2                      |
| LFS_NRF52_LOOKAHEAD_SIZE      | 16                     |
| LFS_NRF52_BLOCK_CYCLES        | 500                    |
| LFS_NRF52_NAME_MAX            | Not assigned, optional |
| LFS_NRF52_FILE_MAX            | Not assigned, optional |
| LFS_NRF52_ATTR_MAX            | Not assigned, optional |
| LFS_NRF52_METADATA_MAX        | Not assigned, optional |

Keep in mind that it is the user's responsibility to ensure that the address range is valid. This should depend on how much storage space you want to allocate for your file system, the size of your flash storage, application, and your potential usage of a bootloader and the a SoftDevice.

## Contributing
Submit a pull request to this repository to contribute to the project.
