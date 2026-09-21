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

## Калибровка нулевого угла резольвера

Для автоматического подбора `thetaCorr` нужны согласованные версии этого репозитория и
`controllerat32`: прошивка передаёт `FluxPositionError` и фактический параметр
`ResolverThetaCorrection` кадром `0x082`, а стенд отправляет единственную разрешённую в
RX-only команду `0x301`. Команда защищена magic-значением и отключается в прошивке при
пропадании keepalive более чем на 250 мс.

1. В обычном режиме включить инвертор с заданиями `Id = 0`, `Iq = 0`.
2. Перейти на вкладку **Resolver RX** и нажать **Start RX-only**.
3. Вращать вал внешним мотором. `FluxPositionError` становится валидным только при
   `|Welectrical| > 300 rad/s` — слишком низкая скорость даст состояние `waiting_for_speed`.
4. Нажать **Start auto thetaCorr**. Алгоритм усредняет пять отсчётов, ограничивает каждый
   шаг и считает результат устойчивым после 20 отсчётов внутри допуска.
5. Записать найденный `thetaCorr`. Значение применяется только в RAM; кнопка **Stop auto**,
   выход из RX-only или потеря связи возвращают заводской `resolverShift`.
