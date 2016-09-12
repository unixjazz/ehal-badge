# e-HAL 2016 Badge

Repositório do crachá ('badge') para o ["1o Encontro Brasileiro de Hardware
Aberto e Livre" (e-HAL)](https://ehal.org.br).

O crachá foi baseado no projeto de hardware aberto "FSM-55" (Flying
Stone Matrix Go! Go!) cujas placas fabricadas pela Seeed Studio foram doadas
para o e-HAL pela empresa **Flying Stone Tech** do Japão.

Para maiores informacões sobre o FSM-55, visite a sua [página wiki oficial](http://wiki.seeedstudio.com/wiki/FSM-55). Para obter aos arquivos de design do crachá, acesse diretamente o [repositório oficial da Flying Stone Tech](http://git.gniibe.org/gitweb/?p=fsm-55.git).

Para obter o código fonte e os arquivos de exemplo do FSM-55, clone o
repositório oficinal do projeto:

``` 
git clone git://git.gniibe.org/chopstx/chopstx.git
```

## Compilando o firmware para o FSM-55

Para compilar o firmware, é necessário configurar o seu sistema para
cross-compilacão `ARM-Cortex-M0` (target: `stm32f0x.cpu`). É possível baixar o
pacote pre-compilado do `gcc-4.6 ou 4.8` da sua distribuicão ou baixar o tarball com o toolchain através do site do projeto [GNU ARM Embedded Toolchain](https://launchpad.net/gcc-arm-embedded). No Debian, o pacote pre-compilado do chama-se ''gcc-arm-none-eabi''.

Após a instalacao, assegure-se de que o compilador está no PATH re-exportando a variável de ambiente. Exemplo:

```
export PATH="$PATH:/usr/src/gcc-arm-none-eabi-4_8-2014q3/bin/"
```

Depois de instalado e configurado o ambiente de compilacao cruzada, é preciso clonar o repositório com o codigo-fonte do RTOS do FSM-55 e copiar os arquivos `ehal.c` e `Makefile` para o diretório `chopstx/example-fsm-55`. Após realizar a cópia, basta rodar `make` e o firmware será gerado no diretório `build/`.

## Programar o FSM-55

Após a compilacao do firmware, é necessário reprogramar o FSM-55. Existem atualmente 3 interfaces compativeis:

### Beaglebone Green (BBG):

[Opcão stand-alone que não requer o uso do OpenOCD](https://www.hackster.io/gniibe/bbg-swd-f6a408).

### STLink v2:

Esta é a opcão mais fácil, mas ela requer a utilizacão de um programador com firmware proprietário. 

Para utilizar o STLink v2, basta ligar o conector "CN3" do STLink com o conector K3 do FSM-55:

* Coloque um jumper no conector CN3 do pino 1 (VAPP) ao pino 19 (VDD). Para
  referencia completa da pinagem, [acesse o manual do STLink v2](http://www.st.com/content/ccc/resource/technical/document/user_manual/65/e0/44/72/9e/34/41/8d/DM00026748.pdf/files/DM00026748.pdf/jcr:content/translations/en.DM00026748.pdf).

```
- K3 (1) ---> CN3 (2)  VDD (3.3v)
- K3 (2) ---> CN3 (7)  SW IO 
- K3 (3) ---> CN3 (9)  SW CLK
- K3 (4) ---> CN3 (15) NRST (reset)
- K3 (5) ---> CN3 (20) GND (ground)
```

Após conectar o programador e o FSM-55, ligue o STLink na USB e digite o
seguinte comando para programar o firmware:

```
sudo openocd -f stlink-v2.cfg -c "program build/ehal.elf; reset run;
shutdown"
```

A saída do programa anunciará que o firmware foi escrito com sucesso:

``` 
** Programming Started ** 
auto erase enabled 
Info : device id = 0x10006444 
Info : flash size = 16kbytes target state: halted target halted due to breakpoint, current mode: Thread xPSR: 0x61000000 pc: 0x2000003a msp:
0x200004e0 wrote 4096 bytes from file build/ehal.elf in 0.473004s (8.457 KiB/s)
** Programming Finished ** 
in procedure 'reset' 
in procedure 'ocd_bouncer'
```

### Bus Pirate v3.6:

Esta é um procedimento completamente livre de tecnologias proprietárias, mas o seu suporte é experimental. 

O primeiro passo é compilar a versão do `openocd-0.9` com o patch aplicado para
extender o suporte SWD ao Bus Pirate. Este procedimento só funcionará se o seu
Bus Pirate possuir as versões seguintes de firmware e bootloader respectivamente: **>=v6.1 e >=4.4**.

Após a compilacao do `openocd`, é preciso conectar os probes do Bus Pirate no
FSM-55:

```
Probe 3.3v ---> K3 (vdd) 
      GND  ---> K3 (ground) 
      MOSI ---> K3 (sw-io) 
      CLK  ---> K3 (sw-clk)
           ---> K3 (reset)
```

O último passo é rodar o `openocd` com o arquivo de configuracao
`openocd.cfg` (incluído neste repositório):

```
sudo openocd -f openocd.cfg
```

Abra outra console e digite os seguintes comandos para escrever o firmware
compilado (e gerado na pasta `../chopstx/example-fsm-55/build`):

```
telnet localhost 4444
init
reset halt
write ehal.elf
shutdown
``` 

Se os comandos acima forem executados com êxito, o FSM-55 estará reprogramado
com o novo firmware.

## Resultado

Após reprogramar o crachá, ele deverá mostrar 'e-HAL' na matriz de LEDs 5x5
conforme o exemplo:

[FSM-55]: ehal.gif "FSM-55 com firmware do e-HAL"

## Licenca

O projeto FSM-55 de software e hardware livre foi fabricado pela Seeed
Studio e projetado por Flying Stone Tech (tm). O projeto é de autoria de [NIIBE
Yutaka](), cujo apoio incondicional ao desenvolvimento do hardware e software
livre serve de inspiracão. Somos gratos pela doacao de crachás para e-HAL 2016 realizada pela empresa Flying Stone Tech.

OpenOCD é um projeto de sofware livre, cuja licenca pode ser obtida no
diretório 'openocd-0.9.0_patched' no arquivo LICENSE. Para maiores informacoes, acesse o [site oficial do projeto](http://openocd.org).

