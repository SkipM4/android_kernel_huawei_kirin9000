# Huawei Mate 40 Pro Kernel Source For HarmonyOS 3.0.0

- **芯片代号**: Baltimore
- **设备代号**: Noah
- **内核版本**: 5.10.43

## 编译配置

- `merge_baltimore_defconfig`: 这是官方的配置文件，作为参考，不做改动。
- `new_baltimore_defconfig`: 新增编译配置文件，实验性质。

## 编译说明
建议使用 **Debian 11** 作为编译环境，配置以下工具并进行编译：

1. **Clang**:
   - [clang-r353983c](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+archive/refs/heads/android10-release/clang-r353983c.tar.gz)

2. **GCC**:
   - [gcc-aarch64-linux-android-4.9](https://android.googlesource.com/platform/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/+archive/refs/tags/android-12.1.0_r27.tar.gz)