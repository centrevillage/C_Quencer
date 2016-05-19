# C_Quencer

## Operation Manual
  https://docs.google.com/document/d/13KKrxWZMQznam8sgkmz99NLFrwOqDBrAlAN6UPDG6dc/edit?usp=sharing

## Building Manual

https://docs.google.com/document/d/1b9S5hhJaEPMowWSLihDXAuJFFu3bsB46beQqKKnV82s/edit?usp=sharing

## BOM
  https://docs.google.com/spreadsheets/d/1VDif7U_uzp9MytvVjhtFVuhgSCxw1dvosUy1xRro134/edit?usp=sharing

## Compile and program to AVR(ATMEGA328)

### Download AVR Toolchain

* Windows
  http://www.atmel.com/ja/jp/tools/ATMELAVRTOOLCHAINFORWINDOWS.aspx
* Linux
  http://www.atmel.com/tools/atmelavrtoolchainforlinux.aspx

* Mac OSX
  https://www.obdev.at/products/crosspack/index.html

### Set enviroment variable

* Windows
```
set AVR_TOOLKIT_ROOT=(AVR Toolchain path)
```

* Linux/Mac OSX (bash)
```
export AVR_TOOLKIT_ROOT=(AVR Toolchain path)
```

### Compile

```
make
```

### Write fuse bit

Writing fuse bit by Arduino ISP
```
(AVR Toolchain path)/bin/avrdude -c avrisp -p m328p -U lfuse:w:0xFF:m -U hfuse:w:0xDE:m -U efuse:w:0x05:m -C (AVR Toolchain path)/etc/avrdude.conf
```

### Program to AVR

```
make program
```