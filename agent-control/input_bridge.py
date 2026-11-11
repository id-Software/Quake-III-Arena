"""Local desktop input bridge for a foreground Quake III client.

Uses bounded, allowlisted input pulses. Wayland's security model does not
provide a compositor-independent foreground-window query, so Wayland input
requires an explicit operator opt-in.
"""
import os
import platform
import shutil
import subprocess
import time

MOVE_KEYS = {"forward": "w", "back": "s"}
STRAFE_KEYS = {"left": "a", "right": "d"}
KEY_VK = {"w": 0x57, "s": 0x53, "a": 0x41, "d": 0x44,
          "space": 0x20, "ctrl": 0x11}
LINUX_EVDEV = {"w": 17, "s": 31, "a": 30, "d": 32, "space": 57, "ctrl": 29}
MAC_KEYCODE = {"w": 13, "s": 1, "a": 0, "d": 2, "space": 49, "ctrl": 59}
WINDOW_TOKENS = ("quake iii", "quake 3", "quake3")


def _is_wayland():
    return platform.system() == "Linux" and bool(os.environ.get("WAYLAND_DISPLAY"))


def backend_name():
    system = platform.system()
    if system == "Windows":
        return "windows-user32"
    if system == "Darwin":
        return "macos-coregraphics"
    if system == "Linux" and _is_wayland() and shutil.which("ydotool"):
        return "linux-wayland-ydotool"
    if system == "Linux" and shutil.which("xdotool") and os.environ.get("DISPLAY"):
        return "linux-x11-xdotool"
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
    if system == "Linux" and _is_wayland():
        # Wayland intentionally prevents generic clients from inspecting
        # another client's surface/focus. Do not pretend this can be verified.
        return None
    if system == "Linux" and shutil.which("xdotool"):
        result = subprocess.run(["xdotool", "getwindowfocus", "getwindowname"],
                                capture_output=True, text=True, timeout=2, check=True)
        return result.stdout.strip()
    if system == "Darwin":
        script = 'tell application "System Events" to get name of first application process whose frontmost is true'
        result = subprocess.run(["osascript", "-e", script], capture_output=True,
                                text=True, timeout=3, check=True)
        return result.stdout.strip()
    return ""


def require_game_foreground():
    title = foreground_window()
    if title is None and _is_wayland():
        if os.environ.get("Q3_AGENT_ALLOW_UNVERIFIED_WAYLAND_FOCUS") != "1":
            raise RuntimeError(
                "Wayland cannot expose a universal focused-window title. Focus Quake III, "
                "then explicitly set Q3_AGENT_ALLOW_UNVERIFIED_WAYLAND_FOCUS=1 to enable "
                "global ydotool input."
            )
        return "Wayland focus not verifiable (operator opt-in)"
    if not title or not any(token in title.casefold() for token in WINDOW_TOKENS):
        raise RuntimeError("Focus the Quake III Arena game window before sending agent input.")
    return title


def _windows_pulse(keys, attack, look_x, look_y, duration):
    import ctypes
    user32 = ctypes.windll.user32
    pressed = []
    attack_down = False
    try:
        for key in keys:
            vk = KEY_VK[key]
            user32.keybd_event(vk, 0, 0, 0)
            pressed.append(vk)
        if attack:
            user32.mouse_event(0x0002, 0, 0, 0, 0)
            attack_down = True
        if look_x or look_y:
            user32.mouse_event(0x0001, int(look_x), int(look_y), 0, 0)
        time.sleep(duration / 1000.0)
    finally:
        if attack_down:
            user32.mouse_event(0x0004, 0, 0, 0, 0)
        for vk in reversed(pressed):
            user32.keybd_event(vk, 0, 2, 0)


def _xdotool_pulse(keys, attack, look_x, look_y, duration):
    binary = shutil.which("xdotool")
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


def _ydotool_pulse(keys, attack, look_x, look_y, duration):
    binary = shutil.which("ydotool")
    if not binary:
        raise RuntimeError("Install ydotool and run its ydotoold daemon for Wayland input.")
    pressed = []
    attack_down = False
    try:
        for key in keys:
            code = LINUX_EVDEV[key]
            subprocess.run([binary, "key", str(code) + ":1"], check=True, timeout=2)
            pressed.append(code)
        if attack:
            # BTN_LEFT is Linux input event code 0x110 (272).
            subprocess.run([binary, "click", "0x40"], check=True, timeout=2)
            attack_down = True
        if look_x or look_y:
            subprocess.run([binary, "mousemove", "-x", str(look_x), "-y", str(look_y)],
                           check=True, timeout=2)
        time.sleep(duration / 1000.0)
    finally:
        if attack_down:
            subprocess.run([binary, "click", "0x80"], check=False, timeout=2)
        for code in reversed(pressed):
            subprocess.run([binary, "key", str(code) + ":0"], check=False, timeout=2)


def _macos_pulse(keys, attack, look_x, look_y, duration):
    import ctypes
    from ctypes import c_void_p, c_uint32, c_bool, c_double, c_int
    cg = ctypes.CDLL("/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics")
    cf = ctypes.CDLL("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation")
    class CGPoint(ctypes.Structure):
        _fields_ = [("x", c_double), ("y", c_double)]
    cg.CGEventCreateKeyboardEvent.restype = c_void_p
    cg.CGEventCreateKeyboardEvent.argtypes = [c_void_p, ctypes.c_ushort, c_bool]
    cg.CGEventCreateMouseEvent.restype = c_void_p
    cg.CGEventCreateMouseEvent.argtypes = [c_void_p, c_uint32, CGPoint, c_uint32]
    cg.CGEventPost.argtypes = [c_uint32, c_void_p]
    cg.CGEventGetLocation.restype = CGPoint
    cg.CGEventGetLocation.argtypes = [c_void_p]
    cg.CGEventCreate.restype = c_void_p
    cf.CFRelease.argtypes = [c_void_p]

    def post(event):
        if not event:
            raise RuntimeError("macOS could not create an input event; grant Accessibility permission.")
        try:
            cg.CGEventPost(0, event)  # kCGHIDEventTap
        finally:
            cf.CFRelease(event)

    pressed = []
    attack_down = False
    try:
        for key in keys:
            code = MAC_KEYCODE[key]
            post(cg.CGEventCreateKeyboardEvent(None, code, True))
            pressed.append(code)
        current = cg.CGEventCreate(None) if (attack or look_x or look_y) else None
        point = CGPoint(0, 0)
        if current:
            try:
                point = cg.CGEventGetLocation(current)
            finally:
                cf.CFRelease(current)
        elif attack or look_x or look_y:
            raise RuntimeError("macOS could not read the pointer location.")
        if attack:
            post(cg.CGEventCreateMouseEvent(None, 1, point, 0))
            attack_down = True
        if look_x or look_y:
            point.x += look_x
            point.y += look_y
            post(cg.CGEventCreateMouseEvent(None, 5, point, 0))
        time.sleep(duration / 1000.0)
    finally:
        if attack_down:
            current = cg.CGEventCreate(None)
            point = CGPoint(0, 0)
            if current:
                try:
                    point = cg.CGEventGetLocation(current)
                finally:
                    cg.CFRelease(current)
            post(cg.CGEventCreateMouseEvent(None, 2, point, 0))
        for code in reversed(pressed):
            post(cg.CGEventCreateKeyboardEvent(None, code, False))


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
    look_x, look_y = action.get("look_x", 0), action.get("look_y", 0)
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
    elif system == "Linux" and _is_wayland():
        _ydotool_pulse(keys, action.get("attack", False), look_x, look_y, duration)
    elif system == "Linux":
        _xdotool_pulse(keys, action.get("attack", False), look_x, look_y, duration)
    elif system == "Darwin":
        _macos_pulse(keys, action.get("attack", False), look_x, look_y, duration)
    else:
        raise RuntimeError("The input bridge supports Windows, Linux X11/Wayland, and macOS.")
    return {"executed": True, "window": title, "backend": backend_name(),
            "duration_ms": duration, "action": action}
