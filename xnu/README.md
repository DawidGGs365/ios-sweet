# XNU build

Built on GitHub's macOS runners by `.github/workflows/xnu.yml`, using
[blacktop/darwin-xnu-build](https://github.com/blacktop/darwin-xnu-build) (MIT) at a pinned commit.

- `MACOS_VERSION=13.5` gives xnu-8796, the iOS 16 generation that matches the iOS 16.7.16 userland.
- `patches/*.patch` are applied in order with `git apply`. The `SWEET` machine config serves both
  `sweet` (Redmi Note 10 Pro) and `guacamole` (OnePlus 7 Pro); drivers pick the board at runtime.
- The output is the `kernel.development.sweet` Mach-O, uploaded as the `xnu-kernel` artifact.

Dispatch (a push does not start a build; a new dispatch cancels the running one):

    gh workflow run xnu.yml -R DawidGGs365/ios-sweet --ref main
    gh run download RUN_ID -R DawidGGs365/ios-sweet -n xnu-kernel -D xnu/out

For guacamole, `scripts/build-test.sh` in unixnu-guacamole does the dispatch, the download and the boot image.
