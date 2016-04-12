# Детектор DDoS атак #
Система **ddosdetector** - это гибкий инструмент для анализа сетевого трафика и автоматизации процесса защиты от DDoS атак. Система основана на фреимворке [Luigi Rizzo](https://github.com/luigirizzo/netmap) [netmap](https://habrahabr.ru/post/183832/) и спроектирована для работы с большим объемом трафика (10Гб/сек и больше) без потерь производительности.

Система написана на языке C++ (стандарт 11) с использованием OpenSource библиотек *STL* и *Boost (1.55)*. Написание и сборка производилось на *Ubuntu 12.04.5 LTS* и компиляторе *g++-4.8*. Для статического анализа кода, проверки стиля и поиска грубых ошибок использовался *cppcheck* версии 1.73.

## Принцип работы ##
Демон запускается на SPAN интерфейсе (на этот интерфейс зеркалируется весь трафик защищаемой сети) сервера и начинает "прослушивать" весь трафик. Управление демоном осуществляется через консоль (доступ по TCP порту или UNIX сокету) стандартными утилитами Linux (telnet/netcat/socat). В консоли управления пользователю предоставляется командная строка с параметрами выделения, обнаружения, подсчета трафика, а также параметрами реакции на тот или иной трафик.

Пример правила для поиска трафика:
```
#!c++
ddoscontrold> show rules
TCP rules (num, rule, counter):
  -d 92.53.96.141/32 --pps-th 100p --hlen <20 --pps-th-period 60 --action log:/tmp/test.log --next    : 814.00p/s (735.03Kb/s), 157106 packets, 22975832 bytes
```
Все правила добавляемые в систему делятся на несколько глобальных групп соответствующих L4 протоколам (TCP, UDP, ICMP и т.д.). Каждое добавляемое правило добавляется в одну группу и в соответсвии с протоколом группы может иметь различные параметры обработки трафика (подробную информацию по доступным командам можно получить в консоли управления, набрав "help"). У каждого правила, в любой группе, имеется ряд обязательных праметров, без которых добавить правило не удастся:

* параметр src/dst ip
* параметр порога триггера (указывающий критическое значения по достижению которого вызывается действие триггера)

## Установка ##
Так как система работает на основе драйвера netmap, требуется установка этого драйвера.
### Установка netmap на Ubuntu ###
Для корректной работы драйвера netmap необходимо собрать модуль netmap и собрать драйвера сетевой карты с поддержкой netmap. Для этого требуется скачать исходники ядра установленного в системе (в инструкции ниже ядро 3.10.90) и собрать netmap с указанием этих исходников (сборка netmap пропатчет драйвера сетевой карты из исходников и собирет их).

Скачиваем исходники ядра и распаковываем:
```
#!bash

cd /usr/src
wget -S https://cdn.kernel.org/pub/linux/kernel/v3.x/linux-3.10.90.tar.xz
tar xpvf ./linux-3.10.90.tar.xz -C /usr/src/
```
либо из репозитория:

```
#!bash

cd /usr/src
apt-get source linux-image-3.10.90
```
скачиваем netmap:

```
#!bash

git clone https://github.com/luigirizzo/netmap
```
настраиваем сборку модуля указывая исходники ядра и какие драйверы нам нужны
```
#!bash

cd ./netmap/LINUX/
./configure --kernel-sources=/usr/src/linux-3.10.90 --drivers=igb,ixgbe,e1000e
```
собираем:
```
#!bash

make
```
загружаем модули в систему:

```
#!bash

insmod /usr/src/netmap/LINUX/netmap.ko
# для 10Гб/сек сетевой карты Intel
rmmod ixgbe && insmod /usr/src/netmap/LINUX/ixgbe/ixgbe.ko
# для 1Гб/сек сетевой карты
rmmod igb && insmod /usr/src/netmap/LINUX/igb/igb.ko
rmmod e1000e && insmod /usr/src/netmap/LINUX/e1000e/e1000e.ko

```
после этого в системе должен появиться интерфейс работы с netmap:
```
#!bash

# ls /dev/netmap 
/dev/netmap
```

### Установка ddosdetector ###
собираем ddosdetector:

```
#!bash

git clone https://velizarx@bitbucket.org/velizarx/ddosdetector.git
cd ./ddosdetector
make
```

## Запуск ##
Для запуска у текущего пользователя должны быть права доступа на чтение и запись к интерфейсу netmap (*/dev/netmap*). Сетевой интерфейс должен быть включен. Драйвер сетевой карты с поддержкой netmap должен быть загружен:
```
#!bash

# lsmod | grep netmap
netmap                143360  27 ixgbe
# modinfo ixgbe | grep depends 
depends:        mdio,netmap,dca
```
**Если подключение к серверу удаленное (SSH, telnet и т.д.), запуск системы ddosdetector на том же сетевом интерфейсе, через который осуществляется подключение, приведет к потере доступа, так как netmap драйвер отключает сетевую карту от ОС!**

Запуск ddosdetector (в примере интерфейс eth4):
```
#!bash

cd <path_to_ddosdetector_directory>
./ddosdetector -i eth4 -p 9090 -l /tmp/ddosd.log
```
после этого к системе можно подключиться:
```
#!bash

telnet 127.1 9090
```

### Будущее ###
* действие триггера "dump" - для создания дампа трафика по определенному правилу;
* "monitor" отображение счетчиков в консоли управления. При вводе команды "monitor rules" страница будет выводить статистику и обновляться раз в секунду. Отмена по Ctrl^D;
* сбор per-ip данных в подсетях.
* SNMP сервер для мониторинга счетчиков правил