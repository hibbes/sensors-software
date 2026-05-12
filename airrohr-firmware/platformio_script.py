Import("env")
import hashlib
import os
import shutil
import subprocess
import sys

PROJECT_DIR = env["PROJECT_DIR"]

def _file_md5_hexdigest(fname):
    return hashlib.md5(open(fname, 'rb').read()).hexdigest()

def embed_assets_if_stale():
    """Issue #18 Phase D: Re-generate web/assets_generated.{h,cpp}
    aus web/assets/*.{css,js} wenn Sources neuer sind als das Generat."""
    script = os.path.join(PROJECT_DIR, "tools", "embed_assets.py")
    if not os.path.exists(script):
        return
    out_h = os.path.join(PROJECT_DIR, "web", "assets_generated.h")
    sources = []
    assets_dir = os.path.join(PROJECT_DIR, "web", "assets")
    if os.path.isdir(assets_dir):
        for fname in os.listdir(assets_dir):
            if fname.endswith((".css", ".js")):
                sources.append(os.path.join(assets_dir, fname))
    needs_regen = not os.path.exists(out_h) or any(
        os.path.getmtime(s) > os.path.getmtime(out_h) for s in sources
    )
    if needs_regen:
        print(f"embed_assets: regenerating {out_h}")
        subprocess.check_call([sys.executable, script])

embed_assets_if_stale()


def after_build(source, target, env):
    if not os.path.exists("builds"):
        os.mkdir("builds")

    lang = env.GetProjectOption('lang')
    target_name = lang.lower()

    with open(f"builds/latest_{target_name}.bin.md5", "w") as md5:
        print(_file_md5_hexdigest(target[0].path), file = md5)
    shutil.copy(target[0].path, f"builds/latest_{target_name}.bin")


env.AddPostAction("$BUILD_DIR/firmware.bin", after_build)
