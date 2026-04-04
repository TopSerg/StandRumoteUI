#ui_style.py 
from tkinter import ttk
from state import APP_FONT, MONO_FONT, PAD

def init_style(root, dark=False):
    style = ttk.Style()
    base = "clam" if "clam" in style.theme_names() else style.theme_use()
    style.theme_use(base)
    if dark:
        bg, fg, acc, sub, frame = "#1f2227","#e6e6e6","#3a7afe","#9aa0a6","#2a2f36"
    else:
        bg, fg, acc, sub, frame = "#f6f7fb","#202124","#215df0","#5f6368","#ffffff"
    style.configure(".", font=APP_FONT)
    style.configure("TFrame", background=bg)
    style.configure("TLabelframe", background=bg)
    style.configure("TLabelframe.Label", background=bg, foreground=fg, font=("Segoe UI Semibold", 10))
    style.configure("TLabel", background=bg, foreground=fg)
    style.configure("TButton", padding=(10,6))
    style.configure("Accent.TButton", foreground="white", background=acc)
    style.map("Accent.TButton", background=[("active", acc)])
    style.configure("Toolbar.TFrame", background=frame)
    style.configure("Card.TFrame", background=frame, relief="groove", borderwidth=1)
    style.configure("Treeview", font=MONO_FONT, background=frame, fieldbackground=frame, foreground=fg, rowheight=22)
    style.configure("Treeview.Heading", font=("Segoe UI Semibold", 9))
    return style

# фокус и стрелки для Scale
_active_scale = None
def make_focusable_scale(scale, var, step=1.0):
    def on_click(_):
        nonlocal step
        global _active_scale
        _active_scale = (scale, var, step)
        scale.focus_set()
    scale.bind("<Button-1>", on_click)

def bind_arrow_keys(root):
    def on_arrow_key(event):
        global _active_scale
        if not _active_scale:
            return
        scale, var, step = _active_scale
        value = var.get()
        if event.keysym == "Up":
            var.set(value + step)
        elif event.keysym == "Down":
            var.set(value - step)
    root.bind("<Up>", on_arrow_key)
    root.bind("<Down>", on_arrow_key)
