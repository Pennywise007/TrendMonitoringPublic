# TrendMonitoringPublic

Для чего нужна эта програма?
Есть рабочий обьект, на котором установлены датчики снимающие различные показатели(прогибометры/толщинометры и пр.). Данные с датчиков через GSM передаются на ПК и записываются отдельной программой в виде бинарных файлов по определенной схеме. Упрощённое представление работы этой схемы: у каждого датчика есть файл описатель с информацией о датчике(его имени, частоте дискретизации, ед измерения и т.д.).
Файлы раскладываются по определенной иерархии в нужных папках и хранятся там. В дальнейшем по этим файлам отдельными программами можно построить тренд сигнала с каждого датчика и посмотреть на изменения показаний датчиков в зависимости от времени.
<p align="center">
<a href="https://ibb.co/hZPZzgx"><img src="https://i.ibb.co/xfpfT2K/image.png" alt="SignalTrends" border="0"></a>
<p align="center">1. Пример трендов сигналов.</p>
</p>

Данная программа следит за показаниями датчиков в реальном времени, расшифровывая файлы с данными, предоставляет данные в читабельном виде и оповещает операторов в случае если значения превышают допустимый уровень. Это позволяет не следить за показаниями постоянно, а заниматься мониторингом удаленно. В систему интегрирована работа с телеграм ботами, на каждый объект запускается свой экземпляр программы со своим ботом, что позволяет управлять/следить за множеством систем одновременно с одного(или нескольких устройств). В случае возникновения проблемы происходит оповещение операторов о проблеме, позволяя им предпринять дальнейшие действия. Поддерживается ежедневная отчётность о состоянии системы с анализом данных. В настоящий момент эта система установлена на нескольких объектах по Москве.
<p align="center">
<a href="https://ibb.co/gtTR2hF"><img src="https://i.ibb.co/wpWzkjr/image.png" alt="Monitoring" border="0"></a>
<p align="center">2. Работающая программа программа.</p>
</p>


<b>Архитектура программы</b>

Програму старался делать модульно, есть два проекта - MonitoringCore, в котором находятся все "мозги" программы и TrendMonitor, в нём находится UI часть.

<p align="center">
<a href="https://ibb.co/wWRPHwR"><img src="https://i.ibb.co/wWRPHwR/image.png" alt="image" border="0"></a>
<p align="center">3. Архитектура программы.</p>
</p>

<b>MonitroingCore</b>

Ститическая библиотека, в которой происходит все волшебство. Наружу выставлены несколько сервисов:
1. ITrendMonitoring - основной объект библиотеки, через него происходит взаимодействие с пользователем. Тут хранятся текущие настройки системы, осуществляется настройка телеграм бота, запускаются задания запроса данных по каналам, формируются отчёты и происходит оповещение о возникших ошибках.
2. IMonitoringTasksService - сервис с заданиями мониторинга, тут находится очередь заданий для получения данных из файлов.
3. DirsService - вспомогательный общий сервис работы с директориями
4. Messages - класс для работы с сообщениями(событиями) системы. Через него происходит взаимодействие объектов которые не должны ничего знать друг о друге и синхронизация потоков.

Отдельно хотелось бы отметить (де)сериализацию настроек, мною был реализован аналог рефлексии на С++. В настоящий момент механизм поддерживает COM объекты, но это не обязательно, привязка к COM была сделана для удобства преобразования интерфейсов и с задумкой использования в других проектах нуждающихся в COM объектах. Рефлексия написана при помощи новых возможностей С++17, метапрограммирования шаблонов, SFINAE и макросов. Данный способ позволяет объявить переменную сериализуемой при помощи однострочного макроса. Так же я добавил свою реализацию InternalCOM объектов(не регистрируемых). Присутствует общий интерфейс для фабрики который позволит осуществлять сохранение в любом виде, в настоящем проекте представлена реализация (де)сериализации в XML. При необходимости расширяется для работы с БД, пересылкой на сервер и т.д.

Что касается телеграм бота то, то в программе осуществляется разделение на администраторов(операторов) системы и обычных пользователей(заказчиков). Данное разделение предназначено для минимизации "дергания" заказчика из-за проблем, потому что порой датчики могут давать ложное срабатывание (может где-то отходить провод, технический работы и т.д.). У пользователей осуществляется разделение доступных команд и некоторые команды поддерживают создание кнопок для быстрого реагирования или упрощения взаимодействия с ботом. Итоговые колбэки парсятся при помощи boost::spirit и regex. изначально каждый запрос парсился по своим структурам, но запросов стало много и все было унифицированно для упрощения поддержки. Основная работа с ботом, обмен данных и работа с протоколами телеграма осуществляется через мою отдельную DLL, с ней можно ознакомиться подробнее тут: https://github.com/Pennywise007/TelegramDLLPublic
<p align="center">
<a href="https://ibb.co/wSmV1ZJ"><img src="https://i.ibb.co/N9B5zg1/image.png" alt="BotExample" border="0"></a>
<p align="center">4. Работа бота и реакция на команды.</p>
</p>


<b>TrendMonitor</b>

Здесь находится вся UI часть приложения, в частности
1. Несколько кастомных таблиц(обёрток над виндовым CListCtrl) реализованных в виде виджетов, что позволяет разделить функционал. Реализованы редактирование ячеек с возможностью вставки различных контролов в таблицу.
2. Кастомный список выбора с поиском через regex, что позволяет быстро и удобно ориентироваться в списке датчиков(на некоторых объектах около 50 различных датчиков). Данный контрол вставляется в таблицу при редактировании.
3. Вспомогательный класс для работы с икононкой в трее и отображением виндовых оповещений(Shell_NotifyIcon) в случае новых событий.
4. Диалоги и вкладки


<b>Tests</b>

Проект для тестирования через googletest, осуществляет модульное тестирование механизмов программы.
 - осуществляет проверку анализа данных(сравнивая с заранее заранее записанными эталонами)
 - проверяет результаты сериализации/десериализации(сравнивает с эталонами)
 - эмулирует и проверяет работу бота с пользователем
 - проверяет работу InternalCOM объектов. Время жизни, приведение и пр.
 - проверяем работу пользователя со списком каналов
<p align="center">
<a href="https://ibb.co/M19G7Rk"><img src="https://i.ibb.co/M19G7Rk/image.png" alt="TestResults" border="0"></a>
<p align="center">5. Результаты тестирования.</p>
</p>


<b>Дополнительная информация</b>

Приложение написанно на С++17, используемые технологии:
- boost (asio/program_options/string/spirit/fusion)
- std (regex/algorithm/filesystem/thread/future/mutext/chrono/functional/memory/optional и многое другео) - старался по максимому делать std нежели на boost, в планах расширить функционал в С++20
- прочее (atlCOM, afx, всевозможные контейнеры)
- pugixml
- библиотека для ботов телеграма https://github.com/Pennywise007/TelegramDLLPublic


Для сборки необходимы 4 макроса
- $(BoostIncludeDir) - Директория включаемых файлов буста (D:\UtilsDir\boost\include\)
- $(BoostLibDir)     - Директория библиотек буста         (D:\UtilsDir\boost\lib\)
- $(MyDLLDir)        - Путь к общей папке откуда будет копироваться TelegramDLLPublic.dll   (D:\Common\DLL\)
- $(MyIncludeDir)    - Путь к общей папке откуда будет браться TelegramDLL\TelegramThread.h (D:\Common\include\)

Для запуска понадобятся:
libeay32 и ssleay32
