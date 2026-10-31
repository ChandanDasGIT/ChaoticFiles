#!/usr/bin/env python3
"""Tests the whole update system against a local server, with a test key and a throwaway HOME.

    python3 tests/update/run-update-tests.py <work folder> [test name ...]

Needs a Wayland session: the real window opens for a moment each time. It refuses to run while the user's own copy is
open. It stops only processes it started itself, and it checks at the end that the user's real files did not change.
It proves an app started by asking the driver built into the test build (tests/update/testdriver.cpp): only a running
window can answer, and it says which version it is.
"""
import base64, hashlib, http.server, json, os, shutil, signal, socketserver, stat, subprocess, sys, tarfile, threading, time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
WORK = Path(sys.argv[1]).resolve()
ONLY = sys.argv[2:]
PORT = 18765
OLD, NEW = "1.0", "1.1"
BASE = WORK / "packages"
REAL_HOME = Path.home()
SHOTS = WORK / "screenshots"

results = []

# ---------------------------------------------------------------- the local GitHub
class Release:
    tag = "v" + NEW
    folder = None            # the release's files
    log = []

class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, *args): pass
    def do_GET(self):
        Release.log.append(self.path)
        if self.path == "/api/releases/latest":
            body = json.dumps({"tag_name": Release.tag}).encode()
            self.send_response(200); self.send_header("Content-Length", str(len(body))); self.end_headers(); self.wfile.write(body)
            return
        name = self.path.rsplit("/", 1)[-1]
        file = Release.folder / name if Release.folder else None
        if not self.path.startswith("/site/releases/download/" + Release.tag + "/") or not file or not file.exists():
            self.send_error(404); return
        data = file.read_bytes()
        self.send_response(200); self.send_header("Content-Length", str(len(data))); self.end_headers(); self.wfile.write(data)

class Server(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True
    allow_reuse_address = True

server = None
def serve():
    global server
    server = Server(("127.0.0.1", PORT), Handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
def stop_serving():
    global server
    server.shutdown(); server.server_close(); server = None

# ---------------------------------------------------------------- safety
def own_copy_running():
    for pid in filter(str.isdigit, os.listdir("/proc")):
        try:
            if Path(f"/proc/{pid}/comm").read_text().strip() == "appChaoticFiles":
                if b"CF_TEST_COMMANDS" not in Path(f"/proc/{pid}/environ").read_bytes(): return pid
        except OSError: pass
    return None

def snapshot():
    state = {}
    for folder in [".config/ChaoticFiles", ".local/share/ChaoticFiles", ".cache/ChaoticFiles", ".local/lib/ChaoticFiles",
                   ".local/share/applications/ChaoticFiles.desktop", ".local/bin/ChaoticFiles"]:
        path = REAL_HOME / folder
        for item in [path] + (sorted(path.rglob("*")) if path.is_dir() else []):
            try:
                info = item.lstat()
                if not stat.S_ISDIR(info.st_mode): state[str(item)] = (info.st_size, info.st_mtime_ns)
            except OSError: pass
    return state

# ---------------------------------------------------------------- one isolated user
class User:
    made = []
    def __init__(self, name):
        User.made.append(self)
        self.home = WORK / "homes" / name
        shutil.rmtree(self.home, ignore_errors=True)
        (self.home / "bin").mkdir(parents=True)
        self.commands = self.home / "commands"; self.answers = self.home / "answers"
        self.commands.touch(); self.answers.touch()
        self.read = 0
        self.pids = set()
        opener = self.home / "bin" / "xdg-open"
        opener.write_text(f'#!/bin/sh\necho "$1" >> "{self.home}/opened"\n'); opener.chmod(0o755)

    def env(self):
        env = dict(os.environ)
        for name in ("APPIMAGE", "APPDIR", "ARGV0", "OWD", "LD_LIBRARY_PATH", "QT_PLUGIN_PATH"): env.pop(name, None)
        assert str(self.home) != str(REAL_HOME)
        env.update(HOME=str(self.home), XDG_CONFIG_HOME=str(self.home / ".config"), XDG_DATA_HOME=str(self.home / ".local/share"),
                   XDG_CACHE_HOME=str(self.home / ".cache"), XDG_STATE_HOME=str(self.home / ".local/state"),
                   PATH=str(self.home / "bin") + ":" + os.environ["PATH"], QT_FORCE_STDERR_LOGGING="1",
                   CF_TEST_COMMANDS=str(self.commands), CF_TEST_ANSWERS=str(self.answers))
        return env

    def start(self, program, *args):
        return subprocess.Popen([str(program), *map(str, args)], env=self.env(), cwd=self.home,
                                stdout=open(self.home / "log", "a"), stderr=subprocess.STDOUT)

    def fresh_answers(self):
        lines = self.answers.read_text().splitlines()
        new = [json.loads(line) for line in lines[self.read:]]
        self.read = len(lines)
        for answer in new: self.pids.add(answer["pid"])
        return new

    def wait_for(self, test, seconds=20, what="answer"):
        end = time.time() + seconds
        while time.time() < end:
            for answer in self.fresh_answers():
                if test(answer): return answer
            time.sleep(0.05)
        raise AssertionError(f"timed out waiting for {what}; log end: {self.log_tail()}")

    def log_tail(self):
        try: return (self.home / "log").read_text()[-400:]
        except OSError: return ""

    def ready(self, version=None, seconds=20):
        answer = self.wait_for(lambda a: a["command"] == "ready" and a["ok"] and (version is None or a["version"] == version), seconds, "a running window")
        return answer

    def send(self, line, seconds=15):
        with open(self.commands, "a") as file: file.write(line + "\n")
        return self.wait_for(lambda a: a["command"] == line, seconds, line)

    def state(self): return self.send("state")

    def until(self, test, seconds=30, what="state"):
        end = time.time() + seconds
        while time.time() < end:
            answer = self.state()
            if test(answer): return answer
            time.sleep(0.15)
        raise AssertionError(f"timed out: {what}; last {answer}")

    def shot(self, name, size=None):
        SHOTS.mkdir(exist_ok=True)
        if size: self.send("resize " + size); time.sleep(0.5)
        else: time.sleep(0.3)
        self.send(f"shot {SHOTS / name}.png")

    def alive(self, pid):
        try:
            return Path(f"/proc/{pid}/stat").read_text().rsplit(")", 1)[1].split()[0] != "Z"
        except OSError: return False

    def finish(self):
        for pid in self.pids:
            try:
                if b"CF_TEST_COMMANDS=" + str(self.commands).encode() in Path(f"/proc/{pid}/environ").read_bytes():
                    os.kill(pid, signal.SIGKILL)
            except OSError: pass

def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def install_tarball(user, version):
    folder = user.home / "unpacked"
    shutil.rmtree(folder, ignore_errors=True); folder.mkdir()
    with tarfile.open(BASE / version / f"ChaoticFiles-{version}-linux-x86_64.tar.xz") as tar: tar.extractall(folder, filter="tar")
    subprocess.run([next(folder.glob("*/install-app.sh"))], env=user.env(), check=True, stdout=subprocess.DEVNULL)
    return user.home / ".local/lib/ChaoticFiles"

def place_appimage(user, version):
    target = user.home / "apps with space" / f"ChaoticFiles-{version}-x86_64.AppImage"
    target.parent.mkdir()
    shutil.copy(BASE / version / target.name, target)
    target.chmod(0o755)
    return target

def use(release): Release.tag = "v" + NEW; Release.folder = BASE / release; Release.log = []

# ---------------------------------------------------------------- the tests
def normal_update(kind):
    user = User("normal-" + kind); use(NEW)
    a = user.home / "first folder"; b = user.home / "second"; c = user.home / "third"
    for folder in (a, b, c): folder.mkdir()
    (a / "download-me.txt").write_text("x")
    if kind == "tarball":
        installed = install_tarball(user, OLD); program = installed / "AppRun"; stage = Path(str(installed) + ".update")
    else:
        installed = place_appimage(user, OLD); program = installed; stage = Path(str(installed) + ".update")
    before = sha(installed) if kind == "appimage" else sha(installed / "usr/bin/appChaoticFiles")
    user.start(program, a, b, c, "--tab=1")
    first = user.ready(OLD)
    assert first["tabs"] == [str(a), str(b), str(c)] and first["currentTab"] == 1, first
    user.until(lambda s: s["available"], what="the dot")
    assert user.send("visible updateDot")["ok"], "no green dot"
    user.shot(f"{kind}-1-dot-normal", "1100x720"); user.shot(f"{kind}-1-dot-small", "640x460")
    user.send("resize 1100x720")
    user.send("click updateDot")
    assert user.send("visible updateNowButton")["ok"], "no Update now button"
    user.shot(f"{kind}-2-about-normal")
    user.send("resize 640x460"); user.shot(f"{kind}-2-about-small"); user.send("resize 1100x720")
    assert not any("download" in path for path in Release.log), "something downloaded before Update now"
    user.send("click updateNowButton")
    user.until(lambda s: s["ready"], what="download and checks")
    assert not list(stage.glob("*.tar.xz")), "the tarball stayed after unpacking"
    user.send("visible Restart now")
    user.shot(f"{kind}-3-prompt-normal"); user.send("resize 640x460"); user.shot(f"{kind}-3-prompt-small"); user.send("resize 1100x720")
    old_pid = first["pid"]
    with open(user.commands, "a") as file: file.write("click Restart now\n")
    second = user.ready(NEW, 40)
    assert second["pid"] != old_pid, "the same process answered"
    assert second["tabs"] == [str(a), str(b), str(c)] and second["currentTab"] == 1, second
    end = time.time() + 15
    while user.alive(old_pid) and time.time() < end: time.sleep(0.05)
    assert not user.alive(old_pid), "the old version is still running"
    user.until(lambda s: not stage.exists(), 10, "the update folder to go")
    after = sha(installed) if kind == "appimage" else sha(installed / "usr/bin/appChaoticFiles")
    assert after != before, "the program did not change"
    if kind == "appimage": assert after == sha(BASE / NEW / f"ChaoticFiles-{NEW}-x86_64.AppImage")
    assert not user.state()["available"], "the dot is still there"
    left = [p.name for p in installed.parent.iterdir() if "update" in p.name or p.name.endswith(".tar.xz")]
    assert not left, left
    # the app opened by the update must not pass the update's variables on
    environ = Path(f"/proc/{second['pid']}/environ").read_bytes()
    assert b"CHAOTICFILES_UPDATE_STARTED" in environ, "this run was not started by the script"
    user.send("quit"); user.finish()

def refused(name, prepare, expected):
    user = User("refuse-" + name); Release.log = []
    Release.tag = "v" + NEW; Release.folder = BASE / prepare
    installed = install_tarball(user, OLD); before = sha(installed / "usr/bin/appChaoticFiles")
    user.start(installed / "AppRun")
    user.ready(OLD)
    user.until(lambda s: s["available"])
    user.send("click updateDot"); user.send("click updateNowButton")
    state = user.until(lambda s: not s["busy"] and s["status"] not in ("", f"Version {NEW} is available.", "Checking the update…") and not s["status"].startswith("Downloading") and not s["status"].startswith("Checking the download"), what="the refusal")
    assert expected in state["status"], state["status"]
    assert not state["ready"], "it became ready"
    assert not user.send("visible Restart now")["ok"], "it offered a restart"
    assert not Path(str(installed) + ".update").exists(), "the update folder stayed"
    assert sha(installed / "usr/bin/appChaoticFiles") == before, "the program changed"
    if name in ("wrong-key", "missing-signature", "edited-manifest", "other-version"):
        assert not any(path.endswith(".tar.xz") for path in Release.log), "it downloaded the file anyway"
    user.send("quit"); user.finish()

def rollback(kind, variant):
    user = User(f"rollback-{kind}-{variant}"); use(f"{NEW}-{variant}")
    a = user.home / "a tab"; a.mkdir()
    if kind == "tarball":
        installed = install_tarball(user, OLD); program = installed / "AppRun"
        before = sha(installed / "usr/bin/appChaoticFiles")
    else:
        installed = place_appimage(user, OLD); program = installed; before = sha(installed)
    stage = Path(str(installed) + ".update")
    user.start(program, a)
    first = user.ready(OLD)
    user.until(lambda s: s["available"]); user.send("click updateDot"); user.send("click updateNowButton")
    user.until(lambda s: s["ready"], what="download")
    with open(user.commands, "a") as file: file.write("click Restart now\n")
    back = user.ready(OLD, 80)                   # the old version, really running again
    assert back["pid"] != first["pid"], "the same process answered"
    assert back["tabs"] == [str(a)], back
    assert "did not start" in back["status"], back["status"]
    assert user.send("visible OK")["ok"], "the rollback notice is not on screen"
    user.shot(f"rollback-{kind}-{variant}-notice", "1100x720")
    now = sha(installed) if kind == "appimage" else sha(installed / "usr/bin/appChaoticFiles")
    assert now == before, "the old program is not the one in place"
    assert not stage.exists() and not Path(str(stage) + ".failed").exists(), "leftovers"
    user.send("quit"); user.finish()

def cancel_prompt():
    user = User("cancel"); use(NEW)
    installed = install_tarball(user, OLD); before = sha(installed / "usr/bin/appChaoticFiles")
    stage = Path(str(installed) + ".update")
    user.start(installed / "AppRun")
    first = user.ready(OLD)
    user.until(lambda s: s["available"]); user.send("click updateDot"); user.send("click updateNowButton")
    user.until(lambda s: s["ready"], what="download")
    assert user.send("click Cancel")["ok"]
    state = user.state()
    assert state["ready"] and user.alive(first["pid"]), "Cancel closed something"
    user.shot("cancel-after-normal", "1100x720")
    user.send("quit")                            # a later normal quit
    end = time.time() + 8
    while user.alive(first["pid"]) and time.time() < end: time.sleep(0.05)
    assert not user.alive(first["pid"]), "it did not quit"
    time.sleep(3)
    assert not [a for a in user.fresh_answers() if a["command"] == "ready"], "the app restarted after a normal quit"
    assert sha(installed / "usr/bin/appChaoticFiles") == before, "a normal quit installed the update"
    assert not (stage / "install.sh").exists(), "the install script was written"
    user.start(installed / "AppRun")             # and the next start cleans up
    second = user.ready(OLD)
    assert not stage.exists(), "the unfinished update stayed"
    assert second["available"]
    user.send("quit"); user.finish()

def wait_for_operations():
    user = User("operations"); use(NEW)
    installed = install_tarball(user, OLD)
    source = user.home / "big.bin"; target = user.home / "to"; target.mkdir()
    with open(source, "wb") as file: file.truncate(1024 * 1024 * 1024)
    user.start(installed / "AppRun")
    first = user.ready(OLD)
    user.until(lambda s: s["available"]); user.send("click updateDot")
    user.send(f"copy {source}|{target}"); user.until(lambda s: s["operations"] == 1, what="the copy"); user.send("pause")
    user.send("click updateNowButton")
    user.until(lambda s: s["ready"], what="download")
    assert user.send("visible Restart when finished")["ok"], "no question about the running copy"
    user.shot("operations-prompt-normal", "1100x720")
    # Cancel first: nothing may restart when the copy ends
    user.send("click Cancel"); assert user.state()["ready"]
    user.send("click updateNowButton")
    assert user.send("visible Restart when finished")["ok"]
    user.send("click Restart when finished")
    state = user.state(); assert state["waiting"] and user.alive(first["pid"]), state
    assert user.send("visible cancelRestartButton")["ok"], "no way to change your mind"
    user.send("click cancelRestartButton"); assert not user.state()["waiting"]
    user.send("click updateNowButton"); user.send("click Restart when finished")
    assert user.state()["waiting"]
    user.send("pause")                           # resumes: the copy ends, and only then the app restarts
    second = user.ready(NEW, 120)
    assert (target / "big.bin").stat().st_size == source.stat().st_size, "the copy was cut short"
    user.send("quit"); user.finish()

def other_window():
    user = User("other-window"); use(NEW)
    installed = install_tarball(user, OLD)
    user.start(installed / "AppRun")
    user.ready(OLD)
    user.until(lambda s: s["available"]); user.send("click updateDot"); user.send("click updateNowButton")
    user.until(lambda s: s["ready"], what="download"); user.send("click Cancel")
    process = subprocess.Popen([str(installed / "AppRun")], env=dict(user.env(), CF_TEST_COMMANDS=str(user.home / "c2"), CF_TEST_ANSWERS=str(user.home / "a2")), cwd=user.home)
    end = time.time() + 15
    while not (user.home / "a2").exists() and time.time() < end: time.sleep(0.05)
    time.sleep(0.5)
    user.send("click updateNowButton")
    state = user.state()
    assert "Another ChaoticFiles window" in state["status"], state["status"]
    assert not user.send("visible Restart now")["ok"], "it offered a restart with another window open"
    process.kill(); process.wait(); time.sleep(0.3)
    # a killed window's lock goes with it
    user.send("click updateNowButton")
    assert user.send("visible Restart now")["ok"], "still blocked after the other window closed"
    user.send("click Cancel"); user.send("quit"); user.finish()

def stage_in_use():
    user = User("stage-in-use"); use(NEW)
    installed = install_tarball(user, OLD); stage = Path(str(installed) + ".update")
    stage.mkdir(); (stage / "previous").write_text("the backup"); mark = stage / "installing"; mark.touch()
    user.start(installed / "AppRun"); user.ready(OLD)
    assert (stage / "previous").exists(), "a second window deleted the backup of a running update"
    user.send("quit"); time.sleep(0.5)
    old = time.time() - 3600; os.utime(mark, (old, old))
    user.start(installed / "AppRun"); user.ready(OLD)
    assert not stage.exists(), "a left-over update stayed"
    user.send("quit"); user.finish()

def switch_off():
    user = User("switch-off"); use(NEW)
    installed = install_tarball(user, OLD)
    settings = user.home / ".config/ChaoticFiles/ChaoticFiles.conf"
    settings.parent.mkdir(parents=True); settings.write_text("[updates]\ncheck=false\n")
    user.start(installed / "AppRun")
    first = user.ready(OLD)
    time.sleep(6)                                # past the first check's delay
    assert Release.log == [], Release.log
    state = user.state(); assert not state["autoCheck"] and not state["available"], state
    assert not user.send("visible updateDot")["ok"]
    user.send("quit"); user.finish()

    user = User("switch-on"); use(NEW)
    installed = install_tarball(user, OLD)
    user.start(installed / "AppRun"); user.ready(OLD)
    time.sleep(1)
    assert Release.log == [], "it asked before the delay"
    user.until(lambda s: s["available"], what="the daily check")
    assert Release.log.count("/api/releases/latest") == 1
    assert len(Release.log) == 1, Release.log    # the check only asks; it fetches nothing
    user.send("quit"); time.sleep(0.5)
    user.start(installed / "AppRun"); user.ready(OLD); time.sleep(5)
    assert Release.log.count("/api/releases/latest") == 1, "it checked twice in a day"
    assert user.state()["available"], "the dot did not come back"
    user.send("quit"); user.finish()

def offline():
    stop_serving()
    try:
        user = User("offline")
        installed = install_tarball(user, OLD)
        user.start(installed / "AppRun")
        user.ready(OLD)
        time.sleep(6)
        state = user.state()
        assert state["status"] == "" and not state["available"] and not state["busy"], state
        assert not user.send("visible updateDot")["ok"]
        settings = user.home / ".config/ChaoticFiles/ChaoticFiles.conf"
        assert not settings.exists() or "lastCheck" not in settings.read_text(), "a failed check counted as the day's check"
        user.shot("offline-normal", "1100x720")
        user.send("quit"); user.finish()
    finally:
        serve()

def check_now():
    user = User("check-now"); use(NEW)
    Release.tag = "v" + OLD                      # the newest release is the one we run
    installed = install_tarball(user, OLD)
    settings = user.home / ".config/ChaoticFiles/ChaoticFiles.conf"
    settings.parent.mkdir(parents=True); settings.write_text("[updates]\ncheck=false\n")
    user.start(installed / "AppRun"); user.ready(OLD)
    assert user.send("about")["ok"] and user.send("click checkNowButton")["ok"]
    state = user.until(lambda s: not s["busy"] and s["status"] != "" and "Checking" not in s["status"], what="the answer")
    assert state["status"] == "You have the latest version.", state["status"]
    assert Release.log == ["/api/releases/latest"], Release.log
    user.shot("check-now-latest-normal", "1100x720"); user.send("resize 640x460"); user.shot("check-now-latest-small"); user.send("resize 1100x720")
    stop_serving()
    try:
        assert user.send("click checkNowButton")["ok"]
        state = user.until(lambda s: not s["busy"] and "Could not" in s["status"], what="the offline answer")
        assert state["status"] == "Could not reach GitHub. Try again later."
        user.shot("check-now-offline-small")
    finally:
        serve()
    Release.tag = "v" + NEW
    assert user.send("click checkNowButton")["ok"]
    user.until(lambda s: s["available"], what="the new version")
    user.send("quit"); user.finish()

def root_install():
    user = User("root-install"); use(NEW)
    prefix = user.home / "usr-local"
    subprocess.run(["cmake", "--install", str(WORK / "build-old-plain"), "--prefix", str(prefix)], check=True, stdout=subprocess.DEVNULL)
    for path in [prefix, *prefix.rglob("*")]:
        if not path.is_symlink(): path.chmod(path.stat().st_mode & ~0o222)
    user.start(prefix / "bin/appChaoticFiles")
    first = user.ready(OLD)
    assert not first["canInstall"], "a read-only install says it can update itself"
    user.until(lambda s: s["available"]); assert user.send("visible updateDot")["ok"]
    user.send("click updateDot")
    assert user.send("visible releasePageButton")["ok"], "no release page button"
    assert not user.send("visible updateNowButton")["ok"], "it offers Update now"
    user.shot("root-about-normal", "1100x720"); user.send("resize 640x460"); user.shot("root-about-small"); user.send("resize 1100x720")
    assert user.send("click releasePageButton")["ok"], "the button could not be clicked"
    end = time.time() + 5
    while not (user.home / "opened").exists() and time.time() < end: time.sleep(0.05)
    opened = (user.home / "opened").read_text().strip()
    assert opened.endswith("/releases/latest"), opened
    assert not any("/site/" in path for path in Release.log), "it downloaded something"
    user.send("quit"); time.sleep(0.5); user.finish()
    for path in [prefix, *prefix.rglob("*")]:
        if not path.is_symlink(): path.chmod(path.stat().st_mode | 0o200)

def appimage_read_only():
    user = User("appimage-read-only"); use(NEW)
    image = place_appimage(user, OLD)
    image.parent.chmod(0o555)
    user.start(image)
    first = user.ready(OLD)
    assert not first["canInstall"], "an AppImage in a read-only folder says it can update itself"
    user.send("quit"); user.finish(); image.parent.chmod(0o755)

def foreign_appimage_variables():
    user = User("foreign-appimage"); use(NEW)
    installed = install_tarball(user, OLD)
    fake = user.home / "Other.AppImage"; fake.write_text("x"); fake.chmod(0o755)
    env = dict(user.env(), APPIMAGE=str(fake), APPDIR="/tmp/.mount_other")
    subprocess.Popen([str(installed / "AppRun")], env=env, cwd=user.home)
    first = user.ready(OLD)
    assert first["canInstall"], first
    user.until(lambda s: s["available"]); user.send("click updateDot"); user.send("click updateNowButton")
    user.until(lambda s: s["ready"], what="download")
    assert not Path(str(fake) + ".update").exists(), "it took another app's AppImage for its own"
    assert Path(str(installed) + ".update").exists()
    user.send("click Cancel"); user.send("quit"); user.finish()

def installers():
    for shell in ("bash", "fish"):
        user = User("installer-" + shell)
        data = user.home / ".local/share/ChaoticFiles"; data.mkdir(parents=True); (data / "recent").write_text("keep")
        folder = user.home / "un packed"; folder.mkdir()
        with tarfile.open(BASE / OLD / f"ChaoticFiles-{OLD}-linux-x86_64.tar.xz") as tar: tar.extractall(folder, filter="tar")
        package = next(folder.glob("*"))
        command = "./install-app.sh"
        for _ in range(2):                       # a second run is an update by hand
            done = subprocess.run([shell, "-c", command], cwd=package, env=user.env(), capture_output=True, text=True)
            assert done.returncode == 0, done.stderr
        assert "is installed" in done.stdout
        command_link = user.home / ".local/bin/ChaoticFiles"
        assert command_link.resolve() == user.home / ".local/lib/ChaoticFiles/AppRun"
        env = user.env(); env["PATH"] = str(user.home / ".local/bin") + ":" + env["PATH"]
        process = subprocess.Popen([shell, "-c", "ChaoticFiles"], cwd=user.home, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        started = user.ready(OLD)
        assert started["canInstall"], "an installed tarball cannot update itself"
        user.send("quit"); time.sleep(0.5); process.wait(timeout=10)
        done = subprocess.run([shell, "-c", "./uninstall-app.sh"], cwd=package, env=user.env(), capture_output=True, text=True)
        assert done.returncode == 0, done.stderr
        assert not (user.home / ".local/lib/ChaoticFiles").exists() and not command_link.is_symlink()
        assert (data / "recent").read_text() == "keep", "the uninstaller took the user's data"
        user.finish()

def small_window():
    user = User("small-window"); use(NEW)
    installed = install_tarball(user, OLD)
    user.start(installed / "AppRun"); user.ready(OLD)
    user.send("resize 640x460"); time.sleep(0.4)
    user.until(lambda s: s["available"])
    assert user.send("click updateDot")["ok"], "the dot could not be clicked"
    time.sleep(0.3)
    user.shot("small-about")
    assert user.send("click updateNowButton")["ok"], "Update now is out of reach in a small window"
    user.until(lambda s: s["ready"], what="download")
    user.shot("small-prompt")
    user.send("click Cancel"); time.sleep(0.2); user.shot("small-ready")
    assert user.send("click updateNowButton")["ok"], "Restart to update is out of reach"
    assert user.send("click Cancel")["ok"]
    user.send("quit"); user.finish()

def file_types():
    user = User("file-types")
    (user.home / ".config/gtk-3.0").mkdir(parents=True)
    real = REAL_HOME / ".config/gtk-3.0/settings.ini"
    if real.exists(): shutil.copy(real, user.home / ".config/gtk-3.0/settings.ini")
    files = user.home / "project"; files.mkdir()
    (files / "Main.qml").write_text("import QtQuick\nItem {}\n")
    (files / "main.cpp").write_text("int main() {}\n")
    (files / "main.h").write_text("#pragma once\n")
    (files / "lib.rs").write_text("fn main() {}\n")
    (files / "app.py").write_text("print(1)\n")
    (files / "notes.txt").write_text("words\n")
    (files / "CMakeLists.txt").write_text("project(x)\n")
    (files / "data.json").write_text("{}\n")
    (files / "script.ts").write_text("function f(): void {}\n")
    (files / "Dockerfile").write_text("FROM x\n")
    user.start(WORK / "build-old-plain/appChaoticFiles", files)
    user.ready(OLD); time.sleep(1.5)
    user.shot("types-details", "1000x640")
    assert user.send(f"openwith {files / 'Main.qml'}")["ok"]; time.sleep(0.5)
    user.shot("types-openwith-qml")
    user.send("click Cancel"); time.sleep(0.2)
    assert user.send(f"openwith {files / 'main.cpp'}")["ok"]; time.sleep(0.5)
    user.shot("types-openwith-cpp")
    user.send("click Cancel"); user.send("grid"); time.sleep(1.5)
    user.shot("types-grid")
    user.send("quit"); user.finish()

def script_is_portable():
    source = (ROOT / "backend/update/update.cpp").read_text()
    script = source.split('R"(', 1)[1].split(')";', 1)[0]
    path = WORK / "install-script.sh"; path.write_text(script)
    done = subprocess.run(["bash", "--posix", "-n", str(path)], capture_output=True, text=True)
    assert done.returncode == 0, done.stderr
    assert "kill -0" not in script
    for word in ("[[", "function ", "local ", "source ", "==", "$RANDOM"):
        assert word not in script, word

TESTS = {
    "normal-tarball": lambda: normal_update("tarball"),
    "normal-appimage": lambda: normal_update("appimage"),
    "refuse-wrong-key": lambda: refused("wrong-key", f"{NEW}-wrong-key", "signature is wrong"),
    "refuse-tampered-file": lambda: refused("tampered-file", f"{NEW}-tampered-file", "does not match its signature"),
    "refuse-missing-signature": lambda: refused("missing-signature", f"{NEW}-missing-signature", "no signed update"),
    "refuse-edited-manifest": lambda: refused("edited-manifest", f"{NEW}-edited-manifest", "signature is wrong"),
    "refuse-other-version": lambda: refused("other-version", f"{NEW}-other-version", "details are wrong"),
    "rollback-tarball-exits": lambda: rollback("tarball", "exits"),
    "rollback-tarball-crashes": lambda: rollback("tarball", "crashes"),
    "rollback-tarball-dies-early": lambda: rollback("tarball", "dies-early"),
    "rollback-tarball-hangs": lambda: rollback("tarball", "hangs"),
    "rollback-appimage-crashes": lambda: rollback("appimage", "crashes"),
    "cancel-prompt": cancel_prompt,
    "wait-for-operations": wait_for_operations,
    "other-window": other_window,
    "stage-in-use": stage_in_use,
    "switch-off": switch_off,
    "offline": offline,
    "check-now": check_now,
    "root-install": root_install,
    "appimage-read-only": appimage_read_only,
    "foreign-appimage-variables": foreign_appimage_variables,
    "installers": installers,
    "script-is-portable": script_is_portable,
    "small-window": small_window,
    "file-types": file_types,
}

def main():
    running = own_copy_running()
    if running:
        print(f"YOUR COPY IS RUNNING (pid {running}): not testing."); return 3
    before = snapshot()
    serve()
    failed = 0
    for name, test in TESTS.items():
        if ONLY and name not in ONLY: continue
        begun = time.time()
        try:
            test(); print(f"PASS  {name}  ({time.time() - begun:.0f}s)", flush=True)
        except Exception as error:
            failed += 1; print(f"FAIL  {name}: {type(error).__name__}: {error}", flush=True)
        for user in User.made: user.finish()
        User.made.clear()
        subprocess.run(["chmod", "-R", "u+w", str(WORK / "homes")], capture_output=True)   # keeps the logs, not the copies
        for home in (WORK / "homes").glob("*"):
            for name_ in ("unpacked", "un packed", ".local", "apps with space", "usr-local", "big.bin", "to"):
                target = home / name_
                if target.is_dir(): shutil.rmtree(target, ignore_errors=True)
                elif target.exists(): target.unlink()
    assert snapshot() == before, "THE USER'S REAL FILES CHANGED"
    print("Real home untouched." if snapshot() == before else "REAL HOME CHANGED")
    print(f"{failed} failed." if failed else "All passed.")
    return 1 if failed else 0

sys.exit(main())
