# network.py
import asyncio
import json
import threading
import traceback

try:
    import websockets
except ImportError:
    websockets = None


class WSClient:
    def __init__(self, url, on_message, on_status, on_error):
        self.url = url
        self.on_message = on_message  # функция, вызываемая при получении сообщения
        self.on_status = on_status    # функция, вызываемая при смене состояния
        self.on_error = on_error      # функция, вызываемая при ошибке

        self.loop = None
        self.thread = None
        self.ws = None
        self._running = threading.Event()
        self._running.clear()

        # ВАЖНО: из правой версии (и нужно для is_connected / clear/set)
        self._connected_evt = threading.Event()
        self._connected_evt.clear()

        self._main_task = None  # asyncio.Task для _connect_forever()

    def start(self):
        if self.thread and self.thread.is_alive():
            return
        if websockets is None:
            self.on_error("Не найден модуль 'websockets'. Установите: pip install websockets")
            return
        self._running.set()
        self.thread = threading.Thread(target=self._run_loop, daemon=True)
        self.thread.start()

    def stop(self):
        self._running.clear()

        # аккуратно закрываем вебсокет перед остановкой цикла
        if self.loop and self.ws:
            try:
                fut = asyncio.run_coroutine_threadsafe(
                    self.ws.close(code=1000, reason="client stop"), self.loop
                )
                fut.result(timeout=1.0)
            except Exception:
                pass

        if self.loop and self.loop.is_running():
            def _stop():
                # 1) отменяем только главный task
                if self._main_task and not self._main_task.done():
                    self._main_task.cancel()

                # 2) аккуратно закрываем websocket (если есть)
                if self.ws is not None:
                    async def _close():
                        try:
                            await self.ws.close()
                        except Exception:
                            pass
                    asyncio.create_task(_close())

                # 3) останавливаем loop чуть позже
                self.loop.call_later(0.05, self.loop.stop)

            self.loop.call_soon_threadsafe(_stop)

        if self.thread:
            try:
                self.thread.join(timeout=3.0)
            except Exception:
                pass
            finally:
                self.thread = None

        self._connected_evt.clear()

    def _run_loop(self):
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)

        self._main_task = self.loop.create_task(self._connect_forever())

        try:
            self.loop.run_forever()
        except Exception as e:
            self.on_error(f"Fatal WS loop error: {e}\n{traceback.format_exc()}")
        finally:
            try:
                pending = asyncio.all_tasks(loop=self.loop)
                for t in pending:
                    t.cancel()
                if pending:
                    self.loop.run_until_complete(asyncio.gather(*pending, return_exceptions=True))
            except Exception:
                pass

            try:
                self.loop.close()
            except Exception:
                pass

            self.loop = None
            self._main_task = None
            self.ws = None
            self._connected_evt.clear()

    async def _connect_forever(self):
        backoff = 1.0
        while self._running.is_set():
            try:
                self.on_status(f"connecting to {self.url}...")

                # из правой версии: ping/timeout полезны против подвисаний
                async with websockets.connect(
                    self.url,
                    ping_interval=20,
                    ping_timeout=20,
                    close_timeout=1
                ) as ws:
                    self.ws = ws
                    self._connected_evt.set()
                    self.on_status("WS connected")
                    backoff = 1.0

                    while self._running.is_set():
                        try:
                            msg = await ws.recv()
                        except asyncio.CancelledError:
                            raise
                        except Exception:
                            break
                        self.on_message(msg)

            except asyncio.CancelledError:
                break
            except Exception as e:
                self.on_error(f"WS error: {e}")
                self.on_status("WS disabled")
                self.ws = None
                self._connected_evt.clear()
                await asyncio.sleep(backoff)
                backoff = min(backoff * 2, 10.0)

        self.on_status("WS stopped")
        self._connected_evt.clear()

    def is_connected(self) -> bool:
        return self._connected_evt.is_set()

    def send_cmd_threadsafe(self, cmd: str):
        """Без await. Можно вызывать из любого потока (в т.ч. из Tk)."""
        if not self.loop:
            self.on_error("WS: no event loop")
            return

        async def _send():
            if self.ws is None:
                raise RuntimeError("WS not connected")
            await self.ws.send(json.dumps({"cmd": cmd}))

        fut = asyncio.run_coroutine_threadsafe(_send(), self.loop)

        def _cb(f):
            try:
                f.result()
                self.on_status(f"sent: {cmd}")
            except Exception as e:
                self.on_error(f"sending error '{cmd}': {e}")

        fut.add_done_callback(_cb)
        return fut

    def send_json_threadsafe(self, payload: dict):
        if not self.loop:
            self.on_error("WS: нет event loop")
            return

        async def _send():
            if self.ws is None:
                raise RuntimeError("WS not connected")
            await self.ws.send(json.dumps(payload))

        fut = asyncio.run_coroutine_threadsafe(_send(), self.loop)

        def _cb(f):
            try:
                f.result()
                self.on_status(f"sent: {payload.get('cmd', '<no cmd>')}")
            except Exception as e:
                self.on_error(f"sending error: {e}")

        fut.add_done_callback(_cb)
        return fut
