"""Local desktop input bridge for a foreground Quake III client.

Only emits short, allowlisted keyboard/mouse input pulses. It never executes
console commands, shell commands, or arbitrary process memory operations.
"""
import platform
import shutil
import subprocess
import time

MOVE_KEYS = {"forward": "w", "back": "s"}
STRAFE_KEYS = {"left": "a", "right": "d"}
KEY_VK = {"w": 0x57, "s": 0x53, "a": 0x41, "d": 0x44,
          "space": 0x20, "ctrl": 0x11}
WINDOW_TOKENS = ("quake iii", "quake 3", "quake3")


def backend_name():
    system = platform.system()
    if system == "Windows":
        return "windows-sendinput"
    if system == "Linux" and shutil.which("xdotool") and (os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY")):
        return "linux-xdotool"
    return None


def foreground_window():
    system = platform.system()
    if system == "Windows":
        import ctypes
        user32 = ctypes.windll.user32
        hwnd = user32.GetForegroundWindow()
        length = user32.GetWindowTextLengthW(hwnd)
        title = ctypes.create_unicode_buffer(length + 1)
        user32.GetWindowTextW(hwnd, title, length + 1)
        return title.value
    if system == "Linux" and shutil.which("xdotool"):
        result = subprocess.run(["xdotool", "getwindowfocus", "getwindowname"],
                                capture_output=True, text=True, timeout=2, check=True)
        return result.stdout.strip()
    return ""


def require_game_foreground():
    title = foreground_window()
    if not any(token in title.casefold() for token in WINDOW_TOKENS):
        raise RuntimeError("Focus the Quake III Arena game window before sending agent input.")
    return title


def _windows_pulse(keys, attack, look_x, look_y, duration):
    import ctypes
    user32 = ctypes.windll.user32
    key_down = 0
    key_up = 2
    mouse_move = 0x0001
    left_down = 0x0002
    left_up = 0x0004
    pressed = []
    try:
        for key in keys:
            vk = KEY_VK[key]
            user32.keybd_event(vk, 0, key_down, 0)
            pressed.append(vk)
        if attack:
            user32.mouse_event(left_down, 0, 0, 0, 0)
        if look_x or look_y:
            user32.mouse_event(mouse_move, int(look_x), int(look_y), 0, 0)
        time.sleep(duration / 1000.0)
    finally:
        if attack:
            user32.mouse_event(left_up, 0, 0, 0, 0)
        for vk in reversed(pressed):
            user32.keybd_event(vk, 0, key_up, 0)


def _linux_pulse(keys, attack, look_x, look_y, duration):
    binary = shutil.which("xdotool")
    if not binary:
        raise RuntimeError("xdotool is required for Linux X11 input. Wayland-only sessions are not supported.")
    pressed = []
    attack_down = False
    try:
        for key in keys:
            subprocess.run([binary, "keydown", "--clearmodifiers", key],
                           check=True, timeout=2)
            pressed.append(key)
        if attack:
            subprocess.run([binary, "mousedown", "1"], check=True, timeout=2)
            attack_down = True
        if look_x or look_y:
            subprocess.run([binary, "mousemove_relative", "--", str(look_x), str(look_y)],
                           check=True, timeout=2)
        time.sleep(duration / 1000.0)
    finally:
        if attack_down:
            subprocess.run([binary, "mouseup", "1"], check=False, timeout=2)
        for key in reversed(pressed):
            subprocess.run([binary, "keyup", key], check=False, timeout=2)


def execute_action(action):
    if not isinstance(action, dict):
        raise ValueError("Control action must be a JSON object.")
    allowed = {"move", "strafe", "jump", "crouch", "attack", "look_x", "look_y", "duration_ms"}
    if set(action) - allowed:
        raise ValueError("Unknown control fields: " + ", ".join(sorted(set(action) - allowed)))
    move = action.get("move", "none")
    strafe = action.get("strafe", "none")
    if move not in ("none", "forward", "back") or strafe not in ("none", "left", "right"):
        raise ValueError("move/strafe must use the documented allowlisted directions.")
    for field in ("jump", "crouch", "attack"):
        if field in action and not isinstance(action[field], bool):
            raise ValueError(field + " must be true or false.")
    look_x = action.get("look_x", 0)
    look_y = action.get("look_y", 0)
    if type(look_x) is not int or type(look_y) is not int or abs(look_x) > 100 or abs(look_y) > 100:
        raise ValueError("look_x and look_y must be integers from -100 to 100.")
    duration = action.get("duration_ms", 120)
    if type(duration) is not int or duration < 40 or duration > 250:
        raise ValueError("duration_ms must be an integer from 40 to 250.")
    keys = []
    if move != "none":
        keys.append(MOVE_KEYS[move])
    if strafe != "none":
        keys.append(STRAFE_KEYS[strafe])
    if action.get("jump", False):
        keys.append("space")
    if action.get("crouch", False):
        keys.append("ctrl")
    title = require_game_foreground()
    system = platform.system()
    if system == "Windows":
        _windows_pulse(keys, action.get("attack", False), look_x, look_y, duration)
    elif system == "Linux":
        _linux_pulse(keys, action.get("attack", False), look_x, look_y, duration)
    else:
        raise RuntimeError("The input bridge currently supports Windows and Linux X11 only.")
    return {"executed": True, "window": title, "backend": backend_name(),
            "duration_ms": duration, "action": action}
