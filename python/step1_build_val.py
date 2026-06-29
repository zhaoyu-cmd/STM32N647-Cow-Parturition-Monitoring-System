"""
步骤1：构建独立验证/测试集

按“完整视频片段”划分数据集，避免同一 clip 同时出现在 train 和 val/test。
本脚本会把 SPLIT_PLAN 中列出的 clip 从 train 移动到目标 split，
而不是从 train 复制一份到 val。
"""

import shutil
from pathlib import Path

DATASET_ROOT = Path(r"D:\STMN647_myself\photo\dataset")

# 按视频片段划分：同一个 clip 只能属于一个 split。
# 需要新增独立测试集时，在 "test" 列表中填写另一个完整 clip 名称。
SPLIT_PLAN = {
    "val": ["720QQ2026512-145311-HD_0.5s"],
    "test": [],
}


def move_clip(clip_name: str, target_split: str) -> None:
    """Move one complete clip from train to target_split for images and labels."""
    for category in ["images", "labels"]:
        src = DATASET_ROOT / category / "train" / clip_name
        dst = DATASET_ROOT / category / target_split / clip_name

        if dst.exists() and src.exists():
            raise RuntimeError(
                f"检测到重复 split：{clip_name} 同时存在于 train 和 {target_split}: {dst}\n"
                "为避免训练/验证集泄漏，请先人工确认并删除其中一份，再重新运行。"
            )
        if dst.exists():
            print(f"[SKIP] {category}/{clip_name} 已在 {target_split}，train 中不存在")
            continue
        if not src.exists():
            raise FileNotFoundError(f"源目录不存在: {src}")

        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(src), str(dst))
        print(f"[OK] {category}: train/{clip_name} -> {target_split}/{clip_name}")


def collect_clips(category: str, split: str) -> set[str]:
    split_dir = DATASET_ROOT / category / split
    if not split_dir.exists():
        return set()
    return {p.name for p in split_dir.iterdir() if p.is_dir()}


def verify_no_overlap() -> None:
    """Fail fast if any clip name appears in train and an evaluation split."""
    for category in ["images", "labels"]:
        train_clips = collect_clips(category, "train")
        for split in ["val", "test"]:
            overlap = train_clips & collect_clips(category, split)
            if overlap:
                names = ", ".join(sorted(overlap))
                raise RuntimeError(f"{category} 存在训练/评估集重叠 clip: {names}")


def main() -> None:
    for split, clips in SPLIT_PLAN.items():
        for clip in clips:
            move_clip(clip, split)
    verify_no_overlap()
    print("\n独立验证/测试集划分完成：train 与 val/test 不再共享同一 clip。")


if __name__ == "__main__":
    main()
