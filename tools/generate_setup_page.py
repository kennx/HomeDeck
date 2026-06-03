Import("env")
import os
import subprocess
import sys

# PlatformIO SCons 环境中没有 __file__ 变量，通过 SCons 内置变量获取项目根目录
project_dir = env.subst("$PROJECT_DIR")
sync_script = os.path.join(project_dir, "tools", "sync_preview.py")

print("Executing automated HTML-to-C++ template synchronization...")
try:
    # 兼容在 platformio 的 python 环境中运行
    subprocess.check_call([sys.executable, sync_script])
except Exception as e:
    print(f"Error executing sync_preview.py: {e}")
    # 降级尝试直接运行 python3
    try:
        subprocess.check_call(["python3", sync_script])
    except Exception as e2:
        print(f"Fallback to python3 also failed: {e2}")
        sys.exit(1)
