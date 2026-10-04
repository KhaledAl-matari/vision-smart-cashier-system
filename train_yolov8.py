# train_yolov8.py
from ultralytics import YOLO

def train_model():
    model = YOLO("yolov8n.pt")

    model.train(
        data="dataset/data.yaml",
        epochs=80,
        patience=10,
        imgsz=640,
        batch=16,
        optimizer="AdamW",
        lr0=0.001,
        weight_decay=0.0005,
        cos_lr=True,

        # Augmentations
        hsv_h=0.015,
        hsv_s=0.7,
        hsv_v=0.4,
        degrees=10,
        translate=0.1,
        scale=0.5,
        shear=2,
        fliplr=0.5,
        mosaic=1.0,
        mixup=0.1,

        workers=4,
        device=0,      # استخدام GPU
        verbose=True
    )

if __name__ == "__main__":
    train_model()
