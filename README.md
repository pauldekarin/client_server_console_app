<h1>Привет!</h1>

Станция:<br>
  MacBook Air M1 2020<br>
  macOs Ventura 13.0<br>
<br>
Разработка:<br>
C++17<br>
Visual Code<br>
Docker Ubuntu(последняя версия)<br>

<h2>Build Project</h2>
make build <br>
<br>
Выход:<br>
Серверное приложение - server.out<br>
Клиентское приложение - client.out<br>

<h2>Usage</h2>
>> parse() - Парсинг сообщения от клиента и возврат сообщения, содержащего табличку с количеством различных букв<br>
>> count() - Возврат количества текущих подключений по запросу клиента<br>
>>  chat() - Начать общение с клиентом через сервер<br>
>>  back() - Выйти из активного чата<br>
>>    id() - Отобразить свой идентификатор определенный на сервере<br>
>>  help() - Список команд<br>
>> exit() - Закрыть приложение<br>

<h2>Недочеты</h2>
Пропускная способность между сокетами 4096 байт, если отправлять больше, то сервер иклиент примут данные, но некорректно отработают, а
так как уже дедлайны подходят, я не успеваю это исправить(((
