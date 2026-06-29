#!/bin/bash
set -e

ProjectDir="$(cd "$(dirname "$0")" && pwd)"
ToolRoot="$1"
if command -v cygpath >/dev/null 2>&1; then
    ToolRoot="$(cygpath -m "$ToolRoot")"
fi

STM32SigningTool="$ToolRoot/STM32_SigningTool_CLI.exe"
if [ ! -f "$STM32SigningTool" ]; then
    STM32SigningTool="$ToolRoot/STM32_SigningTool_CLI"
fi
if [ ! -f "$STM32SigningTool" ]; then
    STM32SigningTool="D:/stm32cubeide/STM32CubeIDE_2.1.1/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506/tools/bin/STM32_SigningTool_CLI.exe"
fi

ProjectOut="$(find "$ProjectDir" -name "Cow_Parturition_Monitoring_System_FSBL.bin" -print -quit)"
if [ -z "$ProjectOut" ]; then
    ProjectOut="$(find "$ProjectDir" -name "*_FSBL.bin" -print -quit)"
fi

if [ -z "$ProjectOut" ] || [ ! -f "$ProjectOut" ]; then
    echo "[ERROR] Cannot find *_FSBL.bin under $ProjectDir" >&2
    exit 1
fi

ProjectConv="${ProjectOut%.*}-trusted.${ProjectOut##*.}"
rm -f "$ProjectConv"
"$STM32SigningTool" -bin "$ProjectOut" -nk -of 0x80000000 -t fsbl -o "$ProjectConv" -hv 2.3 -dump "$ProjectConv"
mkdir -p "$ProjectDir/../../Binary"
arm-none-eabi-objcopy -I binary "$ProjectConv" --change-addresses 0x70000000 -O ihex "$ProjectDir/../../Binary/fsbl.hex"
