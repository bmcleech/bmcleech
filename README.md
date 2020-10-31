# Build Instructions

1. Install dependencies
  * Clone Facebook OpenBMC repository:
    ```
    git clone -b helium https://github.com/facebook/openbmc.git
    cd openbmc/
    ```
  * Use the commit we used for this project:
    ```
    git checkout d419a50e13d62591ab17d95713492be61033a2a5
    ```
  * Clone Yocto build system:
  ```
  ./sync_yocto.sh
  ```
2.  Update build config to use Linux kernel version 5.0
3. Initialize the build environment for the Facebook Tioga Pass (fbtp) hardware
   ```
   source openbmc-init-build-env meta-facebook/meta-fbtp
   ```
4. Let the build system create a Linux kernel tree and apply the patches
5. Update the kernel config to build the aspeed-xdma module
6. Build OpenBMC
```
bitbake fbtp-image
```
7. Verify that aspeed-xdma was built
