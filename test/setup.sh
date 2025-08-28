#!/bin/bash
# ================================
# Setup và chạy Flask App
# ================================

# 1️⃣ Tạo virtual environment (nếu chưa có)
if [ ! -d "venv" ]; then
    python3 -m venv venv
fi

# 2️⃣ Activate venv
source venv/bin/activate

# 3️⃣ Nâng cấp pip
python -m pip install --upgrade pip

# 4️⃣ Cài dependencies
python -m pip install flask requests feedparser

# 5️⃣ Chạy app
python app.py

