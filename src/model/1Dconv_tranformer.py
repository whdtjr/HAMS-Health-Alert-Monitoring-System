import os, glob
import numpy as np
import pandas as pd
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import train_test_split

import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import Dataset, DataLoader, WeightedRandomSampler
from torch.amp import autocast, GradScaler
import gc
import psutil

class DrowsinessDataProcessor:
    FEATURE_COLS = [
        'time_sec', 'current_yawn_duration_sec', 'current_blink_duration_sec',
        'left_ear', 'right_ear', 'mouth_mar', 'mar_60s_rate', 'perclos_30s_rate'
    ]

    def __init__(self, seq_len=50):
        self.seq_len = seq_len
        self.scaler = StandardScaler()

    def _load_all_csv(self, path):
        csv_files = glob.glob(os.path.join(path, '*.csv'))
        if len(csv_files) == 0:
            raise FileNotFoundError(f'CSV 파일이 {path}에 없습니다.')
        
        data_list = []
        for f in csv_files:
            df = pd.read_csv(f)
            data_list.append(df)
            print(f"Loaded: {f} - Shape: {df.shape}")
        
        df_all = pd.concat(data_list, ignore_index=True)
        print(f"Total data shape: {df_all.shape}")
        return df_all

    def _fit_scaler(self, df):
        self.scaler.fit(df[self.FEATURE_COLS].values)

    def _make_sequences(self, df):
        feats = self.scaler.transform(df[self.FEATURE_COLS].values)
        labels = df['label'].values.astype(int)

        X, y = [], []
        # 메모리 효율적으로 시퀀스 생성
        for i in range(0, len(feats) - self.seq_len + 1, self.seq_len // 4):  # 오버랩 감소
            X.append(feats[i:i + self.seq_len])
            y.append(labels[i + self.seq_len - 1])
        
        return np.array(X, dtype=np.float32), np.array(y, dtype=np.int64)

    def preprocess_data(self, path='./sequence'):
        df = self._load_all_csv(path)
        if 'label' not in df.columns:
            raise ValueError('CSV 파일에 label 컬럼이 없습니다.')
        
        self._fit_scaler(df)
        X, y = self._make_sequences(df)
        print(f"Sequence data shape: X={X.shape}, y={y.shape}")

        X_tr, X_val, y_tr, y_val = train_test_split(
            X, y, test_size=0.2, random_state=42,
            stratify=y if len(np.unique(y)) > 1 else None
        )
        return X_tr, X_val, y_tr, y_val

# ---------------------------- GPU 최적화된 Dataset ---------------------------- #
class DrowsinessDataset(Dataset):
    def __init__(self, X, y, device=None):
        # 데이터를 미리 GPU로 옮기지 않고 필요할 때 이동
        self.X = X
        self.y = y
        self.device = device

    def __len__(self):
        return len(self.X)
    
    def __getitem__(self, idx):
        x = torch.tensor(self.X[idx], dtype=torch.float32)
        y = torch.tensor(self.y[idx], dtype=torch.long)
        return x, y

# ---------------------------- GPU 최적화된 모델 ---------------------------- #
class Conv1DLSTM(nn.Module):
    def __init__(self, num_features, conv_channels=64, kernel_size=3,
                 lstm_units=128, dropout=0.3, num_classes=2):
        super().__init__()
        
        # 더 효율적인 Conv1D 구조
        self.conv = nn.Sequential(
            nn.Conv1d(num_features, conv_channels, kernel_size, padding=kernel_size//2),
            nn.BatchNorm1d(conv_channels),
            nn.ReLU(inplace=True),
            nn.Dropout(dropout),
            
            nn.Conv1d(conv_channels, conv_channels, kernel_size, padding=kernel_size//2),
            nn.BatchNorm1d(conv_channels),
            nn.ReLU(inplace=True),
            nn.Dropout(dropout),
            
            nn.Conv1d(conv_channels, conv_channels*2, kernel_size, padding=kernel_size//2),
            nn.BatchNorm1d(conv_channels*2),
            nn.ReLU(inplace=True),
            nn.Dropout(dropout)
        )
        
        # LSTM with reduced complexity
        self.lstm = nn.LSTM(
            input_size=conv_channels*2, 
            hidden_size=lstm_units,
            batch_first=True, 
            dropout=dropout if dropout > 0 else 0,
            num_layers=1  # 레이어 수 고정
        )
        
        self.classifier = nn.Sequential(
            nn.Dropout(dropout),
            nn.Linear(lstm_units, 64),
            nn.ReLU(inplace=True),
            nn.Dropout(dropout),
            nn.Linear(64, num_classes)
        )

    def forward(self, x):
        # Conv1D 처리
        x = self.conv(x.permute(0, 2, 1))
        x = x.permute(0, 2, 1)
        
        # LSTM 처리
        lstm_out, _ = self.lstm(x)
        
        # 분류
        logits = self.classifier(lstm_out[:, -1, :])
        return logits

# --------------------------- GPU 최적화된 학습 함수 --------------------------- #
def get_optimal_batch_size(model, device, sample_input_shape):
    """GPU 메모리에 맞는 최적 배치 크기 찾기"""
    model.eval()
    batch_sizes = [64, 32, 16, 8, 4]
    
    for batch_size in batch_sizes:
        try:
            # 테스트 배치 생성
            test_input = torch.randn(batch_size, *sample_input_shape[1:]).to(device)
            with torch.no_grad():
                _ = model(test_input)
            del test_input
            torch.cuda.empty_cache()
            print(f"Optimal batch size found: {batch_size}")
            return batch_size
        except RuntimeError as e:
            if "out of memory" in str(e):
                continue
            else:
                raise e
    return 4  # 최소 배치 크기

def clear_memory():
    """메모리 정리"""
    gc.collect()
    if torch.cuda.is_available():
        torch.cuda.empty_cache()

def train_pytorch_model_optimized(seq_len=50, epochs=100, batch_size=None, lr=1e-3):
    # GPU 설정
    if not torch.cuda.is_available():
        print("Warning: CUDA not available, using CPU")
        device = torch.device('cpu')
    else:
        device = torch.device('cuda')
        print(f"Using GPU: {torch.cuda.get_device_name()}")
        print(f"GPU Memory: {torch.cuda.get_device_properties(0).total_memory / 1024**3:.1f} GB")

    # 전처리된 데이터 로드
    print("Loading preprocessed data...")
    try:
        X_tr = np.load('X_train.npy')
        X_val = np.load('X_val.npy')
        y_tr = np.load('y_train.npy')
        y_val = np.load('y_val.npy')
        print(f"Data loaded - Train: {X_tr.shape}, Val: {X_val.shape}")
    except FileNotFoundError:
        print("Error: Preprocessed data files not found.")
        return

    # 모델 초기화
    model = Conv1DLSTM(num_features=X_tr.shape[2]).to(device)
    
    # 최적 배치 크기 찾기 (지정되지 않은 경우)
    if batch_size is None:
        batch_size = get_optimal_batch_size(model, device, X_tr.shape)
    
    print(f"Using batch size: {batch_size}")

    # CPU 코어 수에 따른 num_workers 설정
    num_workers = min(4, psutil.cpu_count() // 2)  # 안전한 값 사용
    print(f"Using num_workers: {num_workers}")

    # 데이터셋 및 데이터로더
    train_ds = DrowsinessDataset(X_tr, y_tr)
    val_ds = DrowsinessDataset(X_val, y_val)

    # 클래스 불균형 처리
    class_sample_count = np.bincount(y_tr)
    if (class_sample_count > 0).all() and not np.allclose(class_sample_count, class_sample_count.mean()):
        weights = 1. / torch.tensor(class_sample_count, dtype=torch.float)
        samples_weight = weights[y_tr]
        sampler = WeightedRandomSampler(samples_weight, len(samples_weight))
        train_loader = DataLoader(
            train_ds, 
            batch_size=batch_size, 
            sampler=sampler,
            num_workers=num_workers,
            pin_memory=True if device.type == 'cuda' else False,
            persistent_workers=True if num_workers > 0 else False
        )
    else:
        train_loader = DataLoader(
            train_ds, 
            batch_size=batch_size, 
            shuffle=True,
            num_workers=num_workers,
            pin_memory=True if device.type == 'cuda' else False,
            persistent_workers=True if num_workers > 0 else False
        )

    val_loader = DataLoader(
        val_ds, 
        batch_size=batch_size,
        num_workers=num_workers,
        pin_memory=True if device.type == 'cuda' else False,
        persistent_workers=True if num_workers > 0 else False
    )

    # 최적화 설정
    criterion = nn.CrossEntropyLoss()
    optimizer = optim.AdamW(model.parameters(), lr=lr, weight_decay=1e-5)
    scheduler = optim.lr_scheduler.ReduceLROnPlateau(optimizer, factor=0.5, patience=7)
    
    # Mixed Precision Training
    scaler = GradScaler() if device.type == 'cuda' else None
    
    best_val_acc = 0
    
    for ep in range(1, epochs + 1):
        # Training phase
        model.train()
        tr_loss = tr_correct = tr_total = 0
        
        for batch_idx, (xb, yb) in enumerate(train_loader):
            xb, yb = xb.to(device, non_blocking=True), yb.to(device, non_blocking=True)
            
            optimizer.zero_grad()
            
            if scaler is not None:  # Mixed precision
                with autocast(device_type='cuda'):
                    logits = model(xb)
                    loss = criterion(logits, yb)
                
                scaler.scale(loss).backward()
                scaler.unscale_(optimizer)
                nn.utils.clip_grad_norm_(model.parameters(), 1.0)
                scaler.step(optimizer)
                scaler.update()
            else:  # Standard training
                logits = model(xb)
                loss = criterion(logits, yb)
                loss.backward()
                nn.utils.clip_grad_norm_(model.parameters(), 1.0)
                optimizer.step()
            
            tr_loss += loss.item()
            tr_total += yb.size(0)
            tr_correct += (logits.argmax(1) == yb).sum().item()
            
            # 메모리 정리 (배치마다)
            if batch_idx % 50 == 0:
                clear_memory()

        # Validation phase
        model.eval()
        vl_loss = vl_correct = vl_total = 0
        
        with torch.no_grad():
            for xb, yb in val_loader:
                xb, yb = xb.to(device, non_blocking=True), yb.to(device, non_blocking=True)
                
                if scaler is not None:
                    with autocast(device_type='cuda'):
                        logits = model(xb)
                        loss = criterion(logits, yb)
                else:
                    logits = model(xb)
                    loss = criterion(logits, yb)
                
                vl_loss += loss.item()
                vl_total += yb.size(0)
                vl_correct += (logits.argmax(1) == yb).sum().item()

        tr_acc = 100 * tr_correct / tr_total
        vl_acc = 100 * vl_correct / vl_total
        scheduler.step(vl_loss)

        # 최고 성능 모델 저장
        if vl_acc > best_val_acc:
            best_val_acc = vl_acc
            torch.save(model.state_dict(), 'best_drowsiness_model.pth')

        # GPU 메모리 상태 출력
        if device.type == 'cuda':
            gpu_memory = torch.cuda.memory_allocated() / 1024**3
            print(f'Epoch {ep:03d} | '
                  f'Train Loss {tr_loss/len(train_loader):.4f} Acc {tr_acc:.2f}% || '
                  f'Val Loss {vl_loss/len(val_loader):.4f} Acc {vl_acc:.2f}% || '
                  f'GPU Mem {gpu_memory:.2f}GB')
        else:
            print(f'Epoch {ep:03d} | '
                  f'Train Loss {tr_loss/len(train_loader):.4f} Acc {tr_acc:.2f}% || '
                  f'Val Loss {vl_loss/len(val_loader):.4f} Acc {vl_acc:.2f}%')
        
        # 주기적 메모리 정리
        if ep % 10 == 0:
            clear_memory()

    print(f'Best validation accuracy: {best_val_acc:.2f}%')

def preprocess_and_save(data_dir='./sequence', seq_len=50):
    print("Starting data preprocessing and saving...")
    processor = DrowsinessDataProcessor(seq_len=seq_len)
    X_tr, X_val, y_tr, y_val = processor.preprocess_data(data_dir)
    
    # 메모리 효율적 저장
    print("Saving preprocessed data...")
    np.save('X_train.npy', X_tr)
    np.save('X_val.npy', X_val)  
    np.save('y_train.npy', y_tr)
    np.save('y_val.npy', y_val)
    print("Preprocessing complete. Data saved to .npy files.")
    
    # 메모리 정리
    del X_tr, X_val, y_tr, y_val
    clear_memory()

if __name__ == '__main__':
    dataset_path = './sequence'
    seq_len = 50
    
    # GPU 정보 출력
    if torch.cuda.is_available():
        print(f"CUDA Version: {torch.version.cuda}")
        print(f"PyTorch Version: {torch.__version__}")
        print(f"Available GPUs: {torch.cuda.device_count()}")
        for i in range(torch.cuda.device_count()):
            print(f"GPU {i}: {torch.cuda.get_device_name(i)}")
    
    # 1. 전처리 및 저장 (필요한 경우만)
    if not all(os.path.exists(f) for f in ['X_train.npy', 'X_val.npy', 'y_train.npy', 'y_val.npy']):
        preprocess_and_save(data_dir=dataset_path, seq_len=seq_len)
    
    # 2. GPU 최적화된 학습 시작
    train_pytorch_model_optimized(
        seq_len=seq_len,
        epochs=100,
        batch_size=64,  # 자동으로 최적 크기 찾기
        lr=1e-3
    )
