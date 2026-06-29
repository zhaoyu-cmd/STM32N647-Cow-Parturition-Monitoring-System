"""
步骤2：标签清洗
原始类别映射（来自 classes.txt）：
  0 = 羊水泡  → 删除（无标注）
  1 = sac     → 重映射为 0
  2 = hoof    → 重映射为 1
  3 = psac    → 重映射为 2
  4 = w       → 删除（仅1个标注）
  5 = d       → 删除

清洗范围：train 和 val 目录下的所有 frame_*.txt 标注文件。
classes.txt 文件不做修改（不是标注文件）。
"""

from pathlib import Path

DATASET_ROOT = Path(r"D:\STMN647_myself\photo\dataset")

REMAP = {1: 0, 2: 1, 3: 2}
DELETE_IDS = {0, 4, 5}

def clean_label_file(filepath: Path):
    lines = filepath.read_text(encoding="utf-8").splitlines()
    new_lines = []
    for line in lines:
        line = line.strip()
        if not line:
            continue
        parts = line.split()
        try:
            cls_id = int(parts[0])
        except (ValueError, IndexError):
            continue
        if cls_id in DELETE_IDS:
            continue
        if cls_id in REMAP:
            parts[0] = str(REMAP[cls_id])
            new_lines.append(" ".join(parts))
    filepath.write_text("\n".join(new_lines) + ("\n" if new_lines else ""), encoding="utf-8")
    return len(lines), len(new_lines)

stats = {"processed": 0, "kept": 0, "removed": 0}

for split in ["train", "val"]:
    label_dir = DATASET_ROOT / "labels" / split
    txt_files = [f for f in label_dir.rglob("frame_*.txt")]
    for f in txt_files:
        before, after = clean_label_file(f)
        stats["processed"] += 1
        stats["kept"] += after
        stats["removed"] += (before - after)

print(f"清洗完成:")
print(f"  处理文件数: {stats['processed']}")
print(f"  保留标注行: {stats['kept']}")
print(f"  删除标注行: {stats['removed']}")
print(f"\n最终类别:")
print(f"  0 = sac")
print(f"  1 = hoof")
print(f"  2 = psac")
