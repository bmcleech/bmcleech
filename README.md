# Paper and Presentation
* [Original Paper](https://www.sciencedirect.com/science/article/pii/S2666281720300147)
* [Presentation at DFRWS EU 2020](https://www.youtube.com/watch?v=e-dJC6enmf0)

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
4. Let the build system create a Linux kernel tree and apply the patches. The patches for the Aspeed XDMA Engine Driver are taken from [the LKML](https://lore.kernel.org/lkml/1562010839-1113-1-git-send-email-eajames@linux.ibm.com/).
5. Update the kernel config to build the aspeed-xdma module
6. Build OpenBMC
  ```
  bitbake fbtp-image
  ```
7. Verify that aspeed-xdma was built
