# app.py
import tkinter as tk

from network import WSClient
from state import AppState
from ui_style import init_style
from view import build_ui
from controllers import Controllers
from telemetry import Telemetry

DEFAULT_WS_ADDR = "192.168.8.100:9000" 

def main():
    # --- TK root ---
    root = tk.Tk()
    root.title("StandRemoteGUI")
    root.geometry("1080x740+100+100")
    try:
        root.state("zoomed")
    except Exception:
        pass

    # --- Состояние приложения (все tk.Variable, буферы и пр.) ---
    state = AppState(root)

    # --- Стиль/тема (точно как было в gui_ws.py) ---
    init_style(root, dark=False)\
    
    
    # --- Контроллеры (логика кнопок/команд), пока без видов и без сети ---
    controllers = Controllers(root, state)

    # --- View (построение всего GUI), прокидываем обработчики от контроллеров ---
    views = build_ui(root, state, handlers=controllers.handlers())
    controllers.attach_views(views)

    # --- Телеметрия (разбор входящих сообщений, обновление state и UI) ---
    telemetry = Telemetry(root, state, views, ui_log=controllers.ui_log)

    # --- Сеть (WebSocket клиент остаётся из network.py как есть) ---
    def _make_client(ws_url: str) -> WSClient:
        return WSClient(
            ws_url,
            on_message=telemetry.on_message,
            on_status=telemetry.on_status,
            on_error=telemetry.on_error,
        )

    controllers.bind_network_factory(_make_client)

    # Инициализируем поле адреса (если view его ещё не создал — подстрахуемся)
    if not getattr(state, "ws_addr_var", None):
        state.ws_addr_var = tk.StringVar(value=DEFAULT_WS_ADDR, master=root)
    elif not (state.ws_addr_var.get() or "").strip():
        state.ws_addr_var.set(DEFAULT_WS_ADDR)

    # Автоконнект на дефолтный адрес (по желанию можно убрать)
    controllers.connect_ws(state.ws_addr_var.get())

    # --- Таймеры телеметрии/рендера графиков ---
    telemetry.start_timers()

    # --- Корректное закрытие ---
    def _on_close():
        try:
            controllers.disconnect_ws()
        finally:
            try:
                root.destroy()
            except Exception:
                pass
            
    root.protocol("WM_DELETE_WINDOW", _on_close)

    # --- Главный цикл ---
    root.mainloop()


if __name__ == "__main__":
    main()
