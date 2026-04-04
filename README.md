## Запуск
сервер: 
C:\Users\n.tulupov\Desktop\StandRemoteUI\server>powershell -ExecutionPolicy Bypass -File .\build_ninja.ps1 -VcpkgToolchain "C:\vcpkg\scripts\buildsystems\vcpkg.cmake"
.\server\build\MarathonWS.exe
клиент:
 python client/gui_ws.py


Запуск формирования архива бинарников
powershell -NoProfile -ExecutionPolicy Bypass -File C:\work\StandRemoteUI\make_bundle.ps1

Запуск апдейт-скрипта который скачивает релиз с гита и запускает клиент и сервер
powershell -ExecutionPolicy Bypass -File C:\work\StandRemoteUI\update_standcontrol.ps1



+--------------------+             +-------------------------------+
| Главный поток      |             | Фоновый поток с asyncio       |
| (Tkinter mainloop) |             | (WSClient._run_loop)          |
+--------------------+             +-------------------------------+
|                    |             |  new_event_loop()             |
| gui.py             |             |  run_until_complete(          |
|                    |             |      _connect_forever() )     |
|  ┌──────────────┐  |             |                               |
|  | Кнопки GUI   |  |   send_cmd  |  ┌─────────────────────────┐  |
|  | (Start/Stop) |──┼────────────►|  | websockets.connect(url) |  |
|  └──────────────┘  |  (threadsafe)|  └─────────┬──────────────┘  |
|                    |             |            ws                |
|  ┌──────────────┐  |   after(0)  |  while running:              |
|  | Виджеты      |◄─┼─────────────┤    msg = await ws.recv()     |
|  | метки/поля   |  |  on_message |    on_message(msg)           |
|  └──────────────┘  |             |  on_error/on_status при сбое |
+--------------------+             +-------------------------------+


Последовательность при старте
GUI создаёт WSClient(url, on_message, on_status, on_error).
WSClient.start():
поднимает отдельный поток, в нём создаётся свой asyncio loop, запускается _connect_forever().
_connect_forever():
подключается к ws://…, держит соединение и ждёт сообщения, при ошибке логирует и делает реконнект с backoff.

Отправка команды из GUI
Пользователь нажимает кнопку в Tkinter.
handler вызывает client.send_cmd_threadsafe("Init").
Внутри создаётся корутина _send(), которая делает ws.send({"cmd": "Init"}).
Корутину пускают в чужой loop через asyncio.run_coroutine_threadsafe(...).
По завершении коллбэк пишет в on_status «Отправлено: Init» или в on_error.

Приём данных с сервера  
В фоне await ws.recv() получает строку JSON.
WSClient вызывает on_message(msg).
on_message не трогает виджеты напрямую, а делает root.after(0, lambda: …), чтобы обновить метки/поля из главного потока Tkinter.

Аварии и реконнект
Любая ошибка в приёме/отправке ловится, ws зануляется, статус «WS отключен».
Ждём 1/2/4/…/10 сек и снова пытаемся подключиться.
stop(): снимает флаг, отменяет задачи, останавливает loop.