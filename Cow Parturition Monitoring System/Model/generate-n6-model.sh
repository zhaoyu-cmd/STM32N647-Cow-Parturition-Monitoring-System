#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

MODEL_PATH="${1:-$SCRIPT_DIR/best_model_opset17_quant_qdq_pc.onnx}"
STEDGEAI_BIN="${STEDGEAI_BIN:-stedgeai}"

if [ ! -f "$MODEL_PATH" ]; then
    echo "[ERROR] Quantized YOLOv2 model not found: $MODEL_PATH" >&2
    exit 1
fi

if ! command -v "$STEDGEAI_BIN" >/dev/null 2>&1; then
    if [ -x "/c/ST/STEdgeAI/2.1/Utilities/windows/stedgeai.exe" ]; then
        STEDGEAI_BIN="/c/ST/STEdgeAI/2.1/Utilities/windows/stedgeai.exe"
    elif [ -x "/c/ST/STEdgeAI/2.1/Utilities/windows/stedgeai" ]; then
        STEDGEAI_BIN="/c/ST/STEdgeAI/2.1/Utilities/windows/stedgeai"
    else
        echo "[ERROR] stedgeai not found. Add it to PATH or set STEDGEAI_BIN." >&2
        exit 1
    fi
fi

"$STEDGEAI_BIN" generate --no-inputs-allocation --no-outputs-allocation --model "$MODEL_PATH" --target stm32n6 --st-neural-art default@user_neuralart.json
cp st_ai_output/network_ecblobs.h .
cp st_ai_output/network.c .
cp st_ai_output/network_atonbuf.xSPI2.raw network_data.xSPI2.bin
mkdir -p ../Binary
arm-none-eabi-objcopy -I binary network_data.xSPI2.bin --change-addresses 0x70200000 -O ihex ../Binary/network-data.hex
