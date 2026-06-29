#!/bin/bash
set -e

ProjectDir="$(cd "$(dirname "$0")" && pwd)"
ProjectOut="$(find "$ProjectDir" -name "Cow_Parturition_Monitoring_System_Appli.bin" -print -quit)"
if [ -z "$ProjectOut" ]; then
    ProjectOut="$(find "$ProjectDir" -name "*_Appli.bin" -print -quit)"
fi

if [ -z "$ProjectOut" ] || [ ! -f "$ProjectOut" ]; then
    echo "[ERROR] Cannot find *_Appli.bin under $ProjectDir" >&2
    exit 1
fi

mkdir -p "$ProjectDir/../../Binary"
arm-none-eabi-objcopy -I binary "$ProjectOut" --change-addresses 0x70100400 -O ihex "$ProjectDir/../../Binary/appli.hex"
