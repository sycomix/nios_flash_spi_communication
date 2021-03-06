#include "flash_spi_lib.h"

#ifdef SPI_MEDIATOR_BASE
#elif  BIT(x,n)
#endif

uint32_t spi_mediator_base_address = SPI_MEDIATOR_BASE;
// change this to the corresponding address

uint8_t input[20] = { 0 }; // **
// Idealmente aca iría un malloc(), pero esto permite ahorrar esa librería
// sin mayores problemas.

// ****************************************************************************
// *                        FUNCIONES DE IDENTIFICACION                       *
// ****************************************************************************

void read_id()
{
  alt_putstr("\n");
  alt_putstr("*** READ_ID ***\n");

  uint8_t comando[4] = {0};
  split_32_to_8_bits(0x90000000, comando);

  alt_avalon_spi_command(SPI_MEDIATOR_BASE, 0, 4,
    (alt_u8*)comando, 2, input, 0);
  alt_printf("%x %x %x %x \n", comando[0], comando[1], comando[2], comando[3]);
  alt_printf("%x %x \n", input[0], input[1]);
}

// ****************************************************************************
// *                              RESET FUNCTION                              *
// ****************************************************************************

void reset_command()
{
  alt_putstr("\n");
  alt_putstr("*** SOFTWARE RESET ***\n");

  uint8_t comando = 0xf0;

  alt_avalon_spi_command(SPI_MEDIATOR_BASE, 0, 1, &comando, 0, input, 0);
  alt_putstr("*** DONE ***\n");
}

// ****************************************************************************
// *                      FUNCIONES DE LECTURA/ESCRITURA                      *
// ****************************************************************************

void sector_erase(uint32_t add)

// Antes de escribir a alguna dirección su valor debe estar borrado con
// sector erase según la siguiente distribucion :

// 4kbyte x 32    -- sectors SA00  -- address range 0x0000_0000 - 0x0000_0FFF
//                -- sectors ....  -- address range ........................
//                -- sectors SA31  -- address range 0x0001_F000 - 0x0001_FFFF
// 64kbyte x 510  -- sectors SA32  -- address range 0x0002_0000 - 0x0002_FFFF
//                -- sectors ....  -- address range ........................
//                -- sectors SA541 -- address range 0x01FF_0000 - 0x0002_FFFF

// -Nota: Sector erase es una funcion de escritura y necesita un flanco de WE
// cuando se ejecuta esta acción. Se deshabilita inmeditamente después de
// ejecutada

{
  alt_putstr("\n");
  alt_putstr("*** Clear memory sector ***\n");

  // Sector erase, necesita write enable activado
  uint8_t address_splitted[4] = {0};
  split_32_to_8_bits(add, address_splitted);

  uint8_t comando[5] = {0xdc, address_splitted[0], address_splitted[1],
    address_splitted[2], address_splitted[3]};

  alt_avalon_spi_command(SPI_MEDIATOR_BASE, 0, 5,
    (alt_u8*)comando, 0, input, 0);
  alt_printf("%x\n %x %x %x %x\n",
    comando[0],
    comando[1],
    comando[2],
    comando[3],
    comando[4],
    comando[5]);

  // lock function until write completes
  check_write_in_progress();
}

void write_memory(uint32_t add, uint8_t value)

// Funcion simple de escritura a una dirección de memoria.
// Se necesita la dirección limpia con la funcion sector erase y un flanco de
// WE activado en el status register.

{
  alt_putstr("\n");
  alt_putstr("*** Write to memory ***\n");

  // page programming (write action)
  uint8_t address_splitted[4] = {0};
  split_32_to_8_bits(add, address_splitted);

  uint8_t comando[6] = {0x12, address_splitted[0], address_splitted[1],
    address_splitted[2], address_splitted[3], value};

  alt_avalon_spi_command(SPI_MEDIATOR_BASE, 0, 6,
    (alt_u8*)comando, 0, input, 0);
  alt_printf("%x\n %x %x %x %x\n %x\n",
    comando[0],
    comando[1],
    comando[2],
    comando[3],
    comando[4],
    comando[5],
    comando[6]);

  // lock function until write completes
  check_write_in_progress();
}

uint8_t read_add(uint32_t add)
{
  alt_putstr("\n*** READ ADDRESS ***\n");

  uint8_t address[4] = { 0 };
  split_32_to_8_bits(add, address);

  // Construccion de comando
  uint8_t comando[5] = {0x13, address[0], address[1], address[2], address[3]};

  alt_avalon_spi_command(SPI_MEDIATOR_BASE, 0, 5,
    (alt_u8*)comando, 1, input, 0);
  alt_printf("%x %x %x %x %x\n",
    comando[0], comando[1], comando[2], comando[3], comando[4]);

  return input[0];
}

void read_add_bulk(uint32_t add, uint32_t num_data, uint8_t * data)
// Funcion para realizar lectura de múltiples elementos
// La dirección add da el inicio del lugar de lectura y luego los elementos de
// las siguientes direcciones salen de forma automática.
{
  alt_putstr("\n*** READ ADDRESS BULK ***\n");

  uint8_t address[4] = { 0 };
  split_32_to_8_bits(add, address);

  // Construccion de comando
  uint8_t comando[5] = {0x13, address[0], address[1], address[2], address[3]};

  alt_avalon_spi_command(SPI_MEDIATOR_BASE, 0, 5, (alt_u8*)comando,
                         num_data, data, 0);
  alt_printf("%x %x %x %x %x\n",
    comando[0], comando[1], comando[2], comando[3], comando[4]);
}

void write_add_bulk(uint32_t add, const uint32_t num_data, uint8_t * data)
{
  alt_putstr("\n*** WRITE ADDRESS BULK ***\n");

  uint8_t address[4] = { 0 };
  split_32_to_8_bits(add, address);

  // Construccion de comando
  uint8_t comando[WRITE_BUFFER] =
    {0x12, address[0], address[1], address[2], address[3]};

  uint8_t i = 0;
  for (i = 0; i < num_data; i++) {
    comando[5 + i] = *(data + i);
  }

  alt_avalon_spi_command(SPI_MEDIATOR_BASE, 0, 5 + num_data, (alt_u8*)comando,
                         0, NULL, 0);

  for(i=0; i < 5 + num_data; i++){
    if (i == 4)
      alt_printf("%x\n",comando[i]);
    else
      alt_printf("%x ",comando[i]);
  }
  alt_printf("\n");
  check_write_in_progress();
}

void read_autoboot(uint32_t num_data, uint8_t * data)
{
  alt_putstr("\n*** READ AUTOBOOT ***\n");
  alt_avalon_spi_command(SPI_MEDIATOR_BASE, 0, 0, NULL,
                         num_data, data, 0);
}

// ****************************************************************************
// *                     STATUS REGISTER RELATED FUNCTIONS                    *
// ****************************************************************************

void write_enable()
{
  alt_putstr("\n");
  alt_putstr("*** Write enable ***\n");

  // write enable
  uint8_t wren = 0x06;
  alt_avalon_spi_command(SPI_MEDIATOR_BASE, 0, 1, (alt_u8*) &wren, 0, input, 0);
  check_write_enable(true);
}

void write_disable()
{
  // write disable
  uint8_t wrdi = 0x04;
  alt_avalon_spi_command(SPI_MEDIATOR_BASE, 0, 1, (alt_u8*) &wrdi, 0, input, 0);
  check_write_enable(false);
}

void clear_status_register()
{
  alt_putstr("\n");
  alt_putstr("*** CLEAR STATUS REGISTER ***\n");

  uint8_t comando = 0x30;
  alt_avalon_spi_command(SPI_MEDIATOR_BASE, 0, 1, &comando, 0, input, 0);
}

void check_write_enable(bool value_target)
// Funcion que bloquea el avance del programa hasta que write enable este
// en el valor esperado
{
  while(1) {
    uint8_t status = read_status_register();
    if (BIT(status, 1) == value_target) break;
  }
}

void check_write_in_progress()
// Funcion que bloquea el avance del programa hasta que el trabajo de escritura
// se haya completado
{
  while(1) {
    uint8_t status = read_status_register();
    if (BIT(status, 0) == 0) break;
  }
}

uint8_t read_status_register()
// Read status register 1
{
  // alt_putstr("\n");
  // alt_putstr("*** READ STATUS REGISTER ***\n");

  uint8_t comando = 0x05;
  alt_avalon_spi_command(SPI_MEDIATOR_BASE, 0, 1, &comando, 1, input, 0);
  // alt_printf("%x\n", input[0]);

  return input[0];
}

// ****************************************************************************
// *                             UTILITY FUNCTIONS                            *
// ****************************************************************************

void split_32_to_8_bits(uint32_t number, uint8_t * pointer)
{
  // Descomposicion del address en numeros de 8 bits
  uint32_t mask = 0xff; // mascara de 8'b 1111_1111
  uint8_t number_split[4] =
    {(number >> 24) & mask, (number >> 16) & mask, (number >> 8) & mask,
      number & mask};

  // Memcpy function not available
  int i = 0;
  for (i = 0; i < 4; i++) {
    *pointer = number_split[i]; pointer++;
  }
}
